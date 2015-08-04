/* http_dav.c -- Routines for dealing with DAV properties in httpd
 *
 * Copyright (c) 1994-2011 Carnegie Mellon University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Carnegie Mellon University
 *      Center for Technology Transfer and Enterprise Creation
 *      4615 Forbes Avenue
 *      Suite 302
 *      Pittsburgh, PA  15213
 *      (412) 268-7393, fax: (412) 268-7395
 *      innovation@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * TODO:
 *
 *   - CALDAV:supported-calendar-component-set should be a bitmask in
 *     cyrus.index header Mailbox Options field
 *
 *   - CALDAV:schedule-calendar-transp should be a flag in
 *     cyrus.index header (Mailbox Options)
 *
 *   - DAV:creationdate sould be added to cyrus.header since it only
 *     gets set at creation time
 *
 *   - Should add a last_metadata_update field to cyrus.index header
 *     for use in PROPFIND, PROPPATCH, and possibly REPORT.
 *     This would get updated any time a mailbox annotation, mailbox
 *     acl, or quota root limit is changed
 *
 *   - Should we use cyrus.index header Format field to indicate
 *     CalDAV mailbox?
 *
 */

#include "http_dav.h"
#include "annotate.h"
#include "acl.h"
#include "append.h"
#include "caldav_db.h"
#include "global.h"
#include "http_proxy.h"
#include "index.h"
#include "proxy.h"
#include "times.h"
#include "syslog.h"
#include "strhash.h"
#include "tok.h"
#include "xmalloc.h"
#include "xml_support.h"
#include "xstrlcat.h"
#include "xstrlcpy.h"

/* generated headers are not necessarily in current directory */
#include "imap/http_err.h"
#include "imap/imap_err.h"

#include <libxml/uri.h>

#define SYNC_TOKEN_URL_SCHEME "data:,"

static const struct dav_namespace_t {
    const char *href;
    const char *prefix;
} known_namespaces[] = {
    { XML_NS_DAV, "D" },
    { XML_NS_CALDAV, "C" },
    { XML_NS_CARDDAV, "C" },
    { XML_NS_ISCHED, NULL },
    { XML_NS_CS, "CS" },
    { XML_NS_CYRUS, "CY" },
    { XML_NS_USERFLAG, "UF" },
    { XML_NS_SYSFLAG, "SF" },
};

static int server_info(struct transaction_t *txn);
static int principal_parse_path(const char *path, struct request_target_t *tgt,
                                const char **errstr);
static int propfind_displayname(const xmlChar *name, xmlNsPtr ns,
                                struct propfind_ctx *fctx,
                                xmlNodePtr prop, xmlNodePtr resp,
                                struct propstat propstat[], void *rock);
static int propfind_restype(const xmlChar *name, xmlNsPtr ns,
                            struct propfind_ctx *fctx,
                            xmlNodePtr prop, xmlNodePtr resp,
                            struct propstat propstat[], void *rock);
static int propfind_alturiset(const xmlChar *name, xmlNsPtr ns,
                              struct propfind_ctx *fctx,
                              xmlNodePtr prop, xmlNodePtr resp,
                              struct propstat propstat[], void *rock);
#define propfind_principalurl  propfind_owner

static int report_prin_prop_search(struct transaction_t *txn,
                                   struct meth_params *rparams,
                                   xmlNodePtr inroot,
                                   struct propfind_ctx *fctx);
static int report_prin_search_prop_set(struct transaction_t *txn,
                                       struct meth_params *rparams,
                                       xmlNodePtr inroot,
                                       struct propfind_ctx *fctx);

static int allprop_cb(const char *mailbox __attribute__((unused)),
                      uint32_t uid __attribute__((unused)),
                      const char *entry,
                      const char *userid, const struct buf *attrib,
                      void *rock);

/* Array of supported REPORTs */
static const struct report_type_t principal_reports[] = {

    /* WebDAV Versioning (RFC 3253) REPORTs */
    { "expand-property", NS_DAV, "multistatus", &report_expand_prop,
      DACL_READ, 0 },

    /* WebDAV ACL (RFC 3744) REPORTs */
    { "principal-property-search", NS_DAV, "multistatus",
      &report_prin_prop_search, 0, REPORT_ALLOW_PROPS | REPORT_DEPTH_ZERO },
    { "principal-search-property-set", NS_DAV, "principal-search-property-set",
      &report_prin_search_prop_set, 0, REPORT_DEPTH_ZERO },

    { NULL, 0, NULL, NULL, 0, 0 }
};

/* Array of known "live" properties */
static const struct prop_entry principal_props[] = {

    /* WebDAV (RFC 4918) properties */
    { "creationdate", NS_DAV, PROP_ALLPROP, NULL, NULL, NULL },
    { "displayname", NS_DAV, PROP_ALLPROP | PROP_COLLECTION,
      propfind_displayname, NULL, NULL },
    { "getcontentlanguage", NS_DAV, PROP_ALLPROP, NULL, NULL, NULL },
    { "getcontentlength", NS_DAV, PROP_ALLPROP | PROP_COLLECTION,
      propfind_getlength, NULL, NULL },
    { "getcontenttype", NS_DAV, PROP_ALLPROP, NULL, NULL, NULL },
    { "getetag", NS_DAV, PROP_ALLPROP, NULL, NULL, NULL },
    { "getlastmodified", NS_DAV, PROP_ALLPROP, NULL, NULL, NULL },
    { "lockdiscovery", NS_DAV, PROP_ALLPROP | PROP_COLLECTION,
      propfind_lockdisc, NULL, NULL },
    { "resourcetype", NS_DAV, PROP_ALLPROP | PROP_COLLECTION,
      propfind_restype, NULL, NULL },
    { "supportedlock", NS_DAV, PROP_ALLPROP | PROP_COLLECTION,
      propfind_suplock, NULL, NULL },

    /* WebDAV Versioning (RFC 3253) properties */
    { "supported-report-set", NS_DAV, PROP_COLLECTION,
      propfind_reportset, NULL, (void *) principal_reports },

    /* WebDAV ACL (RFC 3744) properties */
    { "alternate-URI-set", NS_DAV, PROP_COLLECTION,
      propfind_alturiset, NULL, NULL },
    { "principal-URL", NS_DAV, PROP_COLLECTION,
      propfind_principalurl, NULL, NULL },
    { "group-member-set", NS_DAV, 0, NULL, NULL, NULL },
    { "group-membership", NS_DAV, 0, NULL, NULL, NULL },
    { "principal-collection-set", NS_DAV, PROP_COLLECTION,
      propfind_princolset, NULL, NULL },

    /* WebDAV Current Principal (RFC 5397) properties */
    { "current-user-principal", NS_DAV, PROP_COLLECTION,
      propfind_curprin, NULL, NULL },

    /* draft-douglass-server-info */
    { "server-info-href", NS_DAV, PROP_COLLECTION,
      propfind_serverinfo, NULL, NULL },

    /* CalDAV (RFC 4791) properties */
    { "calendar-home-set", NS_CALDAV, PROP_COLLECTION,
      propfind_calurl, NULL, NULL },

    /* CalDAV Scheduling (RFC 6638) properties */
    { "schedule-inbox-URL", NS_CALDAV, PROP_COLLECTION,
      propfind_calurl, NULL, SCHED_INBOX },
    { "schedule-outbox-URL", NS_CALDAV, PROP_COLLECTION,
      propfind_calurl, NULL, SCHED_OUTBOX },
    { "calendar-user-address-set", NS_CALDAV, PROP_COLLECTION,
      propfind_caluseraddr, NULL, NULL },
    { "calendar-user-type", NS_CALDAV, PROP_COLLECTION,
      propfind_calusertype, NULL, NULL },

    /* CardDAV (RFC 6352) properties */
    { "addressbook-home-set", NS_CARDDAV, PROP_COLLECTION,
      propfind_abookhome, NULL, NULL },

    /* Apple Calendar Server properties */
    { "getctag", NS_CS, PROP_ALLPROP, NULL, NULL, NULL },

    { NULL, 0, 0, NULL, NULL, NULL }
};


static struct meth_params princ_params = {
    .parse_path = &principal_parse_path,
    .lprops = principal_props,
    .reports = principal_reports
};

/* Namespace for WebDAV principals */
struct namespace_t namespace_principal = {
    URL_NS_PRINCIPAL, 0, "/dav/principals", NULL, 1 /* auth */,
    /*mbtype */ 0,
    ALLOW_READ | ALLOW_DAV,
    NULL, NULL, NULL, NULL,
    {
        { NULL,                 NULL },                 /* ACL          */
        { NULL,                 NULL },                 /* COPY         */
        { NULL,                 NULL },                 /* DELETE       */
        { &meth_get_head,       &princ_params },        /* GET          */
        { &meth_get_head,       &princ_params },        /* HEAD         */
        { NULL,                 NULL },                 /* LOCK         */
        { NULL,                 NULL },                 /* MKCALENDAR   */
        { NULL,                 NULL },                 /* MKCOL        */
        { NULL,                 NULL },                 /* MOVE         */
        { &meth_options,        NULL },                 /* OPTIONS      */
        { NULL,                 NULL },                 /* POST         */
        { &meth_propfind,       &princ_params },        /* PROPFIND     */
        { NULL,                 NULL },                 /* PROPPATCH    */
        { NULL,                 NULL },                 /* PUT          */
        { &meth_report,         &princ_params },        /* REPORT       */
        { &meth_trace,          NULL },                 /* TRACE        */
        { NULL,                 NULL }                  /* UNLOCK       */
    }
};


/* Linked-list of properties for fetching */
struct propfind_entry_list {
    xmlChar *name;                      /* Property name (needs to be freed) */
    xmlNsPtr ns;                        /* Property namespace */
    unsigned char flags;                /* Flags for how/where prop apply */
    int (*get)(const xmlChar *name,     /* Callback to fetch property */
               xmlNsPtr ns, struct propfind_ctx *fctx, xmlNodePtr prop,
               xmlNodePtr resp, struct propstat propstat[], void *rock);
    xmlNodePtr prop;                    /* Property node from request */
    void *rock;                         /* Add'l data to pass to callback */
    struct propfind_entry_list *next;
};


/* Bitmask of privilege flags */
enum {
    PRIV_IMPLICIT =             (1<<0),
    PRIV_INBOX =                (1<<1),
    PRIV_OUTBOX =               (1<<2)
};


/* Array of precondition/postcondition errors */
static const struct precond_t {
    const char *name;                   /* Property name */
    unsigned ns;                        /* Index into known namespace array */
} preconds[] = {
    /* Placeholder for zero (no) precondition code */
    { NULL, 0 },

    /* WebDAV (RFC 4918) preconditons */
    { "cannot-modify-protected-property", NS_DAV },
    { "lock-token-matches-request-uri", NS_DAV },
    { "lock-token-submitted", NS_DAV },
    { "no-conflicting-lock", NS_DAV },

    /* WebDAV Versioning (RFC 3253) preconditions */
    { "supported-report", NS_DAV },
    { "resource-must-be-null", NS_DAV },

    /* WebDAV ACL (RFC 3744) preconditions */
    { "need-privileges", NS_DAV },
    { "no-invert", NS_DAV },
    { "no-abstract", NS_DAV },
    { "not-supported-privilege", NS_DAV },
    { "recognized-principal", NS_DAV },
    { "allowed-principal", NS_DAV },
    { "grant-only", NS_DAV },

    /* WebDAV Quota (RFC 4331) preconditions */
    { "quota-not-exceeded", NS_DAV },
    { "sufficient-disk-space", NS_DAV },

    /* WebDAV Extended MKCOL (RFC 5689) preconditions */
    { "valid-resourcetype", NS_DAV },

    /* WebDAV Sync (RFC 6578) preconditions */
    { "valid-sync-token", NS_DAV },
    { "number-of-matches-within-limits", NS_DAV },

    /* CalDAV (RFC 4791) preconditions */
    { "supported-calendar-data", NS_CALDAV },
    { "valid-calendar-data", NS_CALDAV },
    { "valid-calendar-object-resource", NS_CALDAV },
    { "supported-calendar-component", NS_CALDAV },
    { "calendar-collection-location-ok", NS_CALDAV },
    { "no-uid-conflict", NS_CALDAV },
    { "supported-filter", NS_CALDAV },
    { "valid-filter", NS_CALDAV },

    /* RSCALE (RFC 7529) preconditions */
    { "supported-rscale", NS_CALDAV },

    /* TZ by Ref (draft-ietf-tzdist-caldav-timezone-ref) preconditions */
    { "valid-timezone", NS_CALDAV },

    /* Managed Attachments (draft-daboo-caldav-attachments) preconditions */
    { "valid-managed-id", NS_CALDAV },

    /* CalDAV Scheduling (RFC 6638) preconditions */
    { "valid-scheduling-message", NS_CALDAV },
    { "valid-organizer", NS_CALDAV },
    { "unique-scheduling-object-resource", NS_CALDAV },
    { "same-organizer-in-all-components", NS_CALDAV },
    { "allowed-organizer-scheduling-object-change", NS_CALDAV },
    { "allowed-attendee-scheduling-object-change", NS_CALDAV },

    /* iSchedule (draft-desruisseaux-ischedule) preconditions */
    { "version-not-supported", NS_ISCHED },
    { "invalid-calendar-data-type", NS_ISCHED },
    { "invalid-calendar-data", NS_ISCHED },
    { "invalid-scheduling-message", NS_ISCHED },
    { "originator-missing", NS_ISCHED },
    { "too-many-originators", NS_ISCHED },
    { "originator-invalid", NS_ISCHED },
    { "originator-denied", NS_ISCHED },
    { "recipient-missing", NS_ISCHED },
    { "recipient-mismatch", NS_ISCHED },
    { "verification-failed", NS_ISCHED },

    /* CardDAV (RFC 6352) preconditions */
    { "supported-address-data", NS_CARDDAV },
    { "valid-address-data", NS_CARDDAV },
    { "no-uid-conflict", NS_CARDDAV },
    { "addressbook-collection-location-ok", NS_CARDDAV },
    { "supported-filter", NS_CARDDAV }
};


/* Parse request-target path in DAV principals namespace */
static int principal_parse_path(const char *path, struct request_target_t *tgt,
                                const char **errstr)
{
    char *p;
    size_t len;

    /* Make a working copy of target path */
    strlcpy(tgt->path, path, sizeof(tgt->path));
    tgt->tail = tgt->path + strlen(tgt->path);

    p = tgt->path;

    /* Sanity check namespace */
    len = strlen(namespace_principal.prefix);
    if (strlen(p) < len ||
        strncmp(namespace_principal.prefix, p, len) ||
        (path[len] && path[len] != '/')) {
        *errstr = "Namespace mismatch request target path";
        return HTTP_FORBIDDEN;
    }

    tgt->prefix = namespace_principal.prefix;

    /* Skip namespace */
    p += len;
    if (!*p || !*++p) return 0;

    /* Check if we're in user space */
    len = strcspn(p, "/");
    if (!strncmp(p, "user", len)) {
        p += len;
        if (!*p || !*++p) return 0;

        /* Get user id */
        len = strcspn(p, "/");
        tgt->userid = xstrndup(p, len);

        p += len;
        if (!*p || !*++p) return 0;
    }
    else if (!strncmp(p, SERVER_INFO, len)) {
        p += len;
        if (!*p || !*++p) {
            tgt->flags = TGT_SERVER_INFO;
            return 0;
        }
    }
    else return HTTP_NOT_FOUND;  /* need to specify a userid */

    if (*p) {
//      *errstr = "Too many segments in request target path";
        return HTTP_NOT_FOUND;
    }

    return 0;
}


/* Evaluate If header.  Note that we can't short-circuit any of the tests
   because we need to check for a lock-token anywhere in the header */
static int eval_if(const char *hdr, const char *etag, const char *lock_token,
                   unsigned *locked)
{
    unsigned ret = 0;
    tok_t tok_l;
    char *list;

    /* Process each list, ORing the results */
    tok_init(&tok_l, hdr, ")", TOK_TRIMLEFT|TOK_TRIMRIGHT);
    while ((list = tok_next(&tok_l))) {
        unsigned ret_l = 1;
        tok_t tok_c;
        char *cond;

        /* XXX  Need to handle Resource-Tag for Tagged-list (COPY/MOVE dest) */

        /* Process each condition, ANDing the results */
        tok_initm(&tok_c, list+1, "]>", TOK_TRIMLEFT|TOK_TRIMRIGHT);
        while ((cond = tok_next(&tok_c))) {
            unsigned r, not = 0;

            if (!strncmp(cond, "Not", 3)) {
                not = 1;
                cond += 3;
                while (*cond == ' ') cond++;
            }
            if (*cond == '[') {
                /* ETag */
                r = !etagcmp(cond+1, etag);
            }
            else {
                /* State Token */
                if (!lock_token) r = 0;
                else {
                    r = !strcmp(cond+1, lock_token);
                    if (r) {
                        /* Correct lock-token has been provided */
                        *locked = 0;
                    }
                }
            }

            ret_l &= (not ? !r : r);
        }

        tok_fini(&tok_c);

        ret |= ret_l;
    }

    tok_fini(&tok_l);

    return (ret || locked);
}


/* Check headers for any preconditions */
int dav_check_precond(struct transaction_t *txn, const void *data,
                      const char *etag, time_t lastmod)
{
    const struct dav_data *ddata = (const struct dav_data *) data;
    hdrcache_t hdrcache = txn->req_hdrs;
    const char **hdr;
    const char *lock_token = NULL;
    unsigned locked = 0;

    /* Check for a write-lock on the source */
    if (ddata && ddata->lock_expire > time(NULL)) {
        lock_token = ddata->lock_token;

        switch (txn->meth) {
        case METH_DELETE:
        case METH_LOCK:
        case METH_MOVE:
        case METH_POST:
        case METH_PUT:
            /* State-changing method: Only the lock owner can execute
               and MUST provide the correct lock-token in an If header */
            if (strcmp(ddata->lock_ownerid, httpd_userid)) return HTTP_LOCKED;

            locked = 1;
            break;

        case METH_UNLOCK:
            /* State-changing method: Authorized in meth_unlock() */
            break;

        case METH_ACL:
        case METH_MKCALENDAR:
        case METH_MKCOL:
        case METH_PROPPATCH:
            /* State-changing method: Locks on collections unsupported */
            break;

        default:
            /* Non-state-changing method: Always allowed */
            break;
        }
    }

    /* Per RFC 4918, If is similar to If-Match, but with lock-token submission.
       Per RFC 7232, LOCK errors supercede preconditions */
    if ((hdr = spool_getheader(hdrcache, "If"))) {
        /* State tokens (sync-token, lock-token) and Etags */
        if (!eval_if(hdr[0], etag, lock_token, &locked))
            return HTTP_PRECOND_FAILED;
    }

    if (locked) {
        /* Correct lock-token was not provided in If header */
        return HTTP_LOCKED;
    }


    /* Do normal HTTP checks */
    return check_precond(txn, etag, lastmod);
}


unsigned get_preferences(struct transaction_t *txn)
{
    unsigned mask = 0, prefs = 0;
    const char **hdr;

    /* Create a mask for preferences honored by method */
    switch (txn->meth) {
    case METH_COPY:
    case METH_MOVE:
    case METH_POST:
    case METH_PUT:
        mask = PREFER_REP;
        break;

    case METH_MKCALENDAR:
    case METH_MKCOL:
    case METH_PROPPATCH:
        mask = PREFER_MIN;
        break;

    case METH_PROPFIND:
    case METH_REPORT:
        mask = (PREFER_MIN | PREFER_NOROOT);
        break;
    }

    if (!mask) return 0;
    else {
        txn->flags.vary |= VARY_PREFER;
        if (mask & PREFER_MIN) txn->flags.vary |= VARY_BRIEF;
    }

    /* Check for Prefer header(s) */
    if ((hdr = spool_getheader(txn->req_hdrs, "Prefer"))) {
        int i;
        for (i = 0; hdr[i]; i++) {
            tok_t tok;
            char *token;

            tok_init(&tok, hdr[i], ",\r\n", TOK_TRIMLEFT|TOK_TRIMRIGHT);
            while ((token = tok_next(&tok))) {
                if ((mask & PREFER_MIN) &&
                    !strcmp(token, "return=minimal"))
                    prefs |= PREFER_MIN;
                else if ((mask & PREFER_REP) &&
                         !strcmp(token, "return=representation"))
                    prefs |= PREFER_REP;
                else if ((mask & PREFER_NOROOT) &&
                         !strcmp(token, "depth-noroot"))
                    prefs |= PREFER_NOROOT;
            }
            tok_fini(&tok);
        }

        txn->resp_body.prefs = prefs;
    }

    /* Check for Brief header */
    if ((mask & PREFER_MIN) &&
        (hdr = spool_getheader(txn->req_hdrs, "Brief")) &&
        !strcasecmp(hdr[0], "t")) {
        prefs |= PREFER_MIN;
    }

    return prefs;
}


/* Check requested MIME type */
struct mime_type_t *get_accept_type(const char **hdr, struct mime_type_t *types)
{
    struct mime_type_t *ret = NULL;
    struct accept *e, *enc = parse_accept(hdr);

    for (e = enc; e && e->token; e++) {
        if (!ret && e->qual > 0.0) {
            struct mime_type_t *m;

            for (m = types; !ret && m->content_type; m++) {
                if (is_mediatype(e->token, m->content_type)) ret = m;
            }
        }

        free(e->token);
    }
    if (enc) free(enc);

    return ret;
}


static int add_privs(int rights, unsigned flags,
                     xmlNodePtr parent, xmlNodePtr root, xmlNsPtr *ns);


/* Ensure that we have a given namespace.  If it doesn't exist in what we
 * parsed in the request, create it and attach to 'node'.
 */
int ensure_ns(xmlNsPtr *respNs, int ns, xmlNodePtr node,
              const char *url, const char *prefix)
{
    if (!respNs[ns]) {
        xmlNsPtr nsDef;
        char myprefix[20];

        /* Search for existing namespace using our prefix */
        for (nsDef = node->nsDef; nsDef; nsDef = nsDef->next) {
            if ((!nsDef->prefix && !prefix) ||
                (nsDef->prefix && prefix &&
                 !strcmp((const char *) nsDef->prefix, prefix))) break;
        }

        if (nsDef) {
            /* Prefix is already used - generate a new one */
            snprintf(myprefix, sizeof(myprefix), "X%X", strhash(url) & 0xffff);
            prefix = myprefix;
        }

        respNs[ns] = xmlNewNs(node, BAD_CAST url, BAD_CAST prefix);
    }

    /* XXX  check for errors */
    return 0;
}


/* Add namespaces declared in the request to our root node and Ns array */
static int xml_add_ns(xmlNodePtr req, xmlNsPtr *respNs, xmlNodePtr root)
{
    for (; req; req = req->next) {
        if (req->type == XML_ELEMENT_NODE) {
            xmlNsPtr nsDef;

            for (nsDef = req->nsDef; nsDef; nsDef = nsDef->next) {
                if (!xmlStrcmp(nsDef->href, BAD_CAST XML_NS_DAV))
                    ensure_ns(respNs, NS_DAV, root,
                              (const char *) nsDef->href,
                              (const char *) nsDef->prefix);
                else if (!xmlStrcmp(nsDef->href, BAD_CAST XML_NS_CALDAV))
                    ensure_ns(respNs, NS_CALDAV, root,
                              (const char *) nsDef->href,
                              (const char *) nsDef->prefix);
                else if (!xmlStrcmp(nsDef->href, BAD_CAST XML_NS_CARDDAV))
                    ensure_ns(respNs, NS_CARDDAV, root,
                              (const char *) nsDef->href,
                              (const char *) nsDef->prefix);
                else if (!xmlStrcmp(nsDef->href, BAD_CAST XML_NS_CS))
                    ensure_ns(respNs, NS_CS, root,
                              (const char *) nsDef->href,
                              (const char *) nsDef->prefix);
                else if (!xmlStrcmp(nsDef->href, BAD_CAST XML_NS_CYRUS))
                    ensure_ns(respNs, NS_CYRUS, root,
                              (const char *) nsDef->href,
                              (const char *) nsDef->prefix);
                else
                    xmlNewNs(root, nsDef->href, nsDef->prefix);
            }
        }

        xml_add_ns(req->children, respNs, root);
    }

    /* XXX  check for errors */
    return 0;
}


/* Initialize an XML tree for a property response */
xmlNodePtr init_xml_response(const char *resp, int ns,
                             xmlNodePtr req, xmlNsPtr *respNs)
{
    /* Start construction of our XML response tree */
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = NULL;

    if (!doc) return NULL;
    if (!(root = xmlNewNode(NULL, BAD_CAST resp))) return NULL;

    xmlDocSetRootElement(doc, root);

    /* Add namespaces from request to our response,
     * creating array of known namespaces that we can reference later.
     */
    memset(respNs, 0, NUM_NAMESPACE * sizeof(xmlNsPtr));
    xml_add_ns(req, respNs, root);

    /* Set namespace of root node */
    if (ns == NS_REQ_ROOT) xmlSetNs(root, req->ns);
    else {
        ensure_ns(respNs, ns, root,
                  known_namespaces[ns].href, known_namespaces[ns].prefix);
        xmlSetNs(root, respNs[ns]);
    }

    return root;
}

xmlNodePtr xml_add_href(xmlNodePtr parent, xmlNsPtr ns, const char *href)
{
    xmlChar *uri = xmlURIEscapeStr(BAD_CAST href, BAD_CAST ":/?=");
    xmlNodePtr node = xmlNewChild(parent, ns, BAD_CAST "href", uri);

    free(uri);
    return node;
}

xmlNodePtr xml_add_error(xmlNodePtr root, struct error_t *err,
                         xmlNsPtr *avail_ns)
{
    xmlNsPtr ns[NUM_NAMESPACE];
    xmlNodePtr error, node;
    const struct precond_t *precond = &preconds[err->precond];
    unsigned err_ns = NS_DAV;
    const char *resp_desc = "responsedescription";

    if (precond->ns == NS_ISCHED) {
        err_ns = NS_ISCHED;
        resp_desc = "response-description";
    }

    if (!root) {
        error = root = init_xml_response("error", err_ns, NULL, ns);
        avail_ns = ns;
    }
    else error = xmlNewChild(root, NULL, BAD_CAST "error", NULL);

    ensure_ns(avail_ns, precond->ns, root, known_namespaces[precond->ns].href,
              known_namespaces[precond->ns].prefix);
    node = xmlNewChild(error, avail_ns[precond->ns],
                       BAD_CAST precond->name, NULL);

    switch (err->precond) {
    case DAV_NEED_PRIVS:
        if (err->resource && err->rights) {
            unsigned flags = 0;
            size_t rlen = strlen(err->resource);
            const char *p = err->resource + rlen;

            node = xmlNewChild(node, NULL, BAD_CAST "resource", NULL);
            xml_add_href(node, NULL, err->resource);

            if (rlen > 6 && !strcmp(p-6, SCHED_INBOX))
                flags = PRIV_INBOX;
            else if (rlen > 7 && !strcmp(p-7, SCHED_OUTBOX))
                flags = PRIV_OUTBOX;

            add_privs(err->rights, flags, node, root, avail_ns);
        }
        break;

    default:
        if (err->resource) xml_add_href(node, avail_ns[NS_DAV], err->resource);
        break;
    }

    if (err->desc) {
        xmlNewTextChild(error, NULL, BAD_CAST resp_desc, BAD_CAST err->desc);
    }

    return root;
}


void xml_add_lockdisc(xmlNodePtr node, const char *root, struct dav_data *data)
{
    time_t now = time(NULL);

    if (data->lock_expire > now) {
        xmlNodePtr active, node1;
        char tbuf[30]; /* "Second-" + long int + NUL */

        active = xmlNewChild(node, NULL, BAD_CAST "activelock", NULL);
        node1 = xmlNewChild(active, NULL, BAD_CAST "lockscope", NULL);
        xmlNewChild(node1, NULL, BAD_CAST "exclusive", NULL);

        node1 = xmlNewChild(active, NULL, BAD_CAST "locktype", NULL);
        xmlNewChild(node1, NULL, BAD_CAST "write", NULL);

        xmlNewChild(active, NULL, BAD_CAST "depth", BAD_CAST "0");

        if (data->lock_owner) {
            /* Last char of token signals href (1) or text (0) */
            if (data->lock_token[strlen(data->lock_token)-1] == '1') {
                node1 = xmlNewChild(active, NULL, BAD_CAST "owner", NULL);
                xml_add_href(node1, NULL, data->lock_owner);
            }
            else {
                xmlNewTextChild(active, NULL, BAD_CAST "owner",
                                BAD_CAST data->lock_owner);
            }
        }

        snprintf(tbuf, sizeof(tbuf), "Second-%lu", data->lock_expire - now);
        xmlNewChild(active, NULL, BAD_CAST "timeout", BAD_CAST tbuf);

        node1 = xmlNewChild(active, NULL, BAD_CAST "locktoken", NULL);
        xml_add_href(node1, NULL, data->lock_token);

        node1 = xmlNewChild(active, NULL, BAD_CAST "lockroot", NULL);
        xml_add_href(node1, NULL, root);
    }
}


/* Add a property 'name', of namespace 'ns', with content 'content',
 * and status code/string 'status' to propstat element 'stat'.
 * 'stat' will be created as necessary.
 */
xmlNodePtr xml_add_prop(long status, xmlNsPtr davns,
                        struct propstat *propstat,
                        const xmlChar *name, xmlNsPtr ns,
                        xmlChar *content,
                        unsigned precond)
{
    xmlNodePtr newprop = NULL;

    if (!propstat->root) {
        propstat->root = xmlNewNode(davns, BAD_CAST "propstat");
        xmlNewChild(propstat->root, NULL, BAD_CAST "prop", NULL);
    }

    if (name) newprop = xmlNewTextChild(propstat->root->children,
                                        ns, name, content);
    propstat->status = status;
    propstat->precond = precond;

    return newprop;
}


struct allprop_rock {
    struct propfind_ctx *fctx;
    struct propstat *propstat;
};

/* Add a response tree to 'root' for the specified href and
   either error code or property list */
int xml_add_response(struct propfind_ctx *fctx, long code, unsigned precond)
{
    xmlNodePtr resp;

    resp = xmlNewChild(fctx->root, fctx->ns[NS_DAV], BAD_CAST "response", NULL);
    if (!resp) {
        fctx->err->desc = "Unable to add response XML element";
        *fctx->ret = HTTP_SERVER_ERROR;
        return HTTP_SERVER_ERROR;
    }
    xml_add_href(resp, NULL, fctx->req_tgt->path);

    if (code) {
        xmlNewChild(resp, NULL, BAD_CAST "status",
                    BAD_CAST http_statusline(code));

        if (precond) {
            xmlNodePtr error = xmlNewChild(resp, NULL, BAD_CAST "error", NULL);

            xmlNewChild(error, NULL, BAD_CAST preconds[precond].name, NULL);
        }
    }
    else {
        struct propstat propstat[NUM_PROPSTAT], *stat;
        struct propfind_entry_list *e;
        int i;

        memset(propstat, 0, NUM_PROPSTAT * sizeof(struct propstat));

        /* Process each property in the linked list */
        for (e = fctx->elist; e; e = e->next) {
            int r = HTTP_NOT_FOUND;

            if (e->get) {
                r = 0;

                /* Pre-screen request based on prop flags */
                if (fctx->req_tgt->resource) {
                    if (!(e->flags & PROP_RESOURCE)) r = HTTP_NOT_FOUND;
                }
                else if (!(e->flags & PROP_COLLECTION)) r = HTTP_NOT_FOUND;

                if (!r) {
                    if (fctx->mode == PROPFIND_NAME) {
                        xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                     &propstat[PROPSTAT_OK],
                                     e->name, e->ns, NULL, 0);
                    }
                    else {
                        r = e->get(e->name, e->ns, fctx,
                                   e->prop, resp, propstat, e->rock);
                    }
                }
            }

            switch (r) {
            case 0:
            case HTTP_OK:
                /* Nothing to do - property handled in callback */
                break;

            case HTTP_UNAUTHORIZED:
                xml_add_prop(HTTP_UNAUTHORIZED, fctx->ns[NS_DAV],
                             &propstat[PROPSTAT_UNAUTH],
                             e->name, e->ns, NULL, 0);
                break;

            case HTTP_FORBIDDEN:
                xml_add_prop(HTTP_FORBIDDEN, fctx->ns[NS_DAV],
                             &propstat[PROPSTAT_FORBID],
                             e->name, e->ns, NULL, 0);
                break;

            case HTTP_NOT_FOUND:
                if (!(fctx->prefer & PREFER_MIN)) {
                    xml_add_prop(HTTP_NOT_FOUND, fctx->ns[NS_DAV],
                                 &propstat[PROPSTAT_NOTFOUND],
                                 e->name, e->ns, NULL, 0);
                }
                break;

            default:
                xml_add_prop(r, fctx->ns[NS_DAV], &propstat[PROPSTAT_ERROR],
                             e->name, e->ns, NULL, 0);
                break;

            }
        }

        /* Process dead properties for allprop/propname */
        if (fctx->mailbox && !fctx->req_tgt->resource &&
            (fctx->mode == PROPFIND_ALL || fctx->mode == PROPFIND_NAME)) {
            struct allprop_rock arock = { fctx, propstat };

            annotatemore_findall(fctx->mailbox->name, 0, "*", allprop_cb, &arock);
        }

        /* Check if we have any propstat elements */
        for (i = 0; i < NUM_PROPSTAT && !propstat[i].root; i++);
        if (i == NUM_PROPSTAT) {
            /* Add an empty propstat 200 */
            xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                         &propstat[PROPSTAT_OK], NULL, NULL, NULL, 0);
        }

        /* Add status and optional error to the propstat elements
           and then add them to response element */
        for (i = 0; i < NUM_PROPSTAT; i++) {
            stat = &propstat[i];

            if (stat->root) {
                xmlNewChild(stat->root, NULL, BAD_CAST "status",
                            BAD_CAST http_statusline(stat->status));
                if (stat->precond) {
                    struct error_t error = { NULL, stat->precond, NULL, 0 };
                    xml_add_error(stat->root, &error, fctx->ns);
                }

                xmlAddChild(resp, stat->root);
            }
        }
    }

    fctx->record = NULL;

    return 0;
}


/* Helper function to prescreen/fetch resource data */
int propfind_getdata(const xmlChar *name, xmlNsPtr ns,
                     struct propfind_ctx *fctx,
                     xmlNodePtr prop, struct propstat propstat[],
                     struct mime_type_t *mime_types, int precond,
                     const char *data, unsigned long datalen)
{
    int ret = 0;
    xmlChar *type, *ver = NULL;
    struct mime_type_t *mime;

    type = xmlGetProp(prop, BAD_CAST "content-type");
    if (type) ver = xmlGetProp(prop, BAD_CAST "version");

    /* Check/find requested MIME type */
    for (mime = mime_types; type && mime->content_type; mime++) {
        if (is_mediatype((const char *) type, mime->content_type)) {
            if (ver &&
                (!mime->version || xmlStrcmp(ver, BAD_CAST mime->version))) {
                continue;
            }
            break;
        }
    }

    if (!propstat) {
        /* Prescreen "property" request */
        if (!mime->content_type) {
            fctx->err->precond = precond;
            ret = *fctx->ret = HTTP_FORBIDDEN;
        }
    }
    else {
        /* Add "property" */
        char *freeme = NULL;

        prop = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                            &propstat[PROPSTAT_OK], name, ns, NULL, 0);

        if (mime != mime_types) {
            /* Not the storage format - convert into requested MIME type */
            void *obj = mime_types->from_string(data);

            data = freeme = mime->to_string(obj);
            datalen = strlen(data);
            mime_types->free(obj);
        }

        if (type) {
            xmlSetProp(prop, BAD_CAST "content-type", type);
            if (ver) xmlSetProp(prop, BAD_CAST "version", ver);
        }

        xmlAddChild(prop,
                    xmlNewCDataBlock(fctx->root->doc, BAD_CAST data, datalen));

        fctx->fetcheddata = 1;

        if (freeme) free(freeme);
    }

    if (type) xmlFree(type);
    if (ver) xmlFree(ver);

    return ret;
}


/* Callback to fetch DAV:creationdate */
int propfind_creationdate(const xmlChar *name, xmlNsPtr ns,
                          struct propfind_ctx *fctx,
                          xmlNodePtr prop __attribute__((unused)),
                          xmlNodePtr resp __attribute__((unused)),
                          struct propstat propstat[],
                          void *rock __attribute__((unused)))
{
    time_t t = 0;
    char datestr[RFC3339_DATETIME_MAX];

    if (fctx->data) {
        struct dav_data *ddata = (struct dav_data *) fctx->data;

        t = ddata->creationdate;
    }
    else if (fctx->mailbox) {
        struct stat sbuf;

        fstat(fctx->mailbox->header_fd, &sbuf);

        t = sbuf.st_ctime;
    }

    if (!t) return HTTP_NOT_FOUND;

    time_to_rfc3339(t, datestr, RFC3339_DATETIME_MAX);

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST datestr, 0);

    return 0;
}


/* Callback to fetch DAV:displayname */
static int propfind_displayname(const xmlChar *name, xmlNsPtr ns,
                                struct propfind_ctx *fctx,
                                xmlNodePtr prop __attribute__((unused)),
                                xmlNodePtr resp __attribute__((unused)),
                                struct propstat propstat[],
                                void *rock __attribute__((unused)))
{
    /* XXX  Do LDAP/SQL lookup here */

    buf_reset(&fctx->buf);

    if (fctx->req_tgt->userid)
        buf_printf(&fctx->buf, "%s", fctx->req_tgt->userid);

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST buf_cstring(&fctx->buf), 0);

    return 0;
}


/* Callback to fetch DAV:getcontentlength */
int propfind_getlength(const xmlChar *name, xmlNsPtr ns,
                       struct propfind_ctx *fctx,
                       xmlNodePtr prop __attribute__((unused)),
                       xmlNodePtr resp __attribute__((unused)),
                       struct propstat propstat[],
                       void *rock __attribute__((unused)))
{
    buf_reset(&fctx->buf);

    if (fctx->record) {
        buf_printf(&fctx->buf, "%u",
                   fctx->record->size - fctx->record->header_size);
    }

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST buf_cstring(&fctx->buf), 0);

    return 0;
}


/* Callback to fetch DAV:getetag */
int propfind_getetag(const xmlChar *name, xmlNsPtr ns,
                     struct propfind_ctx *fctx,
                     xmlNodePtr prop __attribute__((unused)),
                     xmlNodePtr resp __attribute__((unused)),
                     struct propstat propstat[],
                     void *rock __attribute__((unused)))
{
    if (fctx->req_tgt->resource && !fctx->record) return HTTP_NOT_FOUND;
    if (!fctx->mailbox) return HTTP_NOT_FOUND;

    buf_reset(&fctx->buf);

    if (fctx->record) {
        /* add DQUOTEs */
        buf_printf(&fctx->buf, "\"%s\"",
                   message_guid_encode(&fctx->record->guid));
    }
    else {
        buf_printf(&fctx->buf, "\"%u-%u-%u\"", fctx->mailbox->i.uidvalidity,
                   fctx->mailbox->i.last_uid, fctx->mailbox->i.exists);
    }

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST buf_cstring(&fctx->buf), 0);

    return 0;
}


/* Callback to fetch DAV:getlastmodified */
int propfind_getlastmod(const xmlChar *name, xmlNsPtr ns,
                        struct propfind_ctx *fctx,
                        xmlNodePtr prop __attribute__((unused)),
                        xmlNodePtr resp __attribute__((unused)),
                        struct propstat propstat[],
                        void *rock __attribute__((unused)))
{
    if (!fctx->mailbox ||
        (fctx->req_tgt->resource && !fctx->record)) return HTTP_NOT_FOUND;

    buf_ensure(&fctx->buf, 30);
    httpdate_gen(fctx->buf.s, fctx->buf.alloc,
                 fctx->record ? fctx->record->internaldate :
                 fctx->mailbox->index_mtime);

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST fctx->buf.s, 0);

    return 0;
}


/* Callback to fetch DAV:lockdiscovery */
int propfind_lockdisc(const xmlChar *name, xmlNsPtr ns,
                      struct propfind_ctx *fctx,
                      xmlNodePtr prop __attribute__((unused)),
                      xmlNodePtr resp __attribute__((unused)),
                      struct propstat propstat[],
                      void *rock __attribute__((unused)))
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    if (fctx->mailbox && fctx->record) {
        struct dav_data *ddata = (struct dav_data *) fctx->data;

        xml_add_lockdisc(node, fctx->req_tgt->path, ddata);
    }

    return 0;
}


/* Callback to fetch DAV:resourcetype */
static int propfind_restype(const xmlChar *name, xmlNsPtr ns,
                            struct propfind_ctx *fctx,
                            xmlNodePtr prop __attribute__((unused)),
                            xmlNodePtr resp __attribute__((unused)),
                            struct propstat propstat[],
                            void *rock __attribute__((unused)))
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    if (fctx->req_tgt->userid)
        xmlNewChild(node, NULL, BAD_CAST "principal", NULL);

    return 0;
}


/* Callback to "write" resourcetype property */
int proppatch_restype(xmlNodePtr prop, unsigned set,
                      struct proppatch_ctx *pctx,
                      struct propstat propstat[],
                      void *rock)
{
    const char *coltype = (const char *) rock;
    unsigned precond = 0;

    if (set && (pctx->meth != METH_PROPPATCH)) {
        /* "Writeable" for MKCOL/MKCALENDAR only */
        xmlNodePtr cur;

        for (cur = prop->children; cur; cur = cur->next) {
            if (cur->type != XML_ELEMENT_NODE) continue;
            /* Make sure we have valid resourcetypes for the collection */
            if (xmlStrcmp(cur->name, BAD_CAST "collection") &&
                (!coltype || xmlStrcmp(cur->name, BAD_CAST coltype))) break;
        }

        if (!cur) {
            /* All resourcetypes are valid */
            xml_add_prop(HTTP_OK, pctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                         prop->name, prop->ns, NULL, 0);

            return 0;
        }

        /* Invalid resourcetype */
        precond = DAV_VALID_RESTYPE;
    }
    else {
        /* Protected property */
        precond = DAV_PROT_PROP;
    }

    xml_add_prop(HTTP_FORBIDDEN, pctx->ns[NS_DAV], &propstat[PROPSTAT_FORBID],
                 prop->name, prop->ns, NULL, precond);

    *pctx->ret = HTTP_FORBIDDEN;

    return 0;
}


/* Callback to fetch DAV:supportedlock */
int propfind_suplock(const xmlChar *name, xmlNsPtr ns,
                     struct propfind_ctx *fctx,
                     xmlNodePtr prop __attribute__((unused)),
                     xmlNodePtr resp __attribute__((unused)),
                     struct propstat propstat[],
                     void *rock __attribute__((unused)))
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    if (fctx->mailbox && fctx->record) {
        xmlNodePtr entry = xmlNewChild(node, NULL, BAD_CAST "lockentry", NULL);
        xmlNodePtr scope = xmlNewChild(entry, NULL, BAD_CAST "lockscope", NULL);
        xmlNodePtr type = xmlNewChild(entry, NULL, BAD_CAST "locktype", NULL);

        xmlNewChild(scope, NULL, BAD_CAST "exclusive", NULL);
        xmlNewChild(type, NULL, BAD_CAST "write", NULL);
    }

    return 0;
}


/* Callback to fetch DAV:supported-report-set */
int propfind_reportset(const xmlChar *name, xmlNsPtr ns,
                       struct propfind_ctx *fctx,
                       xmlNodePtr prop __attribute__((unused)),
                       xmlNodePtr resp __attribute__((unused)),
                       struct propstat propstat[],
                       void *rock)
{
    xmlNodePtr top, node;
    const struct report_type_t *report;

    top = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                       name, ns, NULL, 0);

    for (report = (const struct report_type_t *) rock;
         report && report->name; report++) {
        node = xmlNewChild(top, NULL, BAD_CAST "supported-report", NULL);
        node = xmlNewChild(node, NULL, BAD_CAST "report", NULL);

        ensure_ns(fctx->ns, report->ns, resp->parent,
                  known_namespaces[report->ns].href,
                  known_namespaces[report->ns].prefix);
        xmlNewChild(node, fctx->ns[report->ns], BAD_CAST report->name, NULL);
    }

    return 0;
}


/* Callback to fetch DAV:alternate-URI-set */
static int propfind_alturiset(const xmlChar *name, xmlNsPtr ns,
                              struct propfind_ctx *fctx,
                              xmlNodePtr prop __attribute__((unused)),
                              xmlNodePtr resp __attribute__((unused)),
                              struct propstat propstat[],
                              void *rock __attribute__((unused)))
{
    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                 &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    return 0;
}


/* Callback to fetch DAV:owner, DAV:principal-URL */
int propfind_owner(const xmlChar *name, xmlNsPtr ns,
                          struct propfind_ctx *fctx,
                          xmlNodePtr prop,
                          xmlNodePtr resp __attribute__((unused)),
                          struct propstat propstat[],
                          void *rock)
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);
    const char *userid = rock ? (const char *) rock : fctx->req_tgt->userid;

    if (userid) {
        const char *domain =
            httpd_extradomain ? httpd_extradomain : config_defdomain;

        buf_reset(&fctx->buf);

        if (strchr(userid, '@') || !domain) {
            buf_printf(&fctx->buf, "%s/user/%s/",
                       namespace_principal.prefix, userid);
        }
        else {
            buf_printf(&fctx->buf, "%s/user/%s@%s/",
                       namespace_principal.prefix, userid, domain);
        }

        if ((fctx->mode == PROPFIND_EXPAND) && xmlFirstElementChild(prop)) {
            /* Return properties for this URL */
            expand_property(prop, fctx, buf_cstring(&fctx->buf),
                            &principal_parse_path, principal_props, node, 0);
        }
        else {
            /* Return just the URL */
            xml_add_href(node, NULL, buf_cstring(&fctx->buf));
        }
    }

    return 0;
}


/* Add possibly 'abstract' supported-privilege 'priv_name', of namespace 'ns',
 * with description 'desc_str' to node 'root'.  For now, we alssume all
 * descriptions are English.
 */
static xmlNodePtr add_suppriv(xmlNodePtr root, const char *priv_name,
                              xmlNsPtr ns, int abstract, const char *desc_str)
{
    xmlNodePtr supp, priv, desc;

    supp = xmlNewChild(root, NULL, BAD_CAST "supported-privilege", NULL);
    priv = xmlNewChild(supp, NULL, BAD_CAST "privilege", NULL);
    xmlNewChild(priv, ns, BAD_CAST priv_name, NULL);
    if (abstract) xmlNewChild(supp, NULL, BAD_CAST "abstract", NULL);
    desc = xmlNewChild(supp, NULL, BAD_CAST "description", BAD_CAST desc_str);
    xmlNodeSetLang(desc, BAD_CAST "en");

    return supp;
}


/* Callback to fetch DAV:supported-privilege-set */
int propfind_supprivset(const xmlChar *name, xmlNsPtr ns,
                        struct propfind_ctx *fctx,
                        xmlNodePtr prop __attribute__((unused)),
                        xmlNodePtr resp,
                        struct propstat propstat[],
                        void *rock __attribute__((unused)))
{
    xmlNodePtr set, all, agg, write;
    unsigned tgt_flags = 0;

    set = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                       name, ns, NULL, 0);

    all = add_suppriv(set, "all", NULL, 0, "Any operation");

    agg = add_suppriv(all, "read", NULL, 0, "Read any object");
    add_suppriv(agg, "read-current-user-privilege-set", NULL, 1,
                "Read current user privilege set");

    if (fctx->req_tgt->namespace == URL_NS_CALENDAR) {
        if (fctx->req_tgt->collection) {
            ensure_ns(fctx->ns, NS_CALDAV, resp->parent, XML_NS_CALDAV, "C");

            if (!strcmp(fctx->req_tgt->collection, SCHED_INBOX))
                tgt_flags = TGT_SCHED_INBOX;
            else if (!strcmp(fctx->req_tgt->collection, SCHED_OUTBOX))
                tgt_flags = TGT_SCHED_OUTBOX;
            else {
                add_suppriv(agg, "read-free-busy", fctx->ns[NS_CALDAV], 0,
                            "Read free/busy time");
            }
        }
    }

    write = add_suppriv(all, "write", NULL, 0, "Write any object");
    add_suppriv(write, "write-content", NULL, 0, "Write resource content");
    add_suppriv(write, "write-properties", NULL, 0, "Write properties");

    agg = add_suppriv(write, "bind", NULL, 0, "Add new member to collection");
    ensure_ns(fctx->ns, NS_CYRUS, resp->parent, XML_NS_CYRUS, "CY");
    add_suppriv(agg, "make-collection", fctx->ns[NS_CYRUS], 0,
                "Make new collection");
    add_suppriv(agg, "add-resource", fctx->ns[NS_CYRUS], 0,
                "Add new resource");

    agg = add_suppriv(write, "unbind", NULL, 0,
                         "Remove member from collection");
    add_suppriv(agg, "remove-collection", fctx->ns[NS_CYRUS], 0,
                "Remove collection");
    add_suppriv(agg, "remove-resource", fctx->ns[NS_CYRUS], 0,
                "Remove resource");

    agg = add_suppriv(all, "admin", fctx->ns[NS_CYRUS], 0,
                        "Perform administrative operations");
    add_suppriv(agg, "read-acl", NULL, 1, "Read ACL");
    add_suppriv(agg, "write-acl", NULL, 1, "Write ACL");
    add_suppriv(agg, "unlock", NULL, 1, "Unlock resource");

    if (tgt_flags == TGT_SCHED_INBOX) {
        agg = add_suppriv(all, "schedule-deliver", fctx->ns[NS_CALDAV], 0,
                          "Deliver scheduling messages");
        add_suppriv(agg, "schedule-deliver-invite", fctx->ns[NS_CALDAV], 0,
                    "Deliver scheduling messages from Organizers");
        add_suppriv(agg, "schedule-deliver-reply", fctx->ns[NS_CALDAV], 0,
                    "Deliver scheduling messages from Attendees");
        add_suppriv(agg, "schedule-query-freebusy", fctx->ns[NS_CALDAV], 0,
                    "Accept free/busy requests");
    }
    else if (tgt_flags == TGT_SCHED_OUTBOX) {
        agg = add_suppriv(all, "schedule-send", fctx->ns[NS_CALDAV], 0,
                          "Send scheduling messages");
        add_suppriv(agg, "schedule-send-invite", fctx->ns[NS_CALDAV], 0,
                    "Send scheduling messages by Organizers");
        add_suppriv(agg, "schedule-send-reply", fctx->ns[NS_CALDAV], 0,
                    "Send scheduling messages by Attendees");
        add_suppriv(agg, "schedule-send-freebusy", fctx->ns[NS_CALDAV], 0,
                    "Submit free/busy requests");
    }

    return 0;
}


static int add_privs(int rights, unsigned flags,
                     xmlNodePtr parent, xmlNodePtr root, xmlNsPtr *ns)
{
    xmlNodePtr priv;

    if ((rights & DACL_ALL) == DACL_ALL &&
        /* DAV:all on CALDAV:schedule-in/outbox MUST include CALDAV:schedule */
        (!(flags & (PRIV_INBOX|PRIV_OUTBOX)) ||
         (rights & DACL_SCHED) == DACL_SCHED)) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "all", NULL);
    }
    if ((rights & DACL_READ) == DACL_READ) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "read", NULL);
        if (flags & PRIV_IMPLICIT) rights |= DACL_READFB;
    }
    if ((rights & DACL_READFB) &&
        /* CALDAV:read-free-busy does not apply to CALDAV:schedule-in/outbox */
        !(flags & (PRIV_INBOX|PRIV_OUTBOX))) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        ensure_ns(ns, NS_CALDAV, root, XML_NS_CALDAV, "C");
        xmlNewChild(priv, ns[NS_CALDAV], BAD_CAST  "read-free-busy", NULL);
    }
    if ((rights & DACL_WRITE) == DACL_WRITE) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "write", NULL);
    }
    if (rights & DACL_WRITECONT) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "write-content", NULL);
    }
    if (rights & DACL_WRITEPROPS) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "write-properties", NULL);
    }

    if (rights & (DACL_BIND|DACL_UNBIND|DACL_ADMIN)) {
        ensure_ns(ns, NS_CYRUS, root, XML_NS_CYRUS, "CY");
    }

    if ((rights & DACL_BIND) == DACL_BIND) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "bind", NULL);
    }
    if (rights & DACL_MKCOL) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, ns[NS_CYRUS], BAD_CAST "make-collection", NULL);
    }
    if (rights & DACL_ADDRSRC) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, ns[NS_CYRUS], BAD_CAST "add-resource", NULL);
    }
    if ((rights & DACL_UNBIND) == DACL_UNBIND) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, NULL, BAD_CAST "unbind", NULL);
    }
    if (rights & DACL_RMCOL) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, ns[NS_CYRUS], BAD_CAST "remove-collection", NULL);
    }
    if ((rights & DACL_RMRSRC) == DACL_RMRSRC) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, ns[NS_CYRUS], BAD_CAST "remove-resource", NULL);
    }
    if (rights & DACL_ADMIN) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        xmlNewChild(priv, ns[NS_CYRUS], BAD_CAST  "admin", NULL);
    }

    if (rights & DACL_SCHED) {
        ensure_ns(ns, NS_CALDAV, root, XML_NS_CALDAV, "C");
    }
    if ((rights & DACL_SCHED) == DACL_SCHED) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        if (flags & PRIV_INBOX)
            xmlNewChild(priv, ns[NS_CALDAV], BAD_CAST "schedule-deliver", NULL);
        else if (flags & PRIV_OUTBOX)
            xmlNewChild(priv, ns[NS_CALDAV], BAD_CAST "schedule-send", NULL);
    }
    if (rights & DACL_INVITE) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        if (flags & PRIV_INBOX)
            xmlNewChild(priv, ns[NS_CALDAV],
                        BAD_CAST "schedule-deliver-invite", NULL);
        else if (flags & PRIV_OUTBOX)
            xmlNewChild(priv, ns[NS_CALDAV],
                        BAD_CAST "schedule-send-invite", NULL);
    }
    if (rights & DACL_REPLY) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        if (flags & PRIV_INBOX)
            xmlNewChild(priv, ns[NS_CALDAV],
                        BAD_CAST "schedule-deliver-reply", NULL);
        else if (flags & PRIV_OUTBOX)
            xmlNewChild(priv, ns[NS_CALDAV],
                        BAD_CAST "schedule-send-reply", NULL);
    }
    if (rights & DACL_SCHEDFB) {
        priv = xmlNewChild(parent, NULL, BAD_CAST "privilege", NULL);
        if (flags & PRIV_INBOX)
            xmlNewChild(priv, ns[NS_CALDAV],
                        BAD_CAST "schedule-query-freebusy", NULL);
        else if (flags & PRIV_OUTBOX)
            xmlNewChild(priv, ns[NS_CALDAV],
                        BAD_CAST "schedule-send-freebusy", NULL);
    }

    return 0;
}


/* Callback to fetch DAV:current-user-privilege-set */
int propfind_curprivset(const xmlChar *name, xmlNsPtr ns,
                        struct propfind_ctx *fctx,
                        xmlNodePtr prop __attribute__((unused)),
                        xmlNodePtr resp,
                        struct propstat propstat[],
                        void *rock __attribute__((unused)))
{
    int rights;
    unsigned flags = 0;
    xmlNodePtr set;

    if (!fctx->mailbox) return HTTP_NOT_FOUND;
    rights = httpd_myrights(fctx->authstate, fctx->mailbox->acl);
    if ((rights & DACL_READ) != DACL_READ) {
        return HTTP_UNAUTHORIZED;
    }

    /* Add in implicit rights */
    if (fctx->userisadmin) {
        rights |= DACL_ADMIN;
    }
    else if (mboxname_userownsmailbox(httpd_userid, fctx->mailbox->name)) {
        rights |= config_implicitrights;
        /* we always allow admin by the owner in DAV */
        rights |= DACL_ADMIN;
    }

    /* Build the rest of the XML response */
    set = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                       name, ns, NULL, 0);

    if (!fctx->req_tgt->resource) {
        if (fctx->req_tgt->namespace == URL_NS_CALENDAR) {
            flags = PRIV_IMPLICIT;

            if (fctx->req_tgt->collection) {
                if (!strcmp(fctx->req_tgt->collection, SCHED_INBOX))
                    flags = PRIV_INBOX;
                else if (!strcmp(fctx->req_tgt->collection, SCHED_OUTBOX))
                    flags = PRIV_OUTBOX;
            }
        }

        add_privs(rights, flags, set, resp->parent, fctx->ns);
    }

    return 0;
}


/* Callback to fetch DAV:acl */
int propfind_acl(const xmlChar *name, xmlNsPtr ns,
                 struct propfind_ctx *fctx,
                 xmlNodePtr prop __attribute__((unused)),
                 xmlNodePtr resp,
                 struct propstat propstat[],
                 void *rock __attribute__((unused)))
{
    xmlNodePtr acl;
    char *aclstr, *userid;
    unsigned flags = 0;

    if (!fctx->mailbox) return HTTP_NOT_FOUND;

    /* owner has implicit admin rights */
    if (!mboxname_userownsmailbox(httpd_userid, fctx->mailbox->name)) {
        int rights = httpd_myrights(fctx->authstate, fctx->mailbox->acl);
        if (!(rights & DACL_ADMIN))
            return HTTP_UNAUTHORIZED;
    }

    if (fctx->req_tgt->namespace == URL_NS_CALENDAR) {
        flags = PRIV_IMPLICIT;

        if (fctx->req_tgt->collection) {
            if (!strcmp(fctx->req_tgt->collection, SCHED_INBOX))
                flags = PRIV_INBOX;
            else if (!strcmp(fctx->req_tgt->collection, SCHED_OUTBOX))
                flags = PRIV_OUTBOX;
        }
    }

    /* Start the acl XML response */
    acl = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                       name, ns, NULL, 0);

    /* Parse the ACL string (userid/rights pairs) */
    userid = aclstr = xstrdup(fctx->mailbox->acl);

    while (userid) {
        int rights;
        char *rightstr, *nextid;
        xmlNodePtr ace, node;

        rightstr = strchr(userid, '\t');
        if (!rightstr) break;
        *rightstr++ = '\0';

        nextid = strchr(rightstr, '\t');
        if (!nextid) break;
        *nextid++ = '\0';

        /* Check for negative rights */
        /* XXX  Does this correspond to DAV:deny? */
        if (*userid == '-') {
            userid = nextid;
            continue;
        }

        rights = cyrus_acl_strtomask(rightstr);

        /* Add ace XML element for this userid/right pair */
        ace = xmlNewChild(acl, NULL, BAD_CAST "ace", NULL);

        node = xmlNewChild(ace, NULL, BAD_CAST "principal", NULL);
        if (!strcmp(userid, fctx->userid))
            xmlNewChild(node, NULL, BAD_CAST "self", NULL);
        else if (mboxname_userownsmailbox(userid, fctx->mailbox->name)) {
            xmlNewChild(node, NULL, BAD_CAST "owner", NULL);
            /* we always allow admin by the owner in DAV */
            rights |= DACL_ADMIN;
        }
        else if (!strcmp(userid, "anyone"))
            xmlNewChild(node, NULL, BAD_CAST "authenticated", NULL);
        /* XXX - well, it's better than a user called 'anonymous'
         * Is there any IMAP equivalent to "unauthenticated"?
         * Is there any DAV equivalent to "anonymous"?
         */
        else if (!strcmp(userid, "anonymous"))
            xmlNewChild(node, NULL, BAD_CAST "unauthenticated", NULL);
        else if (!strncmp(userid, "group:", 6)) {
            buf_reset(&fctx->buf);
            buf_printf(&fctx->buf, "%s/group/%s/",
                       namespace_principal.prefix, userid+6);
            xml_add_href(node, NULL, buf_cstring(&fctx->buf));
        }
        else {
            buf_reset(&fctx->buf);
            buf_printf(&fctx->buf, "%s/user/%s/",
                       namespace_principal.prefix, userid);
            xml_add_href(node, NULL, buf_cstring(&fctx->buf));
        }

        node = xmlNewChild(ace, NULL, BAD_CAST "grant", NULL);
        add_privs(rights, flags, node, resp->parent, fctx->ns);

        /* system userids are protected */
        if (is_system_user(userid))
            xmlNewChild(ace, NULL, BAD_CAST "protected", NULL);

        if (fctx->req_tgt->resource) {
            node = xmlNewChild(ace, NULL, BAD_CAST "inherited", NULL);
            buf_reset(&fctx->buf);
            buf_printf(&fctx->buf, "%.*s",
                       (int)(fctx->req_tgt->resource - fctx->req_tgt->path),
                       fctx->req_tgt->path);
            xml_add_href(node, NULL, buf_cstring(&fctx->buf));
        }

        userid = nextid;
    }

    if (aclstr) free(aclstr);

    return 0;
}


/* Callback to fetch DAV:acl-restrictions */
int propfind_aclrestrict(const xmlChar *name, xmlNsPtr ns,
                         struct propfind_ctx *fctx,
                         xmlNodePtr prop __attribute__((unused)),
                         xmlNodePtr resp __attribute__((unused)),
                         struct propstat propstat[],
                         void *rock __attribute__((unused)))
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    xmlNewChild(node, NULL, BAD_CAST "no-invert", NULL);

    return 0;
}


/* Callback to fetch DAV:principal-collection-set */
EXPORTED int propfind_princolset(const xmlChar *name, xmlNsPtr ns,
                                 struct propfind_ctx *fctx,
                                 xmlNodePtr prop __attribute__((unused)),
                                 xmlNodePtr resp __attribute__((unused)),
                                 struct propstat propstat[],
                                 void *rock __attribute__((unused)))
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    buf_reset(&fctx->buf);
    buf_printf(&fctx->buf, "%s/", namespace_principal.prefix);
    xml_add_href(node, NULL, buf_cstring(&fctx->buf));

    return 0;
}


/* Callback to fetch DAV:quota-available-bytes and DAV:quota-used-bytes */
EXPORTED int propfind_quota(const xmlChar *name, xmlNsPtr ns,
                            struct propfind_ctx *fctx,
                            xmlNodePtr prop __attribute__((unused)),
                            xmlNodePtr resp __attribute__((unused)),
                            struct propstat propstat[],
                            void *rock __attribute__((unused)))
{
    static char prevroot[MAX_MAILBOX_BUFFER];
    char foundroot[MAX_MAILBOX_BUFFER], *qr = NULL;

    if (fctx->mailbox) {
        /* Use the quotaroot as specified in mailbox header */
        qr = fctx->mailbox->quotaroot;
    }
    else {
        /* Find the quotaroot governing this hierarchy */
        if (quota_findroot(foundroot, sizeof(foundroot),
                           fctx->req_tgt->mbentry->name)) {
            qr = foundroot;
        }
    }

    if (!qr) return HTTP_NOT_FOUND;

    if (!fctx->quota.root ||
        strcmp(fctx->quota.root, qr)) {
        /* Different quotaroot - read it */

        syslog(LOG_DEBUG, "reading quota for '%s'", qr);

        fctx->quota.root = strcpy(prevroot, qr);

        quota_read(&fctx->quota, NULL, 0);
    }

    buf_reset(&fctx->buf);
    if (!xmlStrcmp(name, BAD_CAST "quota-available-bytes")) {
        /* Calculate limit in bytes and subtract usage */
        quota_t limit =
            fctx->quota.limits[QUOTA_STORAGE] * quota_units[QUOTA_STORAGE];

        buf_printf(&fctx->buf, QUOTA_T_FMT,
                   limit - fctx->quota.useds[QUOTA_STORAGE]);
    }
    else if (fctx->record) {
        /* Bytes used by resource */
        buf_printf(&fctx->buf, "%u", fctx->record->size);
    }
    else if (fctx->mailbox) {
        /* Bytes used by calendar collection */
        buf_printf(&fctx->buf, QUOTA_T_FMT,
                   fctx->mailbox->i.quota_mailbox_used);
    }
    else {
        /* Bytes used by entire hierarchy */
        buf_printf(&fctx->buf, QUOTA_T_FMT, fctx->quota.useds[QUOTA_STORAGE]);
    }

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST buf_cstring(&fctx->buf), 0);

    return 0;
}


/* Callback to fetch DAV:current-user-principal */
EXPORTED int propfind_curprin(const xmlChar *name, xmlNsPtr ns,
                              struct propfind_ctx *fctx,
                              xmlNodePtr prop,
                              xmlNodePtr resp,
                              struct propstat propstat[],
                              void *rock __attribute__((unused)))
{
    if (httpd_userid) {
        propfind_principalurl(name, ns, fctx,
                              prop, resp, propstat, httpd_userid);
    }
    else {
        xmlNodePtr node =
            xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                         &propstat[PROPSTAT_OK], name, ns, NULL, 0);

        xmlNewChild(node, NULL, BAD_CAST "unauthenticated", NULL);
    }

    return 0;
}


/* Callback to fetch DAV:server-info-href */
EXPORTED int propfind_serverinfo(const xmlChar *name, xmlNsPtr ns,
                                 struct propfind_ctx *fctx,
                                 xmlNodePtr prop __attribute__((unused)),
                                 xmlNodePtr resp __attribute__((unused)),
                                 struct propstat propstat[],
                                 void *rock __attribute__((unused)))
{
    xmlNodePtr node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV],
                                   &propstat[PROPSTAT_OK], name, ns, NULL, 0);

    buf_reset(&fctx->buf);
    buf_printf(&fctx->buf, "%s/%s", namespace_principal.prefix, SERVER_INFO);
    xml_add_href(node, NULL, buf_cstring(&fctx->buf));

    return 0;
}


/* Callback to fetch DAV:add-member */
int propfind_addmember(const xmlChar *name, xmlNsPtr ns,
                       struct propfind_ctx *fctx,
                       xmlNodePtr prop __attribute__((unused)),
                       xmlNodePtr resp __attribute__((unused)),
                       struct propstat propstat[],
                       void *rock __attribute__((unused)))
{
    xmlNodePtr node;
    int len;

    if (!fctx->req_tgt->collection ||
        !strcmp(fctx->req_tgt->collection, SCHED_INBOX) ||
        !strcmp(fctx->req_tgt->collection, SCHED_OUTBOX)) {
        /* Only allowed on non-scheduling collections */
        return HTTP_NOT_FOUND;
    }

    node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                        name, ns, NULL, 0);

    len = fctx->req_tgt->resource ?
        (size_t) (fctx->req_tgt->resource - fctx->req_tgt->path) :
        strlen(fctx->req_tgt->path);
    buf_reset(&fctx->buf);
    buf_printf(&fctx->buf, "%.*s?action=add-member", len, fctx->req_tgt->path);

    xml_add_href(node, NULL, buf_cstring(&fctx->buf));

    return 0;
}


static int get_synctoken(struct mailbox *mailbox, struct buf *buf)
{
    buf_reset(buf);
    buf_printf(buf, SYNC_TOKEN_URL_SCHEME "%u-" MODSEQ_FMT,
               mailbox->i.uidvalidity,
               mailbox->i.highestmodseq);

    return 0;
}

/* Callback to fetch DAV:sync-token and CS:getctag */
int propfind_sync_token(const xmlChar *name, xmlNsPtr ns,
                        struct propfind_ctx *fctx,
                        xmlNodePtr prop __attribute__((unused)),
                        xmlNodePtr resp __attribute__((unused)),
                        struct propstat propstat[],
                        void *rock __attribute__((unused)))
{
    if (!fctx->req_tgt->collection || /* until we support sync on cal-home */
        !fctx->mailbox || fctx->record) return HTTP_NOT_FOUND;

    /* not defined on the top-level collection either (aka #calendars) */
    if (!fctx->req_tgt->collection) return HTTP_NOT_FOUND;

    get_synctoken(fctx->mailbox, &fctx->buf);

    xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                 name, ns, BAD_CAST buf_cstring(&fctx->buf), 0);

    return 0;
}


/* Callback to fetch properties from resource header */
int propfind_fromhdr(const xmlChar *name, xmlNsPtr ns,
                     struct propfind_ctx *fctx,
                     xmlNodePtr prop __attribute__((unused)),
                     xmlNodePtr resp __attribute__((unused)),
                     struct propstat propstat[],
                     void *rock)
{
    const char *hdrname = (const char *) rock;
    int r = HTTP_NOT_FOUND;

    if (fctx->record &&
        (mailbox_cached_header(hdrname) != BIT32_MAX) &&
        !mailbox_cacherecord(fctx->mailbox, fctx->record)) {
        unsigned size;
        struct protstream *stream;
        hdrcache_t hdrs = NULL;
        const char **hdr;

        size = cacheitem_size(fctx->record, CACHE_HEADERS);
        stream = prot_readmap(cacheitem_base(fctx->record,
                                             CACHE_HEADERS), size);
        hdrs = spool_new_hdrcache();
        spool_fill_hdrcache(stream, NULL, hdrs, NULL);
        prot_free(stream);

        if ((hdr = spool_getheader(hdrs, (const char *) hdrname))) {
            xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                         name, ns, BAD_CAST hdr[0], 0);
            r = 0;
        }

        spool_free_hdrcache(hdrs);
    }

    return r;
}

static struct flaggedresources {
    const char *name;
    int flag;
} fres[] = {
    { "answered", FLAG_ANSWERED },
    { "flagged", FLAG_FLAGGED },
    { "seen", FLAG_SEEN },
    { NULL, 0 } /* last is always NULL */
};

/* Callback to write a property to annotation DB */
static int proppatch_toresource(xmlNodePtr prop, unsigned set,
                         struct proppatch_ctx *pctx,
                         struct propstat propstat[],
                         void *rock __attribute__((unused)))
{
    xmlChar *freeme = NULL;
    annotate_state_t *astate = NULL;
    struct buf value = BUF_INITIALIZER;
    int r = 1; /* default to error */

    /* flags only store "exists" */

    if (!strcmp((const char *)prop->ns->href, XML_NS_SYSFLAG)) {
        struct flaggedresources *frp;
        int isset;
        for (frp = fres; frp->name; frp++) {
            if (strcasecmp((const char *)prop->name, frp->name)) continue;
            r = 0; /* ok to do nothing */
            isset = pctx->record->system_flags & frp->flag;
            if (set) {
                if (isset) goto done;
                pctx->record->system_flags |= frp->flag;
            }
            else {
                if (!isset) goto done;
                pctx->record->system_flags &= ~frp->flag;
            }
            r = mailbox_rewrite_index_record(pctx->mailbox, pctx->record);
            goto done;
        }
        goto done;
    }

    if (!strcmp((const char *)prop->ns->href, XML_NS_USERFLAG)) {
        int userflag = 0;
        int isset;
        r = mailbox_user_flag(pctx->mailbox, (const char *)prop->name, &userflag, 1);
        if (r) goto done;
        isset = pctx->record->user_flags[userflag/32] & (1<<userflag%31);
        if (set) {
            if (isset) goto done;
            pctx->record->user_flags[userflag/32] |= (1<<userflag%31);
        }
        else {
            if (!isset) goto done;
            pctx->record->user_flags[userflag/32] &= ~(1<<userflag%31);
        }
        r = mailbox_rewrite_index_record(pctx->mailbox, pctx->record);
        goto done;
    }

    /* otherwise it's a database annotation */

    buf_reset(&pctx->buf);
    buf_printf(&pctx->buf, DAV_ANNOT_NS "<%s>%s",
               (const char *) prop->ns->href, prop->name);

    if (set) {
        freeme = xmlNodeGetContent(prop);
        buf_init_ro_cstr(&value, (const char *)freeme);
    }

    r = mailbox_get_annotate_state(pctx->mailbox, pctx->record->uid, &astate);
    if (!r) r = annotate_state_write(astate, buf_cstring(&pctx->buf), "", &value);
    /* we need to rewrite the record to update the modseq because the layering
     * of annotations and mailboxes is broken */
    if (!r) r = mailbox_rewrite_index_record(pctx->mailbox, pctx->record);

 done:

    if (!r) {
        xml_add_prop(HTTP_OK, pctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                     prop->name, prop->ns, NULL, 0);
    }
    else {
        xml_add_prop(HTTP_SERVER_ERROR, pctx->ns[NS_DAV],
                     &propstat[PROPSTAT_ERROR], prop->name, prop->ns, NULL, 0);
    }

    if (freeme) xmlFree(freeme);

    return 0;
}


/* Callback to read a property from annotation DB */
static int propfind_fromresource(const xmlChar *name, xmlNsPtr ns,
                                 struct propfind_ctx *fctx,
                                 xmlNodePtr prop __attribute__((unused)),
                                 xmlNodePtr resp __attribute__((unused)),
                                 struct propstat propstat[],
                                 void *rock __attribute__((unused)))
{
    struct buf attrib = BUF_INITIALIZER;
    xmlNodePtr node;
    int r = 0; /* default no error */

    if (!strcmp((const char *)ns->href, XML_NS_SYSFLAG)) {
        struct flaggedresources *frp;
        int isset;
        for (frp = fres; frp->name; frp++) {
            if (strcasecmp((const char *)name, frp->name)) continue;
            isset = fctx->record->system_flags & frp->flag;
            if (isset)
                buf_setcstr(&attrib, "1");
            goto done;
        }
        goto done;
    }

    if (!strcmp((const char *)ns->href, XML_NS_USERFLAG)) {
        int userflag = 0;
        int isset;
        r = mailbox_user_flag(fctx->mailbox, (const char *)name, &userflag, 0);
        if (r) goto done;
        isset = fctx->record->user_flags[userflag/32] & (1<<userflag%31);
        if (isset)
            buf_setcstr(&attrib, "1");
        goto done;
    }

    /* otherwise it's a DB annotation */

    buf_reset(&fctx->buf);
    buf_printf(&fctx->buf, DAV_ANNOT_NS "<%s>%s",
               (const char *) ns->href, name);

    r = annotatemore_msg_lookup(fctx->mailbox->name, fctx->record->uid,
                                buf_cstring(&fctx->buf), NULL, &attrib);

done:
    if (r) return HTTP_SERVER_ERROR;
    if (!buf_len(&attrib)) return HTTP_NOT_FOUND;

    node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                        name, ns, NULL, 0);
    xmlAddChild(node, xmlNewCDataBlock(fctx->root->doc,
                                       BAD_CAST buf_cstring(&attrib), buf_len(&attrib)));

    return 0;
}


/* Callback to read a property from annotation DB */
int propfind_fromdb(const xmlChar *name, xmlNsPtr ns,
                    struct propfind_ctx *fctx,
                    xmlNodePtr prop, xmlNodePtr resp,
                    struct propstat propstat[], void *rock)
{
    struct buf attrib = BUF_INITIALIZER;
    xmlNodePtr node;
    int r = 0;

    if (fctx->req_tgt->resource)
        return propfind_fromresource(name, ns, fctx, prop, resp, propstat, rock);

    buf_reset(&fctx->buf);
    buf_printf(&fctx->buf, DAV_ANNOT_NS "<%s>%s",
               (const char *) ns->href, name);

    if (fctx->mailbox && !fctx->record &&
        !(r = annotatemore_lookupmask(fctx->mailbox->name,
                                      buf_cstring(&fctx->buf),
                                      httpd_userid, &attrib))) {
        if (!buf_len(&attrib) &&
            !xmlStrcmp(name, BAD_CAST "displayname")) {
            /* Special case empty displayname -- use last segment of path */
            buf_setcstr(&attrib, strrchr(fctx->mailbox->name, '.') + 1);
        }
    }

    if (r) return HTTP_SERVER_ERROR;
    if (!buf_len(&attrib)) return HTTP_NOT_FOUND;

    node = xml_add_prop(HTTP_OK, fctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                        name, ns, NULL, 0);
    xmlAddChild(node, xmlNewCDataBlock(fctx->root->doc,
                                       BAD_CAST buf_cstring(&attrib),
                                       buf_len(&attrib)));

    buf_free(&attrib);

    return 0;
}

/* Callback to write a property to annotation DB */
int proppatch_todb(xmlNodePtr prop, unsigned set,
                   struct proppatch_ctx *pctx,
                   struct propstat propstat[],
                   void *rock)
{
    xmlChar *freeme = NULL;
    annotate_state_t *astate = NULL;
    struct buf value = BUF_INITIALIZER;
    int r;

    if (pctx->req_tgt->resource)
        return proppatch_toresource(prop, set, pctx, propstat, NULL);

    buf_reset(&pctx->buf);
    buf_printf(&pctx->buf, DAV_ANNOT_NS "<%s>%s",
               (const char *) prop->ns->href, prop->name);

    if (set) {
        if (rock) {
            buf_init_ro_cstr(&value, (const char *)rock);
        }
        else {
            freeme = xmlNodeGetContent(prop);
            buf_init_ro_cstr(&value, (const char *)freeme);
        }
    }

    r = mailbox_get_annotate_state(pctx->mailbox, 0, &astate);
    if (!r) r = annotate_state_writemask(astate, buf_cstring(&pctx->buf),
                                         httpd_userid, &value);

    if (!r) {
        xml_add_prop(HTTP_OK, pctx->ns[NS_DAV], &propstat[PROPSTAT_OK],
                     prop->name, prop->ns, NULL, 0);
    }
    else {
        xml_add_prop(HTTP_SERVER_ERROR, pctx->ns[NS_DAV],
                     &propstat[PROPSTAT_ERROR], prop->name, prop->ns, NULL, 0);
    }

    if (freeme) xmlFree(freeme);

    return 0;
}


/* annotemore_findall callback for adding dead properties (allprop/propname) */
static int allprop_cb(const char *mailbox __attribute__((unused)),
                      uint32_t uid __attribute__((unused)),
                      const char *entry,
                      const char *userid, const struct buf *attrib,
                      void *rock)
{
    struct allprop_rock *arock = (struct allprop_rock *) rock;
    const struct prop_entry *pentry;
    char *href, *name;
    xmlNsPtr ns;
    xmlNodePtr node;

    /* Make sure its a shared entry or the user's private one */
    if (*userid && strcmp(userid, arock->fctx->userid)) return 0;

    /* Split entry into namespace href and name ( <href>name ) */
    buf_setcstr(&arock->fctx->buf, entry + strlen(DAV_ANNOT_NS) + 1);
    href = (char *) buf_cstring(&arock->fctx->buf);
    if ((name = strchr(href, '>'))) *name++ = '\0';
    else if ((name = strchr(href, ':'))) *name++ = '\0';

    /* Look for a match against live properties */
    for (pentry = arock->fctx->lprops;
         pentry->name &&
             (strcmp(name, pentry->name) ||
              strcmp(href, known_namespaces[pentry->ns].href));
         pentry++);

    if (pentry->name &&
        (arock->fctx->mode == PROPFIND_ALL    /* Skip all live properties */
         || (pentry->flags & PROP_ALLPROP)))  /* Skip those already included */
        return 0;

    /* Look for an instance of this namespace in our response */
    ns = hash_lookup(href, arock->fctx->ns_table);
    if (!ns) {
        char prefix[5];
        snprintf(prefix, sizeof(prefix), "X%u", arock->fctx->prefix_count++);
        ns = xmlNewNs(arock->fctx->root, BAD_CAST href, BAD_CAST prefix);
        hash_insert(href, ns, arock->fctx->ns_table);
    }

    /* Add the dead property to the response */
    node = xml_add_prop(HTTP_OK, arock->fctx->ns[NS_DAV],
                        &arock->propstat[PROPSTAT_OK],
                        BAD_CAST name, ns, NULL, 0);

    if (arock->fctx->mode == PROPFIND_ALL) {
        xmlAddChild(node, xmlNewCDataBlock(arock->fctx->root->doc,
                                           BAD_CAST attrib->s, attrib->len));
    }

    return 0;
}


static int prescreen_prop(const struct prop_entry *entry,
                          xmlNodePtr prop,
                          struct propfind_ctx *fctx)
{
    unsigned allowed = 1;

    if (fctx->req_tgt->resource && !(entry->flags & PROP_RESOURCE)) allowed = 0;
    else if (entry->flags & PROP_PRESCREEN) {
        allowed = !entry->get(prop->name, NULL, fctx,
                              prop, NULL, NULL, entry->rock);
    }

    return allowed;
}


/* Parse the requested properties and create a linked list of fetch callbacks.
 * The list gets reused for each href if Depth > 0
 */
static int preload_proplist(xmlNodePtr proplist, struct propfind_ctx *fctx)
{
    int ret = 0;
    xmlNodePtr prop;
    const struct prop_entry *entry;
    struct propfind_entry_list *tail = NULL;

    switch (fctx->mode) {
    case PROPFIND_ALL:
    case PROPFIND_NAME:
        /* Add live properties for allprop/propname */
        for (entry = fctx->lprops; entry->name; entry++) {
            if (entry->flags & PROP_ALLPROP) {
                /* Pre-screen request based on prop flags */
                int allowed = prescreen_prop(entry, NULL, fctx);

                if (allowed || fctx->mode == PROPFIND_ALL) {
                    struct propfind_entry_list *nentry =
                        xzmalloc(sizeof(struct propfind_entry_list));

                    ensure_ns(fctx->ns, entry->ns, fctx->root,
                              known_namespaces[entry->ns].href,
                              known_namespaces[entry->ns].prefix);

                    nentry->name = xmlStrdup(BAD_CAST entry->name);
                    nentry->ns = fctx->ns[entry->ns];
                    if (allowed) {
                        nentry->flags = entry->flags;
                        nentry->get = entry->get;
                        nentry->prop = NULL;
                        nentry->rock = entry->rock;
                    }

                    /* Append new entry to linked list */
                    if (tail) {
                        tail->next = nentry;
                        tail = nentry;
                    }
                    else tail = fctx->elist = nentry;
                }
            }
        }
        /* Fall through and build hash table of namespaces */

    case PROPFIND_EXPAND:
        /* Add all namespaces attached to the response to our hash table */
        if (!fctx->ns_table->size) {
            xmlNsPtr nsDef;

            construct_hash_table(fctx->ns_table, 10, 1);

            for (nsDef = fctx->root->nsDef; nsDef; nsDef = nsDef->next) {
                hash_insert((const char *) nsDef->href, nsDef, fctx->ns_table);
            }
        }
    }

    /* Iterate through requested properties */
    for (prop = proplist; !*fctx->ret && prop; prop = prop->next) {
        if (prop->type == XML_ELEMENT_NODE) {
            struct propfind_entry_list *nentry;
            xmlChar *name, *namespace = NULL;
            xmlNsPtr ns = prop->ns;
            const char *ns_href = (const char *) ns->href;
            unsigned i;

            if (fctx->mode == PROPFIND_EXPAND) {
                /* Get name/namespace from <property> */
                name = xmlGetProp(prop, BAD_CAST "name");
                namespace = xmlGetProp(prop, BAD_CAST "namespace");

                if (namespace) {
                    ns_href = (const char *) namespace;

                    /* Look for this namespace in hash table */
                    ns = hash_lookup(ns_href, fctx->ns_table);
                    if (!ns) {
                        char prefix[6];
                        snprintf(prefix, sizeof(prefix),
                                 "X%X", strhash(ns_href) & 0xffff);
                        ns = xmlNewNs(fctx->root,
                                      BAD_CAST ns_href, BAD_CAST prefix);
                        hash_insert(ns_href, ns, fctx->ns_table);
                    }
                }
            }
            else {
                /* node IS the property */
                name = xmlStrdup(prop->name);
            }

            /* Look for this namespace in our known array */
            for (i = 0; i < NUM_NAMESPACE; i++) {
                if (!strcmp(ns_href, known_namespaces[i].href)) {
                    ensure_ns(fctx->ns, i, fctx->root,
                              known_namespaces[i].href,
                              known_namespaces[i].prefix);
                    ns = fctx->ns[i];
                    break;
                }
            }

            if (namespace) xmlFree(namespace);

            /* Look for a match against our known properties */
            for (entry = fctx->lprops;
                 entry->name &&
                     (strcmp((const char *) name, entry->name) ||
                      strcmp((const char *) ns->href,
                             known_namespaces[entry->ns].href));
                 entry++);

            /* Skip properties already included by allprop */
            if (fctx->mode == PROPFIND_ALL && (entry->flags & PROP_ALLPROP)) {
                xmlFree(name);
                continue;
            }

            nentry = xzmalloc(sizeof(struct propfind_entry_list));
            nentry->name = name;
            nentry->ns = ns;
            if (entry->name) {
                /* Found a match - Pre-screen request based on prop flags */
                if (prescreen_prop(entry, prop, fctx)) {
                    nentry->flags = entry->flags;
                    nentry->get = entry->get;
                    nentry->prop = prop;
                    nentry->rock = entry->rock;
                }
                ret = *fctx->ret;
            }
            else {
                /* No match, treat as a dead property.  Need to look for both collections
                 * resources */
                nentry->flags = PROP_COLLECTION | PROP_RESOURCE;
                nentry->get = propfind_fromdb;
                nentry->prop = NULL;
                nentry->rock = NULL;
            }

            /* Append new entry to linked list */
            if (tail) {
                tail->next = nentry;
                tail = nentry;
            }
            else tail = fctx->elist = nentry;
        }
    }

    return ret;
}


/* Execute the given property patch instructions */
static int do_proppatch(struct proppatch_ctx *pctx, xmlNodePtr instr)
{
    struct propstat propstat[NUM_PROPSTAT];
    int i;

    memset(propstat, 0, NUM_PROPSTAT * sizeof(struct propstat));

    /* Iterate through propertyupdate children */
    for (; instr; instr = instr->next) {
        if (instr->type == XML_ELEMENT_NODE) {
            xmlNodePtr prop;
            unsigned set = 0;

            if (!xmlStrcmp(instr->name, BAD_CAST "set")) set = 1;
            else if ((pctx->meth == METH_PROPPATCH) &&
                     !xmlStrcmp(instr->name, BAD_CAST "remove")) set = 0;
            else {
                syslog(LOG_INFO, "Unknown PROPPATCH instruction");
                pctx->err->desc = "Unknown PROPPATCH instruction";
                return HTTP_BAD_REQUEST;
            }

            /* Find child element */
            for (prop = instr->children;
                 prop && prop->type != XML_ELEMENT_NODE; prop = prop->next);
            if (!prop || xmlStrcmp(prop->name, BAD_CAST "prop")) {
                pctx->err->desc = "Missing prop element";
                return HTTP_BAD_REQUEST;
            }

            /* Iterate through requested properties */
            for (prop = prop->children; prop; prop = prop->next) {
                if (prop->type == XML_ELEMENT_NODE) {
                    const struct prop_entry *entry;

                    /* Look for a match against our known properties */
                    for (entry = pctx->lprops;
                         entry->name &&
                             (strcmp((const char *) prop->name, entry->name) ||
                              strcmp((const char *) prop->ns->href,
                                     known_namespaces[entry->ns].href));
                         entry++);

                    if (entry->name) {
                        if (!entry->put) {
                            /* Protected property */
                            xml_add_prop(HTTP_FORBIDDEN, pctx->ns[NS_DAV],
                                         &propstat[PROPSTAT_FORBID],
                                         prop->name, prop->ns, NULL,
                                         DAV_PROT_PROP);
                            *pctx->ret = HTTP_FORBIDDEN;
                        }
                        else {
                            /* Write "live" property */
                            entry->put(prop, set, pctx, propstat, entry->rock);
                        }
                    }
                    else {
                        /* Write "dead" property */
                        proppatch_todb(prop, set, pctx, propstat, NULL);
                    }
                }
            }
        }
    }

    /* One or more of the properties failed */
    if (*pctx->ret && propstat[PROPSTAT_OK].root) {
        /* 200 status must become 424 */
        propstat[PROPSTAT_FAILEDDEP].root = propstat[PROPSTAT_OK].root;
        propstat[PROPSTAT_FAILEDDEP].status = HTTP_FAILED_DEP;
        propstat[PROPSTAT_OK].root = NULL;
    }

    /* Add status and optional error to the propstat elements
       and then add them to the response element */
    for (i = 0; i < NUM_PROPSTAT; i++) {
        struct propstat *stat = &propstat[i];

        if (stat->root) {
            xmlNewChild(stat->root, NULL, BAD_CAST "status",
                        BAD_CAST http_statusline(stat->status));
            if (stat->precond) {
                struct error_t error = { NULL, stat->precond, NULL, 0 };
                xml_add_error(stat->root, &error, pctx->ns);
            }

            xmlAddChild(pctx->root, stat->root);
        }
    }

    return 0;
}


/* Parse an XML body into a tree */
int parse_xml_body(struct transaction_t *txn, xmlNodePtr *root)
{
    const char **hdr;
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc = NULL;
    int r = 0;

    *root = NULL;

    /* Read body */
    txn->req_body.flags |= BODY_DECODE;
    r = http_read_body(httpd_in, httpd_out,
                       txn->req_hdrs, &txn->req_body, &txn->error.desc);
    if (r) {
        txn->flags.conn = CONN_CLOSE;
        return r;
    }

    if (!buf_len(&txn->req_body.payload)) return 0;

    /* Check Content-Type */
    if (!(hdr = spool_getheader(txn->req_hdrs, "Content-Type")) ||
        (!is_mediatype("text/xml", hdr[0]) &&
         !is_mediatype("application/xml", hdr[0]))) {
        txn->error.desc = "This method requires an XML body\r\n";
        return HTTP_BAD_MEDIATYPE;
    }

    /* Parse the XML request */
    ctxt = xmlNewParserCtxt();
    if (ctxt) {
        doc = xmlCtxtReadMemory(ctxt, buf_cstring(&txn->req_body.payload),
                                buf_len(&txn->req_body.payload), NULL, NULL,
                                XML_PARSE_NOWARNING);
        xmlFreeParserCtxt(ctxt);
    }
    if (!doc) {
        txn->error.desc = "Unable to parse XML body\r\n";
        return HTTP_BAD_REQUEST;
    }

    /* Get the root element of the XML request */
    if (!(*root = xmlDocGetRootElement(doc))) {
        txn->error.desc = "Missing root element in request\r\n";
        return HTTP_BAD_REQUEST;
    }

    return 0;
}

static void set_system_acls(struct buf *buf, mbentry_t *mbentry)
{
    char *userid, *aclstr;

    /* Parse the ACL string (userid/rights pairs) */
    userid = aclstr = xstrdup(mbentry->acl);

    while (userid) {
        char *rightstr, *nextid;

        rightstr = strchr(userid, '\t');
        if (!rightstr) break;
        *rightstr++ = '\0';

        nextid = strchr(rightstr, '\t');
        if (!nextid) break;
        *nextid++ = '\0';

        /* we only want system users */
        if (is_system_user(userid))
            buf_printf(buf, "%s\t%s\t", userid, rightstr);

        userid = nextid;
    }

    free(aclstr);
}

/* Perform an ACL request
 *
 * preconditions:
 *   DAV:no-ace-conflict
 *   DAV:no-protected-ace-conflict
 *   DAV:no-inherited-ace-conflict
 *   DAV:limited-number-of-aces
 *   DAV:deny-before-grant
 *   DAV:grant-only
 *   DAV:no-invert
 *   DAV:no-abstract
 *   DAV:not-supported-privilege
 *   DAV:missing-required-principal
 *   DAV:recognized-principal
 *   DAV:allowed-principal
 */
int meth_acl(struct transaction_t *txn, void *params)
{
    struct meth_params *aparams = (struct meth_params *) params;
    int ret = 0, r, rights, overwrite = OVERWRITE_YES;
    xmlDocPtr indoc = NULL;
    xmlNodePtr root, ace;
    struct mailbox *mailbox = NULL;
    struct buf acl = BUF_INITIALIZER;
    const char **hdr;

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    if ((r = aparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    /* Make sure method is allowed (only allowed on collections) */
    if (!(txn->req_tgt.allow & ALLOW_WRITECOL)) {
        txn->error.desc = "ACLs can only be set on collections\r\n";
        syslog(LOG_DEBUG, "Tried to set ACL on non-collection");
        return HTTP_NOT_ALLOWED;
    }

    if (!mboxname_userownsmailbox(httpd_userid, txn->req_tgt.mbentry->name)) {
        /* Check ACL for current user */
        rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
        if (!(rights & DACL_ADMIN)) {
            /* DAV:need-privileges */
            txn->error.precond = DAV_NEED_PRIVS;
            txn->error.resource = txn->req_tgt.path;
            txn->error.rights = DACL_ADMIN;
            return HTTP_NO_PRIVS;
        }
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Check for "IMAP-mode" */
    if ((hdr = spool_getheader(txn->req_hdrs, "Overwrite")) &&
        !strcmp(hdr[0], "F")) {
        overwrite = OVERWRITE_NO;
    }

    if (overwrite) {
        /* Open mailbox for writing */
        r = mailbox_open_iwl(txn->req_tgt.mbentry->name, &mailbox);
        if (r) {
            syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
                   txn->req_tgt.mbentry->name, error_message(r));
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }
    }

    /* Parse the ACL body */
    ret = parse_xml_body(txn, &root);
    if (!ret && !root) {
        txn->error.desc = "Missing request body\r\n";
        ret = HTTP_BAD_REQUEST;
    }
    if (ret) goto done;

    indoc = root->doc;

    /* Make sure its an DAV:acl element */
    if (xmlStrcmp(root->name, BAD_CAST "acl")) {
        txn->error.desc = "Missing acl element in ACL request\r\n";
        ret = HTTP_BAD_REQUEST;
        goto done;
    }

    set_system_acls(&acl, txn->req_tgt.mbentry);

    /* Parse the DAV:ace elements */
    for (ace = root->children; ace; ace = ace->next) {
        if (ace->type == XML_ELEMENT_NODE) {
            xmlNodePtr child = NULL, prin = NULL, privs = NULL;
            const char *userid = NULL;
            int rights = 0;
            char rightstr[100];
            struct request_target_t tgt;

            for (child = ace->children; child; child = child->next) {
                if (child->type == XML_ELEMENT_NODE) {
                    if (!xmlStrcmp(child->name, BAD_CAST "principal")) {
                        if (prin) {
                            txn->error.desc = "Multiple principals in ACE\r\n";
                            ret = HTTP_BAD_REQUEST;
                            goto done;
                        }

                        for (prin = child->children; prin &&
                             prin->type != XML_ELEMENT_NODE; prin = prin->next);
                    }
                    else if (!xmlStrcmp(child->name, BAD_CAST "grant")) {
                        if (privs) {
                            txn->error.desc = "Multiple grant|deny in ACE\r\n";
                            ret = HTTP_BAD_REQUEST;
                            goto done;
                        }

                        for (privs = child->children; privs &&
                             privs->type != XML_ELEMENT_NODE; privs = privs->next);
                    }
                    else if (!xmlStrcmp(child->name, BAD_CAST "deny")) {
                        /* DAV:grant-only */
                        txn->error.precond = DAV_GRANT_ONLY;
                        ret = HTTP_FORBIDDEN;
                        goto done;
                    }
                    else if (!xmlStrcmp(child->name, BAD_CAST "invert")) {
                        /* DAV:no-invert */
                        txn->error.precond = DAV_NO_INVERT;
                        ret = HTTP_FORBIDDEN;
                        goto done;
                    }
                    else {
                        txn->error.desc = "Unknown element in ACE\r\n";
                        ret = HTTP_BAD_REQUEST;
                        goto done;
                    }
                }
            }

            if (!xmlStrcmp(prin->name, BAD_CAST "self")) {
                userid = proxy_userid;
            }
            else if (!xmlStrcmp(prin->name, BAD_CAST "owner")) {
                userid = mboxname_to_userid(txn->req_tgt.mbentry->name);
            }
            else if (!xmlStrcmp(prin->name, BAD_CAST "authenticated")) {
                userid = "anyone";
            }
            else if (!xmlStrcmp(prin->name, BAD_CAST "unauthenticated")) {
                userid = "anonymous";
            }
            else if (!xmlStrcmp(prin->name, BAD_CAST "href")) {
                xmlChar *href = xmlNodeGetContent(prin);
                xmlURIPtr uri;
                const char *errstr = NULL;
                size_t plen = strlen(namespace_principal.prefix);

                uri = parse_uri(METH_UNKNOWN, (const char *) href, 1, &errstr);
                if (uri &&
                    !strncmp(namespace_principal.prefix, uri->path, plen) &&
                    uri->path[plen] == '/') {
                    memset(&tgt, 0, sizeof(struct request_target_t));
                    tgt.namespace = URL_NS_PRINCIPAL;
                    /* XXX: there is no doubt that this leaks memory */
                    r = principal_parse_path(uri->path, &tgt, &errstr);
                    if (!r && tgt.userid) userid = tgt.userid;
                }
                if (uri) xmlFreeURI(uri);
                xmlFree(href);
            }

            if (!userid) {
                /* DAV:recognized-principal */
                txn->error.precond = DAV_RECOG_PRINC;
                ret = HTTP_FORBIDDEN;
                goto done;
            }

            if (is_system_user(userid)) {
                /* DAV:allowed-principal */
                txn->error.precond = DAV_ALLOW_PRINC;
                ret = HTTP_FORBIDDEN;
                goto done;
            }

            for (; privs; privs = privs->next) {
                if (privs->type == XML_ELEMENT_NODE) {
                    xmlNodePtr priv = privs->children;
                    for (; priv->type != XML_ELEMENT_NODE; priv = priv->next);

                    if (aparams->acl_ext &&
                        aparams->acl_ext(txn, priv, &rights)) {
                        /* Extension (CalDAV) privileges */
                        if (txn->error.precond) {
                            ret = HTTP_FORBIDDEN;
                            goto done;
                        }
                    }
                    else if (!xmlStrcmp(priv->ns->href,
                                        BAD_CAST XML_NS_DAV)) {
                        /* WebDAV privileges */
                        if (!xmlStrcmp(priv->name,
                                       BAD_CAST "all")) {
                            rights |= DACL_ALL;
                        }
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "read"))
                            rights |= DACL_READ;
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "write"))
                            rights |= DACL_WRITE;
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "write-content"))
                            rights |= DACL_WRITECONT;
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "write-properties"))
                            rights |= DACL_WRITEPROPS;
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "bind"))
                            rights |= DACL_BIND;
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "unbind"))
                            rights |= DACL_UNBIND;
                        else if (!xmlStrcmp(priv->name,
                                            BAD_CAST "read-current-user-privilege-set")
                                 || !xmlStrcmp(priv->name,
                                               BAD_CAST "read-acl")
                                 || !xmlStrcmp(priv->name,
                                               BAD_CAST "write-acl")
                                 || !xmlStrcmp(priv->name,
                                               BAD_CAST "unlock")) {
                            /* DAV:no-abstract */
                            txn->error.precond = DAV_NO_ABSTRACT;
                            ret = HTTP_FORBIDDEN;
                            goto done;
                        }
                        else {
                            /* DAV:not-supported-privilege */
                            txn->error.precond = DAV_SUPP_PRIV;
                            ret = HTTP_FORBIDDEN;
                            goto done;
                        }
                    }
                    else if (!xmlStrcmp(priv->ns->href,
                                   BAD_CAST XML_NS_CALDAV)) {
                        if (!xmlStrcmp(priv->name,
                                       BAD_CAST "read-free-busy"))
                            rights |= DACL_READFB;
                        else {
                            /* DAV:not-supported-privilege */
                            txn->error.precond = DAV_SUPP_PRIV;
                            ret = HTTP_FORBIDDEN;
                            goto done;
                        }
                    }
                    else if (!xmlStrcmp(priv->ns->href,
                                   BAD_CAST XML_NS_CYRUS)) {
                        /* Cyrus-specific privileges */
                        if (!xmlStrcmp(priv->name,
                                       BAD_CAST "make-collection"))
                            rights |= DACL_MKCOL;
                        else if (!xmlStrcmp(priv->name,
                                       BAD_CAST "remove-collection"))
                            rights |= DACL_RMCOL;
                        else if (!xmlStrcmp(priv->name,
                                       BAD_CAST "add-resource"))
                            rights |= DACL_ADDRSRC;
                        else if (!xmlStrcmp(priv->name,
                                       BAD_CAST "remove-resource"))
                            rights |= DACL_RMRSRC;
                        else if (!xmlStrcmp(priv->name,
                                       BAD_CAST "admin"))
                            rights |= DACL_ADMIN;
                        else {
                            /* DAV:not-supported-privilege */
                            txn->error.precond = DAV_SUPP_PRIV;
                            ret = HTTP_FORBIDDEN;
                            goto done;
                        }
                    }
                    else {
                        /* DAV:not-supported-privilege */
                        txn->error.precond = DAV_SUPP_PRIV;
                        ret = HTTP_FORBIDDEN;
                        goto done;
                    }
                }
            }

            /* gotta have something to do! */
            if (rights) {
                cyrus_acl_masktostr(rights, rightstr);
                buf_printf(&acl, "%s\t%s\t", userid, rightstr);
            }
        }
    }

    mailbox_set_acl(mailbox, buf_cstring(&acl), 1);
    r = mboxlist_sync_setacls(txn->req_tgt.mbentry->name, buf_cstring(&acl));
    if (r) {
        syslog(LOG_ERR, "mboxlist_setacl(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    response_header(HTTP_OK, txn);

  done:
    buf_free(&acl);
    if (indoc) xmlFreeDoc(indoc);
    mailbox_close(&mailbox);
    return ret;
}


/* Perform a COPY/MOVE request
 *
 * preconditions:
 *   *DAV:need-privileges
 */
int meth_copy_move(struct transaction_t *txn, void *params)
{
    struct meth_params *cparams = (struct meth_params *) params;
    int ret = HTTP_CREATED, r, precond, rights;
    const char **hdr;
    xmlURIPtr dest_uri;
    static struct request_target_t dest_tgt;  /* Parsed destination URL -
                                                 static for Location resp hdr */
    struct backend *src_be = NULL, *dest_be = NULL;
    struct mailbox *src_mbox = NULL, *dest_mbox = NULL;
    struct dav_data *ddata;
    struct index_record src_rec;
    const char *etag = NULL;
    time_t lastmod = 0;
    unsigned meth_move = (txn->meth == METH_MOVE);
    void *src_davdb = NULL, *dest_davdb = NULL, *obj = NULL;
    struct buf msg_buf = BUF_INITIALIZER;

    memset(&dest_tgt, 0, sizeof(struct request_target_t));

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the source path */
    if ((r = cparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    /* Make sure method is allowed (not allowed on collections yet) */
    if (!(txn->req_tgt.allow & ALLOW_WRITE)) return HTTP_NOT_ALLOWED;

    /* Check ACL for current user on source mailbox */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
    if (((rights & DACL_READ) != DACL_READ) ||
        (meth_move && !(rights & DACL_RMRSRC))) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights =
            (rights & DACL_READ) != DACL_READ ? DACL_READ : DACL_RMRSRC;
        return HTTP_NO_PRIVS;
    }

    /* Check for mandatory Destination header */
    if (!(hdr = spool_getheader(txn->req_hdrs, "Destination"))) {
        txn->error.desc = "Missing Destination header";
        return HTTP_BAD_REQUEST;
    }

    /* Parse destination URI */
    if (!(dest_uri = parse_uri(METH_UNKNOWN, hdr[0], 1, &txn->error.desc))) {
        txn->error.desc = "Illegal Destination target URI";
        return HTTP_BAD_REQUEST;
    }

    /* Make sure source and dest resources are NOT the same */
    if (!strcmp(txn->req_uri->path, dest_uri->path)) {
        txn->error.desc = "Source and destination resources are the same";
        ret = HTTP_FORBIDDEN;
        goto done;
    }

    /* Parse the destination path */
    r = cparams->parse_path(dest_uri->path, &dest_tgt, &txn->error.desc);
    if (r) {
        ret = HTTP_FORBIDDEN;
        goto done;
    }

    /* We don't yet handle COPY/MOVE on collections */
    if (!dest_tgt.resource) {
        ret = HTTP_NOT_ALLOWED;
        goto done;
    }

    /* Check ACL for current user on destination */
    rights = httpd_myrights(httpd_authstate, dest_tgt.mbentry->acl);
    if (!(rights & DACL_ADDRSRC) || !(rights & DACL_WRITECONT)) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = dest_tgt.path;
        txn->error.rights =
            !(rights & DACL_ADDRSRC) ? DACL_ADDRSRC : DACL_WRITECONT;
        ret = HTTP_NO_PRIVS;
        goto done;
    }

    /* Check we're not copying within the same user */
    if (!meth_move && mboxname_same_userid(dest_tgt.mbentry->name, txn->req_tgt.mbentry->name)) {
        /* XXX - need generic error for uid-conflict */
        txn->error.desc = "Can only move within same user";
        ret = HTTP_NOT_ALLOWED;
        goto done;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote source mailbox */

        /* Replace cached Destination header with just the absolute path */
        spool_replace_header(xstrdup("Destination"),
                             dest_tgt.path, txn->req_hdrs);

        if (!dest_tgt.mbentry->server) {
            /* Local destination mailbox */

            /* XXX  Currently only supports standard Murder */
            txn->error.desc = "COPY/MOVE only supported in a standard Murder";
            ret = HTTP_NOT_ALLOWED;
        }
        else if (!(src_be = proxy_findserver(txn->req_tgt.mbentry->server,
                                             &http_protocol, proxy_userid,
                                             &backend_cached, NULL, NULL,
                                             httpd_in))) {
            txn->error.desc = "Unable to connect to source backend";
            ret = HTTP_UNAVAILABLE;
        }
        else if (!(dest_be = proxy_findserver(dest_tgt.mbentry->server,
                                              &http_protocol, proxy_userid,
                                              &backend_cached, NULL, NULL,
                                              httpd_in))) {
            txn->error.desc = "Unable to connect to destination backend";
            ret = HTTP_UNAVAILABLE;
        }
        else if (src_be == dest_be) {
            /* Simply send the COPY to the backend */
            ret = http_pipe_req_resp(src_be, txn);
        }
        else {
            /* This is the harder case: GET from source and PUT on dest */
            ret = http_proxy_copy(src_be, dest_be, txn);
        }

        goto done;
    }
    else if (dest_tgt.mbentry->server) {
        /* Local source and remote destination mailbox */

        /* XXX  Currently only supports standard Murder */
        txn->error.desc = "COPY/MOVE only supported in a standard Murder";
        ret = HTTP_NOT_ALLOWED;
        goto done;
    }

    /* Local source and destination mailboxes */

    if (meth_move) {
        /* Open source mailbox for writing */
        r = mailbox_open_iwl(txn->req_tgt.mbentry->name, &src_mbox);
    }
    else {
        /* Open source mailbox for reading */
        r = mailbox_open_irl(txn->req_tgt.mbentry->name, &src_mbox);
    }
    if (r) {
        syslog(LOG_ERR, "mailbox_open_i%cl(%s) failed: %s",
               meth_move ? 'w' : 'r', txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Open the DAV DB corresponding to the src mailbox */
    src_davdb = cparams->davdb.open_db(src_mbox);

    /* Find message UID for the source resource */
    cparams->davdb.lookup_resource(src_davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource,
                                   (void **) &ddata, 0);
    if (!ddata->rowid) {
        ret = HTTP_NOT_FOUND;
        goto done;
    }

    if (ddata->imap_uid) {
        /* Mapped URL - Fetch index record for the resource */
        r = mailbox_find_index_record(src_mbox, ddata->imap_uid, &src_rec);
        if (r) {
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        etag = message_guid_encode(&src_rec.guid);
        lastmod = src_rec.internaldate;
    }
    else {
        /* Unmapped URL (empty resource) */
        etag = NULL_ETAG;
        lastmod = ddata->creationdate;
    }

    /* Check any preconditions on source */
    precond = cparams->check_precond(txn, (void **) ddata, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
        break;

    case HTTP_LOCKED:
        txn->error.precond = DAV_NEED_LOCK_TOKEN;
        txn->error.resource = txn->req_tgt.path;

    default:
        /* We failed a precondition - don't perform the request */
        ret = precond;
        goto done;
    }

    /* Load message containing the resource and parse data */
    mailbox_map_record(src_mbox, &src_rec, &msg_buf);
    obj = cparams->mime_types[0].from_string(buf_base(&msg_buf) +
                                             src_rec.header_size);
    buf_free(&msg_buf);

    if (!meth_move) {
        /* Done with source mailbox */
        mailbox_unlock_index(src_mbox, NULL);
    }

    /* Open dest mailbox for writing */
    r = mailbox_open_iwl(dest_tgt.mbentry->name, &dest_mbox);
    if (r) {
        syslog(LOG_ERR, "mailbox_open_iwl(%s) failed: %s",
               dest_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Open the DAV DB corresponding to the dest mailbox */
    dest_davdb = cparams->davdb.open_db(dest_mbox);

    /* Find message UID for the dest resource, if exists */
    cparams->davdb.lookup_resource(dest_davdb, dest_tgt.mbentry->name,
                                   dest_tgt.resource, (void **) &ddata, 0);
    /* XXX  Check errors */

    /* Check any preconditions on destination */
    if ((hdr = spool_getheader(txn->req_hdrs, "Overwrite")) &&
        !strcmp(hdr[0], "F")) {

        if (ddata->rowid) {
            /* Don't overwrite the destination resource */
            ret = HTTP_PRECOND_FAILED;
            goto done;
        }
    }

    /* Store the resource at destination */
    ret = cparams->copy(txn, obj, dest_mbox, dest_tgt.resource, dest_davdb);

    /* Done with destination mailbox */
    mailbox_unlock_index(dest_mbox, NULL);

    switch (ret) {
    case HTTP_CREATED:
    case HTTP_NO_CONTENT:
        if (meth_move) {
            /* Expunge the source message */
            src_rec.system_flags |= FLAG_EXPUNGED;
            if ((r = mailbox_rewrite_index_record(src_mbox, &src_rec))) {
                syslog(LOG_ERR, "expunging src record (%s) failed: %s",
                       txn->req_tgt.mbentry->name, error_message(r));
                txn->error.desc = error_message(r);
                ret = HTTP_SERVER_ERROR;
                goto done;
            }
        }
    }

  done:
    if (ret == HTTP_CREATED) {
        /* Tell client where to find the new resource */
        txn->location = dest_tgt.path;
    }
    else {
        /* Don't confuse client by providing ETag of Destination resource */
        txn->resp_body.etag = NULL;
    }

    if (obj) cparams->mime_types[0].free(obj);
    if (dest_davdb) cparams->davdb.close_db(dest_davdb);
    if (dest_mbox) mailbox_close(&dest_mbox);
    if (src_davdb) cparams->davdb.close_db(src_davdb);
    if (src_mbox) mailbox_close(&src_mbox);

    xmlFreeURI(dest_uri);
    free(dest_tgt.userid);
    mboxlist_entry_free(&dest_tgt.mbentry);

    return ret;
}

/* this is when a user tries to delete a mailbox that they don't own, and don't
 * have permission to delete */
static int remove_user_acl(const char *userid, const char *mboxname)
{
    return mboxlist_setacl(&httpd_namespace, mboxname, userid, /*rights*/NULL,
                           /*isadmin*/1, httpd_userid, httpd_authstate);
}

struct delete_rock {
    struct transaction_t *txn;
    struct mailbox *mailbox;
    delete_proc_t deletep;
};

static int delete_cb(void *rock, void *data)
{
    struct delete_rock *drock = (struct delete_rock *) rock;
    struct dav_data *ddata = (struct dav_data *) data;
    struct index_record record;
    int r;

    if (!ddata->imap_uid) {
        /* Unmapped URL (empty resource) */
        return 0;
    }

    /* Fetch index record for the resource */
    r = mailbox_find_index_record(drock->mailbox, ddata->imap_uid, &record);
    if (r) {
        drock->txn->error.desc = error_message(r);
        return HTTP_SERVER_ERROR;
    }

    r = drock->deletep(drock->txn, drock->mailbox, &record, data);

    return r;
}

/* Perform a DELETE request */
int meth_delete(struct transaction_t *txn, void *params)
{
    struct meth_params *dparams = (struct meth_params *) params;
    int ret = HTTP_NO_CONTENT, r = 0, precond, rights, needrights;
    struct mboxevent *mboxevent = NULL;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record record;
    const char *etag = NULL;
    time_t lastmod = 0;
    void *davdb = NULL;

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    r = dparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc);
    if (r) return r;

    /* Make sure method is allowed */
    if (!(txn->req_tgt.allow & ALLOW_DELETE)) return HTTP_NOT_ALLOWED;

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
    needrights = txn->req_tgt.resource ? DACL_RMRSRC : DACL_RMCOL;
    if (!(rights & needrights)) {
        /* special case delete mailbox where not the owner */
        if (!txn->req_tgt.resource &&
            !mboxname_userownsmailbox(httpd_userid, txn->req_tgt.mbentry->name) &&
            !remove_user_acl(httpd_userid, txn->req_tgt.mbentry->name)) {
            return HTTP_OK;
        }

        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        return HTTP_NO_PRIVS;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    if (!txn->req_tgt.resource) {
        /* DELETE collection */

        if (dparams->delete) {
            /* Do special processing on all resources */
            struct delete_rock drock = { txn, NULL, dparams->delete };

            /* Open mailbox for reading */
            r = mailbox_open_irl(txn->req_tgt.mbentry->name, &mailbox);
            if (r) {
                syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
                       txn->req_tgt.mbentry->name, error_message(r));
                txn->error.desc = error_message(r);
                return HTTP_SERVER_ERROR;
            }

            /* Open the DAV DB corresponding to the mailbox */
            davdb = dparams->davdb.open_db(mailbox);

            drock.mailbox = mailbox;
            r = dparams->davdb.foreach_resource(davdb, mailbox->name,
                                                  &delete_cb, &drock);
            /* we need the mailbox closed before we delete it */
            mailbox_close(&mailbox);
            if (r) {
                txn->error.desc = error_message(r);
                return HTTP_SERVER_ERROR;
            }
        }

        mboxevent = mboxevent_new(EVENT_MAILBOX_DELETE);

        if (mboxlist_delayed_delete_isenabled()) {
            r = mboxlist_delayed_deletemailbox(txn->req_tgt.mbentry->name,
                                       httpd_userisadmin || httpd_userisproxyadmin,
                                       httpd_userid, httpd_authstate, mboxevent,
                                       /*checkack*/1, /*localonly*/0, /*force*/0);
        }
        else {
            r = mboxlist_deletemailbox(txn->req_tgt.mbentry->name,
                                       httpd_userisadmin || httpd_userisproxyadmin,
                                       httpd_userid, httpd_authstate, mboxevent,
                                       /*checkack*/1, /*localonly*/0, /*force*/0);
        }
        if (r == IMAP_PERMISSION_DENIED) ret = HTTP_FORBIDDEN;
        else if (r == IMAP_MAILBOX_NONEXISTENT) ret = HTTP_NOT_FOUND;
        else if (r) ret = HTTP_SERVER_ERROR;

        goto done;
    }

    /* DELETE resource */

    /* Open mailbox for writing */
    r = mailbox_open_iwl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = dparams->davdb.open_db(mailbox);

    /* Find message UID for the resource, if exists */
    dparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void **) &ddata, 0);
    if (!ddata->rowid) {
        ret = HTTP_NOT_FOUND;
        goto done;
    }

    memset(&record, 0, sizeof(struct index_record));
    if (ddata->imap_uid) {
        /* Mapped URL - Fetch index record for the resource */
        r = mailbox_find_index_record(mailbox, ddata->imap_uid, &record);
        if (r) {
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        etag = message_guid_encode(&record.guid);
        lastmod = record.internaldate;
    }
    else {
        /* Unmapped URL (empty resource) */
        etag = NULL_ETAG;
        lastmod = ddata->creationdate;
    }

    /* Check any preconditions */
    precond = dparams->check_precond(txn, (void *) ddata, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
        break;

    case HTTP_LOCKED:
        txn->error.precond = DAV_NEED_LOCK_TOKEN;
        txn->error.resource = txn->req_tgt.path;

    default:
        /* We failed a precondition - don't perform the request */
        ret = precond;
        goto done;
    }

    if (record.uid) {
        /* Do any special processing */
        if (dparams->delete) dparams->delete(txn, mailbox, &record, ddata);

        /* Expunge the resource */
        record.system_flags |= FLAG_EXPUNGED;

        mboxevent = mboxevent_new(EVENT_MESSAGE_EXPUNGE);

        r = mailbox_rewrite_index_record(mailbox, &record);

        if (r) {
            syslog(LOG_ERR, "expunging record (%s) failed: %s",
                   txn->req_tgt.mbentry->name, error_message(r));
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        mboxevent_extract_record(mboxevent, mailbox, &record);
        mboxevent_extract_mailbox(mboxevent, mailbox);
        mboxevent_set_numunseen(mboxevent, mailbox, -1);
        mboxevent_set_access(mboxevent, NULL, NULL, httpd_userid,
                             txn->req_tgt.mbentry->name, 0);
    }

  done:
    if (davdb) dparams->davdb.close_db(davdb);
    mailbox_close(&mailbox);

    if (!r)
        mboxevent_notify(mboxevent);
    mboxevent_free(&mboxevent);

    return ret;
}


/* Perform a GET/HEAD request on a DAV resource */
int meth_get_head(struct transaction_t *txn, void *params)
{
    struct meth_params *gparams = (struct meth_params *) params;
    const char **hdr;
    struct mime_type_t *mime = NULL;
    int ret = 0, r, precond, rights;
    const char *data = NULL;
    unsigned long datalen = 0, offset = 0;
    struct buf msg_buf = BUF_INITIALIZER;
    struct resp_body_t *resp_body = &txn->resp_body;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record record;
    const char *etag = NULL;
    time_t lastmod = 0;
    void *davdb = NULL;
    char *freeme = NULL;

    /* Parse the path */
    ret = gparams->parse_path(txn->req_uri->path,
                              &txn->req_tgt, &txn->error.desc);
    if (ret) return ret;

    if (txn->req_tgt.namespace == URL_NS_PRINCIPAL) {
        /* Special "principal" */
        if (txn->req_tgt.flags == TGT_SERVER_INFO) return server_info(txn);

        /* No content for principals (yet) */
        return HTTP_NO_CONTENT;
    }

    if (!txn->req_tgt.resource) {
        /* Do any collection processing */
        if (gparams->get) return gparams->get(txn, NULL, NULL, NULL);

        /* We don't handle GET on a collection */
        return HTTP_NO_CONTENT;
    }

    if (gparams->mime_types) {
        /* Check requested MIME type:
           1st entry in gparams->mime_types array MUST be default MIME type */
        if ((hdr = spool_getheader(txn->req_hdrs, "Accept")))
            mime = get_accept_type(hdr, gparams->mime_types);
        else mime = gparams->mime_types;
        if (!mime) return HTTP_NOT_ACCEPTABLE;
    }

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
    if (!(rights & DACL_READ)) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights = DACL_READ;
        return HTTP_NO_PRIVS;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Open mailbox for reading */
    r = mailbox_open_irl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        goto done;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = gparams->davdb.open_db(mailbox);

    /* Find message UID for the resource */
    gparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void **) &ddata, 0);
    if (!ddata->rowid) {
        ret = HTTP_NOT_FOUND;
        goto done;
    }

    memset(&record, 0, sizeof(struct index_record));
    if (ddata->imap_uid) {
        /* Mapped URL - Fetch index record for the resource */
        r = mailbox_find_index_record(mailbox, ddata->imap_uid, &record);
        if (r) {
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        txn->flags.ranges = 1;
        etag = message_guid_encode(&record.guid);
        lastmod = record.internaldate;
    }
    else {
        /* Unmapped URL (empty resource) */
        txn->flags.ranges = 0;
        etag = NULL_ETAG;
        lastmod = ddata->creationdate;
    }

    /* Check any preconditions, including range request */
    precond = gparams->check_precond(txn, (void *) ddata, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
    case HTTP_PARTIAL:
    case HTTP_NOT_MODIFIED:
        /* Fill in ETag, Last-Modified, Expires, and Cache-Control */
        resp_body->etag = etag;
        resp_body->lastmod = lastmod;
        resp_body->maxage = 3600;       /* 1 hr */
        txn->flags.cc |= CC_MAXAGE | CC_REVALIDATE;  /* don't use stale data */

        if (precond != HTTP_NOT_MODIFIED) break;

    default:
        /* We failed a precondition - don't perform the request */
        ret = precond;
        goto done;
    }

    /* Do any special processing */
    if (gparams->get) {
        ret = gparams->get(txn, mailbox, &record, ddata);
        if (ret) goto done;
    }

    if (record.uid) {
        if (mime) {
            txn->flags.vary |= VARY_ACCEPT;
            resp_body->type = mime->content_type;
        }

        /* Resource length doesn't include RFC 5322 header */
        offset = record.header_size;
        datalen = record.size - offset;

        if (txn->meth == METH_GET) {
            /* Load message containing the resource */
            r = mailbox_map_record(mailbox, &record, &msg_buf);
            if (r) goto done;

            /* iCalendar data in response should not be transformed */
            txn->flags.cc |= CC_NOTRANSFORM;

            data = buf_base(&msg_buf) + offset;

            if (mime != gparams->mime_types) {
                /* Not the storage format - convert into requested MIME type */
                void *obj = gparams->mime_types[0].from_string(data);

                data = freeme = mime->to_string(obj);
                datalen = strlen(data);
                gparams->mime_types[0].free(obj);
            }
        }
    }

    write_body(precond, txn, data, datalen);

    buf_free(&msg_buf);

  done:
    if (davdb) gparams->davdb.close_db(davdb);
    if (r) {
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
    }
    free(freeme);
    mailbox_close(&mailbox);

    return ret;
}


/* Perform a LOCK request
 *
 * preconditions:
 *   DAV:need-privileges
 *   DAV:no-conflicting-lock
 *   DAV:lock-token-submitted
 */
int meth_lock(struct transaction_t *txn, void *params)
{
    struct meth_params *lparams = (struct meth_params *) params;
    int ret = HTTP_OK, r, precond, rights;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record oldrecord;
    const char *etag;
    time_t lastmod;
    xmlDocPtr indoc = NULL, outdoc = NULL;
    xmlNodePtr root = NULL;
    xmlNsPtr ns[NUM_NAMESPACE];
    xmlChar *owner = NULL;
    time_t now = time(NULL);
    void *davdb = NULL;

    /* XXX  We ignore Depth and Timeout header fields */

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    if ((r = lparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    /* Make sure method is allowed (only allowed on resources) */
    if (!(txn->req_tgt.allow & ALLOW_WRITE)) return HTTP_NOT_ALLOWED;

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
    if (!(rights & DACL_WRITECONT) || !(rights & DACL_ADDRSRC)) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights =
            !(rights & DACL_WRITECONT) ? DACL_WRITECONT : DACL_ADDRSRC;
        return HTTP_NO_PRIVS;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Open mailbox for reading */
    r = mailbox_open_irl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = lparams->davdb.open_db(mailbox);
    lparams->davdb.begin_transaction(davdb);

    /* Find message UID for the resource, if exists */
    lparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void *) &ddata, 0);

    if (ddata->imap_uid) {
        /* Locking existing resource */

        /* Fetch index record for the resource */
        r = mailbox_find_index_record(mailbox, ddata->imap_uid, &oldrecord);
        if (r) {
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        etag = message_guid_encode(&oldrecord.guid);
        lastmod = oldrecord.internaldate;
    }
    else if (ddata->rowid) {
        /* Unmapped URL (empty resource) */
        etag = NULL_ETAG;
        lastmod = ddata->creationdate;
    }
    else {
        /* New resource */
        etag = NULL;
        lastmod = 0;

        ddata->creationdate = now;
        ddata->mailbox = mailbox->name;
        ddata->resource = txn->req_tgt.resource;
    }

    /* Check any preconditions */
    precond = lparams->check_precond(txn, ddata, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
        break;

    case HTTP_LOCKED:
        if (strcmp(ddata->lock_ownerid, httpd_userid))
            txn->error.precond = DAV_LOCKED;
        else
            txn->error.precond = DAV_NEED_LOCK_TOKEN;
        txn->error.resource = txn->req_tgt.path;

    default:
        /* We failed a precondition - don't perform the request */
        ret = precond;
        goto done;
    }

    if (ddata->lock_expire <= now) {
        /* Create new lock */
        xmlNodePtr node, sub;
        unsigned owner_is_href = 0;

        /* Parse the required body */
        ret = parse_xml_body(txn, &root);
        if (!ret && !root) {
            txn->error.desc = "Missing request body";
            ret = HTTP_BAD_REQUEST;
        }
        if (ret) goto done;

        /* Check for correct root element */
        indoc = root->doc;
        if (xmlStrcmp(root->name, BAD_CAST "lockinfo")) {
            txn->error.desc = "Incorrect root element in XML request\r\n";
            ret = HTTP_BAD_MEDIATYPE;
            goto done;
        }

        /* Parse elements of lockinfo */
        for (node = root->children; node; node = node->next) {
            if (node->type != XML_ELEMENT_NODE) continue;

            if (!xmlStrcmp(node->name, BAD_CAST "lockscope")) {
                /* Find child element of lockscope */
                for (sub = node->children;
                     sub && sub->type != XML_ELEMENT_NODE; sub = sub->next);
                /* Make sure its an exclusive element */
                if (!sub || xmlStrcmp(sub->name, BAD_CAST "exclusive")) {
                    txn->error.desc = "Only exclusive locks are supported";
                    ret = HTTP_BAD_REQUEST;
                    goto done;
                }
            }
            else if (!xmlStrcmp(node->name, BAD_CAST "locktype")) {
                /* Find child element of locktype */
                for (sub = node->children;
                     sub && sub->type != XML_ELEMENT_NODE; sub = sub->next);
                /* Make sure its a write element */
                if (!sub || xmlStrcmp(sub->name, BAD_CAST "write")) {
                    txn->error.desc = "Only write locks are supported";
                    ret = HTTP_BAD_REQUEST;
                    goto done;
                }
            }
            else if (!xmlStrcmp(node->name, BAD_CAST "owner")) {
                /* Find child element of owner */
                for (sub = node->children;
                     sub && sub->type != XML_ELEMENT_NODE; sub = sub->next);
                if (!sub) {
                    owner = xmlNodeGetContent(node);
                }
                /* Make sure its a href element */
                else if (xmlStrcmp(sub->name, BAD_CAST "href")) {
                    ret = HTTP_BAD_REQUEST;
                    goto done;
                }
                else {
                    owner_is_href = 1;
                    owner = xmlNodeGetContent(sub);
                }
            }
        }

        ddata->lock_ownerid = httpd_userid;
        if (owner) ddata->lock_owner = (const char *) owner;

        /* Construct lock-token */
        assert(!buf_len(&txn->buf));
        buf_printf(&txn->buf, XML_NS_CYRUS "lock/%s-%x-%u",
                   mailbox->uniqueid, strhash(txn->req_tgt.resource),
                   owner_is_href);

        ddata->lock_token = buf_cstring(&txn->buf);
    }

    /* Update lock expiration */
    ddata->lock_expire = now + 300;  /* 5 min */

    /* Start construction of our prop response */
    if (!(root = init_xml_response("prop", NS_DAV, root, ns))) {
        ret = HTTP_SERVER_ERROR;
        txn->error.desc = "Unable to create XML response\r\n";
        goto done;
    }

    outdoc = root->doc;
    root = xmlNewChild(root, NULL, BAD_CAST "lockdiscovery", NULL);
    xml_add_lockdisc(root, txn->req_tgt.path, (struct dav_data *) ddata);

    lparams->davdb.write_resourceLOCKONLY(davdb, ddata);

    txn->resp_body.lock = ddata->lock_token;

    if (!ddata->rowid) {
        ret = HTTP_CREATED;

        /* Tell client about the new resource */
        txn->resp_body.etag = NULL_ETAG;

        /* Tell client where to find the new resource */
        txn->location = txn->req_tgt.path;
    }
    else ret = HTTP_OK;

    xml_response(ret, txn, outdoc);
    ret = 0;

  done:
    if (davdb) {
        /* XXX - error handling/abort */
        lparams->davdb.commit_transaction(davdb);
        lparams->davdb.close_db(davdb);
    }
    mailbox_close(&mailbox);
    if (outdoc) xmlFreeDoc(outdoc);
    if (indoc) xmlFreeDoc(indoc);
    if (owner) xmlFree(owner);

    return ret;
}


/* Perform a MKCOL/MKCALENDAR request */
/*
 * preconditions:
 *   DAV:resource-must-be-null
 *   DAV:need-privileges
 *   DAV:valid-resourcetype
 *   CALDAV:calendar-collection-location-ok
 *   CALDAV:valid-calendar-data (CALDAV:calendar-timezone)
 */
int meth_mkcol(struct transaction_t *txn, void *params)
{
    struct meth_params *mparams = (struct meth_params *) params;
    int ret = 0, r = 0;
    xmlDocPtr indoc = NULL, outdoc = NULL;
    xmlNodePtr root = NULL, instr = NULL;
    xmlNsPtr ns[NUM_NAMESPACE];
    char *partition = NULL;
    struct proppatch_ctx pctx;
    struct mailbox *mailbox = NULL;

    memset(&pctx, 0, sizeof(struct proppatch_ctx));

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path (use our own entry to suppress lookup) */
    txn->req_tgt.mbentry = mboxlist_entry_create();
    if ((r = mparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) {
        txn->error.precond = CALDAV_LOCATION_OK;
        return HTTP_FORBIDDEN;
    }

    /* Make sure method is allowed (only allowed on home-set) */
    if (!(txn->req_tgt.allow & ALLOW_WRITECOL)) {
        txn->error.precond = CALDAV_LOCATION_OK;
        return HTTP_FORBIDDEN;
    }

    if (config_mupdate_server && !config_getstring(IMAPOPT_PROXYSERVERS)) {
        /* Remote mailbox - find the parent */
        mbentry_t *parent = NULL;
        struct backend *be;

        r = mboxlist_findparent(txn->req_tgt.mbentry->name, &parent);
        if (r) {
            txn->error.precond = CALDAV_LOCATION_OK;
            ret = HTTP_FORBIDDEN;
            goto done;
	}

        be = proxy_findserver(parent->server, &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);

        if (!be) ret = HTTP_UNAVAILABLE;
        else ret = http_pipe_req_resp(be, txn);

        goto done;
    }

    /* Local Mailbox */

    /* Parse the MKCOL/MKCALENDAR body, if exists */
    ret = parse_xml_body(txn, &root);
    if (ret) goto done;

    if (root) {
        /* Check for correct root element (lowercase method name ) */
        indoc = root->doc;

        buf_setcstr(&txn->buf, http_methods[txn->meth].name);
        r = xmlStrcmp(root->name, BAD_CAST buf_lcase(&txn->buf));
        if (r) {
            txn->error.desc = "Incorrect root element in XML request\r\n";
            ret = HTTP_BAD_MEDIATYPE;
            goto done;
        }

        instr = root->children;
    }

    /* Create the mailbox */
    r = mboxlist_createmailbox(txn->req_tgt.mbentry->name,
                               mparams->mkcol_mbtype, partition,
                               httpd_userisadmin || httpd_userisproxyadmin,
                               httpd_userid, httpd_authstate,
                               /*localonly*/0, /*forceuser*/0,
                               /*dbonly*/0, /*notify*/0,
                               &mailbox);

    if (instr && !r) {
        /* Start construction of our mkcol/mkcalendar response */
        buf_appendcstr(&txn->buf, "-response");
        root = init_xml_response(buf_cstring(&txn->buf), NS_REQ_ROOT, root, ns);
        if (!root) {
            ret = HTTP_SERVER_ERROR;
            txn->error.desc = "Unable to create XML response\r\n";
            goto done;
        }

        outdoc = root->doc;

        /* Populate our proppatch context */
        pctx.req_tgt = &txn->req_tgt;
        pctx.meth = txn->meth;
        pctx.mailbox = mailbox;
        pctx.lprops = mparams->lprops;
        pctx.root = root;
        pctx.ns = ns;
        pctx.tid = NULL;
        pctx.err = &txn->error;
        pctx.ret = &r;

        /* Execute the property patch instructions */
        ret = do_proppatch(&pctx, instr);

        if (ret || r) {
            /* Setting properties failed - delete mailbox */
            mailbox_close(&mailbox);
            mboxlist_deletemailbox(txn->req_tgt.mbentry->name,
                                   /*isadmin*/1, NULL, NULL, NULL,
                                   /*checkacl*/0, /*localonly*/0, /*force*/1);

            if (!ret) {
                /* Output the XML response */
                xml_response(r, txn, outdoc);
            }

            goto done;
        }
    }

    if (!r) ret = HTTP_CREATED;
    else if (r == IMAP_PERMISSION_DENIED) ret = HTTP_NO_PRIVS;
    else if (r == IMAP_MAILBOX_EXISTS) {
        txn->error.precond = DAV_RSRC_EXISTS;
        ret = HTTP_FORBIDDEN;
    }
    else {
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
    }

  done:
    buf_free(&pctx.buf);
    mailbox_close(&mailbox);

    if (partition) free(partition);
    if (outdoc) xmlFreeDoc(outdoc);
    if (indoc) xmlFreeDoc(indoc);

    return ret;
}


/* dav_foreach() callback to find props on a resource */
int propfind_by_resource(void *rock, void *data)
{
    struct propfind_ctx *fctx = (struct propfind_ctx *) rock;
    struct dav_data *ddata = (struct dav_data *) data;
    struct index_record record;
    char *p;
    size_t len;
    int r, ret = 0;

    /* Append resource name to URL path */
    if (!fctx->req_tgt->resource) {
        len = strlen(fctx->req_tgt->path);
        p = fctx->req_tgt->path + len;
    }
    else {
        p = fctx->req_tgt->resource;
        len = p - fctx->req_tgt->path;
    }

    if (p[-1] != '/') {
        *p++ = '/';
        len++;
    }
    strlcpy(p, ddata->resource, MAX_MAILBOX_PATH - len);
    fctx->req_tgt->resource = p;
    fctx->req_tgt->reslen = strlen(p);

    fctx->data = data;
    if (ddata->imap_uid && !fctx->record) {
        /* Fetch index record for the resource */
        r = mailbox_find_index_record(fctx->mailbox, ddata->imap_uid, &record);
        /* XXX  Check errors */

        fctx->record = r ? NULL : &record;
    }

    if (!ddata->imap_uid || !fctx->record) {
        /* Add response for missing target */
        ret = xml_add_response(fctx, HTTP_NOT_FOUND, 0);
    }
    else if (!fctx->filter || fctx->filter(fctx, data)) {
        /* Add response for target */
        ret = xml_add_response(fctx, 0, 0);
    }

    buf_free(&fctx->msg_buf);
    fctx->record = NULL;
    fctx->data = NULL;

    return ret;
}


/* mboxlist_findall() callback to find props on a collection */
int propfind_by_collection(const char *mboxname, int matchlen,
                           int maycreate __attribute__((unused)),
                           void *rock)
{
    struct propfind_ctx *fctx = (struct propfind_ctx *) rock;
    mbentry_t *mbentry = NULL;
    struct buf writebuf = BUF_INITIALIZER;
    struct mboxname_parts parts;
    struct mailbox *mailbox = NULL;
    char *p;
    size_t len;
    int r = 0, rights, root = 1;

    mboxname_init_parts(&parts);

    /* If this function is called outside of mboxlist_findall()
     * with matchlen == 0, this is the root resource of the PROPFIND,
     * otherwise it's just one of many found.  Inbox and Outbox can't
     * appear unless they are the root */
    if (matchlen) {
        p = strrchr(mboxname, '.');
        if (!p) goto done;
        p++; /* skip dot */
        if (!strncmp(p, SCHED_INBOX, strlen(SCHED_INBOX) - 1)) goto done;
        if (!strncmp(p, SCHED_OUTBOX, strlen(SCHED_OUTBOX) - 1)) goto done;
        /* and while we're at it, reject the fricking top-level folders too.
         * XXX - this is evil and bad and wrong */
        if (*p == '#') goto done;
        root = 0;
    }

    /* Check ACL on mailbox for current user */
    r = mboxlist_lookup(mboxname, &mbentry, NULL);
    if (r == IMAP_MAILBOX_NONEXISTENT) return 0;
    if (r) {
        syslog(LOG_INFO, "mboxlist_lookup(%s) failed: %s",
               mboxname, error_message(r));
        fctx->err->desc = error_message(r);
        *fctx->ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* if finding all, we only match known types */
    if (matchlen && !(mbentry->mbtype & fctx->req_tgt->mboxtype))
        goto done;

    rights = httpd_myrights(httpd_authstate, mbentry->acl);
    if ((rights & fctx->reqd_privs) != fctx->reqd_privs) goto done;

    /* Open mailbox for reading */
    if ((r = mailbox_open_irl(mboxname, &mailbox))) {
        syslog(LOG_INFO, "mailbox_open_irl(%s) failed: %s",
               mboxname, error_message(r));
        fctx->err->desc = error_message(r);
        *fctx->ret = HTTP_SERVER_ERROR;
        goto done;
    }

    fctx->mailbox = mailbox;
    fctx->record = NULL;

    if (!fctx->req_tgt->resource) {
        mboxname_to_parts(mboxname, &parts);

        buf_setcstr(&writebuf, fctx->req_tgt->prefix);

        if (parts.userid) {
            const char *domain =
                parts.domain ? parts.domain :
                httpd_extradomain ? httpd_extradomain : config_defdomain;

            buf_printf(&writebuf, "/user/%s", parts.userid);
            if (domain) buf_printf(&writebuf, "@%s", domain);
        }
        buf_putc(&writebuf, '/');

        len = writebuf.len;

        /* one day this will just be the final element of 'boxes' hopefully */
        if (!parts.box) goto done;
        p = strrchr(parts.box, '.');
        if (!p) goto done;

        /* OK, we're doing this mailbox */
        buf_appendcstr(&writebuf, p+1);

        /* don't forget the trailing slash */
        buf_putc(&writebuf, '/');

        /* copy it all back into place... in theory we should check against
         * 'last' and make sure it doesn't change from the original request.
         * yay for micro-optimised memory usage... */
        strlcpy(fctx->req_tgt->path, buf_cstring(&writebuf), MAX_MAILBOX_PATH);
        p = fctx->req_tgt->path + len;
        fctx->req_tgt->collection = p;
        fctx->req_tgt->collen = strlen(p);

        /* If not filtering by calendar resource, and not excluding root,
           add response for collection */
        if (!fctx->filter_crit &&
            (!root || (fctx->depth == 1) || !(fctx->prefer & PREFER_NOROOT)) &&
            (r = xml_add_response(fctx, 0, 0))) goto done;
    }

    if (fctx->depth > 1 && fctx->open_db) { // can't do davdb searches if no dav db
        /* Resource(s) */

        /* Open the DAV DB corresponding to the mailbox.
         *
         * Note we open the new one first before closing the old one, so we
         * get refcounted retaining of the open database within a single user */
        sqlite3 *newdb = fctx->open_db(mailbox);
        if (fctx->davdb) fctx->close_db(fctx->davdb);
        fctx->davdb = newdb;

        if (fctx->req_tgt->resource) {
            /* Add response for target resource */
            void *data;

            /* Find message UID for the resource */
            fctx->lookup_resource(fctx->davdb,
                                  mboxname, fctx->req_tgt->resource, &data, 0);
            /* XXX  Check errors */

            r = fctx->proc_by_resource(rock, data);
        }
        else {
            /* Add responses for all contained resources */
            fctx->foreach_resource(fctx->davdb, mboxname,
                                   fctx->proc_by_resource, rock);

            /* Started with NULL resource, end with NULL resource */
            fctx->req_tgt->resource = NULL;
            fctx->req_tgt->reslen = 0;
        }
    }

  done:
    buf_free(&writebuf);
    mboxname_free_parts(&parts);
    mboxlist_entry_free(&mbentry);
    if (mailbox) mailbox_close(&mailbox);

    return r;
}


/* Perform a PROPFIND request */
EXPORTED int meth_propfind(struct transaction_t *txn, void *params)
{
    struct meth_params *fparams = (struct meth_params *) params;
    int ret = 0, r;
    const char **hdr;
    unsigned depth;
    xmlDocPtr indoc = NULL, outdoc = NULL;
    xmlNodePtr root, cur = NULL, props = NULL;
    xmlNsPtr ns[NUM_NAMESPACE];
    struct hash_table ns_table = { 0, NULL, NULL };
    struct propfind_ctx fctx;
    struct propfind_entry_list *elist = NULL;

    memset(&fctx, 0, sizeof(struct propfind_ctx));

    /* Parse the path */
    if (fparams->parse_path) {
        r = fparams->parse_path(txn->req_uri->path,
                                &txn->req_tgt, &txn->error.desc);
        if (r) return r;
    }

    /* Make sure method is allowed */
    if (!(txn->req_tgt.allow & ALLOW_DAV)) return HTTP_NOT_ALLOWED;

    /* Check Depth */
    hdr = spool_getheader(txn->req_hdrs, "Depth");
    if (!hdr || !strcmp(hdr[0], "infinity")) {
        depth = 3;
    }
    else if (!strcmp(hdr[0], "1")) {
        depth = 1;
    }
    else if (!strcmp(hdr[0], "0")) {
        depth = 0;
    }
    else {
        txn->error.desc = "Illegal Depth value\r\n";
        return HTTP_BAD_REQUEST;
    }

    if (txn->req_tgt.mbentry) {
        int rights;

        /* Check ACL for current user */
        rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
        if ((rights & DACL_READ) != DACL_READ) {
            /* DAV:need-privileges */
            txn->error.precond = DAV_NEED_PRIVS;
            txn->error.resource = txn->req_tgt.path;
            txn->error.rights = DACL_READ;
            ret = HTTP_NO_PRIVS;
            goto done;
        }

        if (txn->req_tgt.mbentry->server) {
            /* Remote mailbox */
            struct backend *be;

            be = proxy_findserver(txn->req_tgt.mbentry->server,
                                  &http_protocol, proxy_userid,
                                  &backend_cached, NULL, NULL, httpd_in);
            if (!be) return HTTP_UNAVAILABLE;

            return http_pipe_req_resp(be, txn);
        }

        /* Local Mailbox */
    }

    /* Principal or Local Mailbox */

    /* Normalize depth so that:
     * 0 = home-set, 1+ = collection, 2+ = resource, 3+ = infinity!
     */
    if (txn->req_tgt.collection) depth++;
    if (txn->req_tgt.resource) depth++;

    /* Parse the PROPFIND body, if exists */
    ret = parse_xml_body(txn, &root);
    if (ret) goto done;

    if (!root) {
        /* Empty request */
        fctx.mode = PROPFIND_ALL;
    }
    else {
        indoc = root->doc;

        /* Make sure its a propfind element */
        if (xmlStrcmp(root->name, BAD_CAST "propfind")) {
            txn->error.desc = "Missing propfind element in PROPFIND request";
            ret = HTTP_BAD_REQUEST;
            goto done;
        }

        /* Find child element of propfind */
        for (cur = root->children;
             cur && cur->type != XML_ELEMENT_NODE; cur = cur->next);

        if (!cur) {
            txn->error.desc = "Missing child node element in PROPFIND request";
            ret = HTTP_BAD_REQUEST;
            goto done;
        }

        /* Add propfind type to our header cache */
        spool_cache_header(xstrdup(":type"), xstrdup((const char *) cur->name),
                           txn->req_hdrs);

        /* Make sure its a known element */
        if (!xmlStrcmp(cur->name, BAD_CAST "allprop")) {
            fctx.mode = PROPFIND_ALL;
        }
        else if (!xmlStrcmp(cur->name, BAD_CAST "propname")) {
            fctx.mode = PROPFIND_NAME;
            fctx.prefer = PREFER_MIN;  /* Don't want 404 (Not Found) */
        }
        else if (!xmlStrcmp(cur->name, BAD_CAST "prop")) {
            fctx.mode = PROPFIND_PROP;
            props = cur->children;
        }
        else {
            ret = HTTP_BAD_REQUEST;
            goto done;
        }

        /* Check for extra elements */
        for (cur = cur->next; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE) {
                if ((fctx.mode == PROPFIND_ALL) && !props &&
                    /* Check for 'include' element */
                    !xmlStrcmp(cur->name, BAD_CAST "include")) {
                    props = cur->children;
                }
                else {
                    ret = HTTP_BAD_REQUEST;
                    goto done;
                }
            }
        }
    }

    /* Start construction of our multistatus response */
    root = init_xml_response("multistatus", NS_DAV, root, ns);
    if (!root) {
        ret = HTTP_SERVER_ERROR;
        txn->error.desc = "Unable to create XML response";
        goto done;
    }

    outdoc = root->doc;

    /* Populate our propfind context */
    fctx.req_tgt = &txn->req_tgt;
    fctx.depth = depth;
    fctx.prefer |= get_preferences(txn);
    fctx.req_hdrs = txn->req_hdrs;
    fctx.userid = proxy_userid;
    fctx.userisadmin = httpd_userisadmin;
    fctx.authstate = httpd_authstate;
    fctx.mailbox = NULL;
    fctx.record = NULL;
    fctx.reqd_privs = DACL_READ;
    fctx.filter = NULL;
    fctx.filter_crit = NULL;
    fctx.open_db = fparams->davdb.open_db;
    fctx.close_db = fparams->davdb.close_db;
    fctx.lookup_resource = fparams->davdb.lookup_resource;
    fctx.foreach_resource = fparams->davdb.foreach_resource;
    fctx.proc_by_resource = &propfind_by_resource;
    fctx.elist = NULL;
    fctx.lprops = fparams->lprops;
    fctx.root = root;
    fctx.ns = ns;
    fctx.ns_table = &ns_table;
    fctx.err = &txn->error;
    fctx.ret = &ret;
    fctx.fetcheddata = 0;

    /* Parse the list of properties and build a list of callbacks */
    preload_proplist(props, &fctx);

    if (!txn->req_tgt.collection &&
        (!depth || !(fctx.prefer & PREFER_NOROOT))) {
        /* Add response for principal or home-set collection */
        if (txn->req_tgt.mbentry) {
            /* Open mailbox for reading */
            if ((r = mailbox_open_irl(txn->req_tgt.mbentry->name, &fctx.mailbox))) {
                syslog(LOG_INFO, "mailbox_open_irl(%s) failed: %s",
                       txn->req_tgt.mbentry->name, error_message(r));
                txn->error.desc = error_message(r);
                ret = HTTP_SERVER_ERROR;
                goto done;
            }
        }

        xml_add_response(&fctx, 0, 0);

        mailbox_close(&fctx.mailbox);
    }

    if (depth > 0) {
        /* Calendar collection(s) */

        if (txn->req_tgt.collection) {
            /* Add response for target calendar collection */
            propfind_by_collection(txn->req_tgt.mbentry->name, 0, 0, &fctx);
        }
        else {
            /* Add responses for all calendar collections */
            int isadmin = httpd_userisadmin||httpd_userisproxyadmin;
            r = mboxlist_findall(&httpd_namespace, "*", isadmin, httpd_userid,
                                 httpd_authstate, propfind_by_collection, &fctx);
        }

        if (fctx.davdb) fctx.close_db(fctx.davdb);

        ret = *fctx.ret;
    }

    /* Output the XML response */
    if (!ret) {
        /* iCalendar data in response should not be transformed */
        if (fctx.fetcheddata) txn->flags.cc |= CC_NOTRANSFORM;

        xml_response(HTTP_MULTI_STATUS, txn, outdoc);
    }

  done:
    /* Free the entry list */
    elist = fctx.elist;
    while (elist) {
        struct propfind_entry_list *freeme = elist;
        elist = elist->next;
        xmlFree(freeme->name);
        free(freeme);
    }

    buf_free(&fctx.buf);

    free_hash_table(&ns_table, NULL);

    if (outdoc) xmlFreeDoc(outdoc);
    if (indoc) xmlFreeDoc(indoc);

    return ret;
}


/* Perform a PROPPATCH request
 *
 * preconditions:
 *   DAV:cannot-modify-protected-property
 *   CALDAV:valid-calendar-data (CALDAV:calendar-timezone)
 */
int meth_proppatch(struct transaction_t *txn, void *params)
{
    struct meth_params *pparams = (struct meth_params *) params;
    int ret = 0, r = 0, rights;
    xmlDocPtr indoc = NULL, outdoc = NULL;
    xmlNodePtr root, instr, resp;
    xmlNsPtr ns[NUM_NAMESPACE];
    struct mailbox *mailbox = NULL;
    struct proppatch_ctx pctx;
    struct index_record record;
    void *davdb = NULL;

    memset(&pctx, 0, sizeof(struct proppatch_ctx));

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    if ((r = pparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    if (!txn->req_tgt.collection && !txn->req_tgt.userid) {
        txn->error.desc = "PROPPATCH requires a collection";
        return HTTP_NOT_ALLOWED;
    }

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
    if (!(rights & DACL_WRITEPROPS)) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights = DACL_WRITEPROPS;
        return HTTP_NO_PRIVS;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    r = mailbox_open_iwl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "IOERROR: failed to open mailbox %s for proppatch",
               txn->req_tgt.mbentry->name);
        return HTTP_SERVER_ERROR;
    }

    /* Parse the PROPPATCH body */
    ret = parse_xml_body(txn, &root);
    if (!ret && !root) {
        txn->error.desc = "Missing request body\r\n";
        ret = HTTP_BAD_REQUEST;
    }
    if (ret) goto done;

    indoc = root->doc;

    /* Make sure its a propertyupdate element */
    if (xmlStrcmp(root->name, BAD_CAST "propertyupdate")) {
        txn->error.desc =
            "Missing propertyupdate element in PROPPATCH request\r\n";
        ret = HTTP_BAD_REQUEST;
        goto done;
    }
    instr = root->children;

    /* Start construction of our multistatus response */
    if (!(root = init_xml_response("multistatus", NS_DAV, root, ns))) {
        txn->error.desc = "Unable to create XML response\r\n";
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    outdoc = root->doc;

    /* Add a response tree to 'root' for the specified href */
    resp = xmlNewChild(root, NULL, BAD_CAST "response", NULL);
    if (!resp) syslog(LOG_ERR, "new child response failed");
    xmlNewChild(resp, NULL, BAD_CAST "href", BAD_CAST txn->req_tgt.path);

    /* Populate our proppatch context */
    pctx.req_tgt = &txn->req_tgt;
    pctx.meth = txn->meth;
    pctx.mailbox = mailbox;
    pctx.record = NULL;
    pctx.lprops = pparams->lprops;
    pctx.root = resp;
    pctx.ns = ns;
    pctx.tid = NULL;
    pctx.err = &txn->error;
    pctx.ret = &r;

    if (txn->req_tgt.resource) {
        struct dav_data *ddata;
        /* gotta find the resource */
        /* Open the DAV DB corresponding to the mailbox */
        davdb = pparams->davdb.open_db(mailbox);

        /* Find message UID for the resource */
        pparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                       txn->req_tgt.resource, (void **) &ddata, 0);
        if (!ddata->imap_uid) {
            ret = HTTP_NOT_FOUND;
            goto done;
        }

        memset(&record, 0, sizeof(struct index_record));
        /* Mapped URL - Fetch index record for the resource */
        r = mailbox_find_index_record(mailbox, ddata->imap_uid, &record);
        if (r) {
            ret = HTTP_NOT_FOUND;
            goto done;
        }

        pctx.record = &record;
    }

    /* Execute the property patch instructions */
    ret = do_proppatch(&pctx, instr);

    /* Output the XML response */
    if (!ret) {
        if (get_preferences(txn) & PREFER_MIN) ret = HTTP_OK;
        else xml_response(HTTP_MULTI_STATUS, txn, outdoc);
    }

  done:
    if (davdb) pparams->davdb.close_db(davdb);
    mailbox_close(&mailbox);
    buf_free(&pctx.buf);

    if (outdoc) xmlFreeDoc(outdoc);
    if (indoc) xmlFreeDoc(indoc);

    return ret;
}


/* Perform a POST request */
int meth_post(struct transaction_t *txn, void *params)
{
    struct meth_params *pparams = (struct meth_params *) params;
    static unsigned post_count = 0;
    struct strlist *action;
    int r, ret;
    size_t len;

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    if ((r = pparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    /* Make sure method is allowed (only allowed on certain collections) */
    if (!(txn->req_tgt.allow & ALLOW_POST)) return HTTP_NOT_ALLOWED;

    /* Do any special processing */
    if (pparams->post) {
        ret = pparams->post(txn);
        if (ret != HTTP_CONTINUE) return ret;
    }

    action = hash_lookup("action", &txn->req_qparams);
    if (!action || action->next || strcmp(action->s, "add-member"))
        return HTTP_FORBIDDEN;

    /* POST add-member to regular collection */

    /* Append a unique resource name to URL path and perform a PUT */
    len = strlen(txn->req_tgt.path);
    txn->req_tgt.resource = txn->req_tgt.path + len;
    txn->req_tgt.reslen =
        snprintf(txn->req_tgt.resource, MAX_MAILBOX_PATH - len,
                 "%x-%d-%ld-%u.ics",
                 strhash(txn->req_tgt.path), getpid(), time(0), post_count++);

    /* Tell client where to find the new resource */
    txn->location = txn->req_tgt.path;

    ret = meth_put(txn, params);

    if (ret != HTTP_CREATED) txn->location = NULL;

    return ret;
}


/* Perform a PUT request
 *
 * preconditions:
 *   *DAV:supported-address-data
 */
int meth_put(struct transaction_t *txn, void *params)
{
    struct meth_params *pparams = (struct meth_params *) params;
    int ret, r, precond, rights;
    const char **hdr, *etag;
    struct mime_type_t *mime = NULL;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record oldrecord;
    quota_t qdiffs[QUOTA_NUMRESOURCES] = QUOTA_DIFFS_INITIALIZER;
    time_t lastmod;
    unsigned flags = 0;
    void *davdb = NULL, *obj = NULL;
    struct buf msg_buf = BUF_INITIALIZER;

    if (txn->meth == METH_PUT) {
        /* Response should not be cached */
        txn->flags.cc |= CC_NOCACHE;

        /* Parse the path */
        if ((r = pparams->parse_path(txn->req_uri->path,
                                     &txn->req_tgt, &txn->error.desc))) {
            return HTTP_FORBIDDEN;
        }

        /* Make sure method is allowed (only allowed on resources) */
        if (!(txn->req_tgt.allow & ALLOW_WRITE)) return HTTP_NOT_ALLOWED;
    }

    /* Make sure Content-Range isn't specified */
    if (spool_getheader(txn->req_hdrs, "Content-Range"))
        return HTTP_BAD_REQUEST;

    /* Check Content-Type */
    if ((hdr = spool_getheader(txn->req_hdrs, "Content-Type"))) {
        for (mime = pparams->mime_types; mime->content_type; mime++) {
            if (is_mediatype(mime->content_type, hdr[0])) break;
        }
    }
    if (!mime || !mime->content_type) {
        txn->error.precond = pparams->put.supp_data_precond;
        return HTTP_FORBIDDEN;
    }

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
    if (!(rights & DACL_WRITECONT) || !(rights & DACL_ADDRSRC)) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights =
            !(rights & DACL_WRITECONT) ? DACL_WRITECONT : DACL_ADDRSRC;
        return HTTP_NO_PRIVS;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Read body */
    txn->req_body.flags |= BODY_DECODE;
    ret = http_read_body(httpd_in, httpd_out,
                         txn->req_hdrs, &txn->req_body, &txn->error.desc);
    if (ret) {
        txn->flags.conn = CONN_CLOSE;
        return ret;
    }

    /* Make sure we have a body */
    qdiffs[QUOTA_STORAGE] = buf_len(&txn->req_body.payload);
    if (!qdiffs[QUOTA_STORAGE]) {
        txn->error.desc = "Missing request body\r\n";
        return HTTP_BAD_REQUEST;
    }

    /* Check if we can append a new message to mailbox */
    if ((r = append_check(txn->req_tgt.mbentry->name,
                          httpd_authstate, ACL_INSERT, ignorequota ? NULL : qdiffs))) {
        syslog(LOG_ERR, "append_check(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        return HTTP_SERVER_ERROR;
    }

    /* Open mailbox for writing */
    r = mailbox_open_iwl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        return HTTP_SERVER_ERROR;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = pparams->davdb.open_db(mailbox);

    /* Find message UID for the resource, if exists */
    pparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void *) &ddata, 0);
    /* XXX  Check errors */

    if (ddata->imap_uid) {
        /* Overwriting existing resource */

        /* Fetch index record for the resource */
        r = mailbox_find_index_record(mailbox, ddata->imap_uid, &oldrecord);
        if (r) {
            syslog(LOG_ERR, "mailbox_find_index_record(%s, %u) failed: %s",
                   txn->req_tgt.mbentry->name, ddata->imap_uid, error_message(r));
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        etag = message_guid_encode(&oldrecord.guid);
        lastmod = oldrecord.internaldate;
    }
    else if (ddata->rowid) {
        /* Unmapped URL (empty resource) */
        etag = NULL_ETAG;
        lastmod = ddata->creationdate;
    }
    else {
        /* New resource */
        etag = NULL;
        lastmod = 0;
    }

    /* Check any preferences */
    if (get_preferences(txn) & PREFER_REP) flags |= PREFER_REP;

    /* Check any preconditions */
    ret = precond = pparams->check_precond(txn, ddata, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
        /* Parse, validate, and store the resource */
        obj = mime->from_string(buf_cstring(&txn->req_body.payload));
        ret = pparams->put.proc(txn, obj, mailbox, txn->req_tgt.resource, davdb);
        break;

    case HTTP_PRECOND_FAILED:
        if (flags & PREFER_REP) {
            unsigned offset;

            /* Load message containing the resource */
            mailbox_map_record(mailbox, &oldrecord, &msg_buf);

            /* Resource length doesn't include RFC 5322 header */
            offset = oldrecord.header_size;

            /* Parse existing resource */
            obj =
                pparams->mime_types[0].from_string(buf_base(&msg_buf) + offset);

            /* Fill in ETag and Last-Modified */
            txn->resp_body.etag = etag;
            txn->resp_body.lastmod = lastmod;
        }
        break;

    case HTTP_LOCKED:
        txn->error.precond = DAV_NEED_LOCK_TOKEN;
        txn->error.resource = txn->req_tgt.path;

    default:
        /* We failed a precondition */
        goto done;
    }

    if (flags & PREFER_REP) {
        struct resp_body_t *resp_body = &txn->resp_body;
        const char **hdr;
        char *data;
        unsigned long datalen;

        if ((hdr = spool_getheader(txn->req_hdrs, "Accept"))) {
            mime = get_accept_type(hdr, pparams->mime_types);
            if (!mime) goto done;
        }

        switch (ret) {
        case HTTP_NO_CONTENT:
            ret = HTTP_OK;

        case HTTP_CREATED:
        case HTTP_PRECOND_FAILED:
            /* Convert into requested MIME type */
            data = mime->to_string(obj);
            datalen = strlen(data);

            /* Fill in Content-Type, Content-Length */
            resp_body->type = mime->content_type;
            resp_body->len = datalen;

            /* Fill in Content-Location */
            resp_body->loc = txn->req_tgt.path;

            /* Fill in Expires and Cache-Control */
            resp_body->maxage = 3600;   /* 1 hr */
            txn->flags.cc = CC_MAXAGE
                | CC_REVALIDATE         /* don't use stale data */
                | CC_NOTRANSFORM;       /* don't alter iCal data */

            /* Output current representation */
            write_body(ret, txn, data, datalen);

            free(data);
            ret = 0;
            break;

        default:
            /* failure - do nothing */
            break;
        }
    }

  done:
    if (obj) {
        pparams->mime_types[0].free(obj);
        buf_free(&msg_buf);
    }
    if (davdb) pparams->davdb.close_db(davdb);
    mailbox_close(&mailbox);

    return ret;
}


/* Compare modseq in index maps -- used for sorting */
static int map_modseq_cmp(const struct index_map *m1,
                          const struct index_map *m2)
{
    if (m1->modseq < m2->modseq) return -1;
    if (m1->modseq > m2->modseq) return 1;
    return 0;
}


/* CALDAV:calendar-multiget/CARDDAV:addressbook-multiget REPORT */
int report_multiget(struct transaction_t *txn, struct meth_params *rparams,
                    xmlNodePtr inroot, struct propfind_ctx *fctx)
{
    int r, ret = 0;
    struct mailbox *mailbox = NULL;
    xmlNodePtr node;
    struct buf uri = BUF_INITIALIZER;

    /* Get props for each href */
    for (node = inroot->children; node; node = node->next) {
        if ((node->type == XML_ELEMENT_NODE) &&
            !xmlStrcmp(node->name, BAD_CAST "href")) {
            xmlChar *href = xmlNodeListGetString(inroot->doc, node->children, 1);
            int len = xmlStrlen(href);
            struct request_target_t tgt;
            struct dav_data *ddata;

            buf_ensure(&uri, len+1);
            xmlURIUnescapeString((const char *) href, len, uri.s);
            uri.len = strlen(uri.s);
            xmlFree(href);

            /* Parse the path */
            memset(&tgt, 0, sizeof(struct request_target_t));
            tgt.namespace = txn->req_tgt.namespace;

            if ((r = rparams->parse_path(buf_cstring(&uri), &tgt, &fctx->err->desc))) {
                ret = r;
                goto done;
            }

            fctx->req_tgt = &tgt;

            /* Check if we already have this mailbox open */
            if (!mailbox || strcmp(mailbox->name, tgt.mbentry->name)) {
                if (mailbox) mailbox_close(&mailbox);

                /* Open mailbox for reading */
                r = mailbox_open_irl(tgt.mbentry->name, &mailbox);
                if (r && r != IMAP_MAILBOX_NONEXISTENT) {
                    syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
                           tgt.mbentry->name, error_message(r));
                    txn->error.desc = error_message(r);
                    ret = HTTP_SERVER_ERROR;
                    goto done;
                }

                fctx->mailbox = mailbox;
            }

            if (!fctx->mailbox || !tgt.resource) {
                /* Add response for missing target */
                xml_add_response(fctx, HTTP_NOT_FOUND, 0);
                continue;
            }

            /* Open the DAV DB corresponding to the mailbox */
            fctx->davdb = rparams->davdb.open_db(fctx->mailbox);

            /* Find message UID for the resource */
            rparams->davdb.lookup_resource(fctx->davdb, tgt.mbentry->name,
                                           tgt.resource, (void **) &ddata, 0);
            ddata->resource = tgt.resource;
            /* XXX  Check errors */

            fctx->proc_by_resource(fctx, ddata);

            /* XXX - split this into a req_tgt cleanup */
            free(tgt.userid);
            mboxlist_entry_free(&tgt.mbentry);

            rparams->davdb.close_db(fctx->davdb);
        }
    }

  done:
    mailbox_close(&mailbox);
    buf_free(&uri);

    return (ret ? ret : HTTP_MULTI_STATUS);
}


/* DAV:sync-collection REPORT */
int report_sync_col(struct transaction_t *txn,
                    struct meth_params *rparams __attribute__((unused)),
                    xmlNodePtr inroot, struct propfind_ctx *fctx)
{
    int ret = 0, r, i, unbind_flag = -1, unchanged_flag = -1;
    struct mailbox *mailbox = NULL;
    uint32_t uidvalidity = 0;
    modseq_t syncmodseq = 0;
    modseq_t basemodseq = 0;
    modseq_t highestmodseq = 0;
    modseq_t respmodseq = 0;
    uint32_t limit = -1;
    uint32_t msgno;
    uint32_t nresp = 0;
    xmlNodePtr node;
    struct index_state istate;
    const struct index_record *record;
    char tokenuri[MAX_MAILBOX_PATH+1];

    /* XXX  Handle Depth (cal-home-set at toplevel) */

    memset(&istate, 0, sizeof(struct index_state));
    istate.map = NULL;

    /* Open mailbox for reading */
    r = mailbox_open_irl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    fctx->mailbox = mailbox;

    highestmodseq = mailbox->i.highestmodseq;
    mailbox_user_flag(mailbox, DFLAG_UNBIND, &unbind_flag, 1);
    mailbox_user_flag(mailbox, DFLAG_UNCHANGED, &unchanged_flag, 1);

    /* Parse children element of report */
    for (node = inroot->children; node; node = node->next) {
        xmlNodePtr node2;
        xmlChar *str = NULL;
        if (node->type == XML_ELEMENT_NODE) {
            if (!xmlStrcmp(node->name, BAD_CAST "sync-token") &&
                (str = xmlNodeListGetString(inroot->doc, node->children, 1))) {
                /* Parse sync-token */
                r = sscanf((char *) str, SYNC_TOKEN_URL_SCHEME
                           "%u-" MODSEQ_FMT "-" MODSEQ_FMT "%1s",
                           &uidvalidity, &syncmodseq, &basemodseq,
                           tokenuri /* test for trailing junk */);

                syslog(LOG_ERR, "scanned token %s to %d %u %llu %llu",
                       str, r, uidvalidity, syncmodseq, basemodseq);
                /* Sanity check the token components */
                if (r < 2 || r > 3 ||
                    (uidvalidity != mailbox->i.uidvalidity) ||
                    (syncmodseq > highestmodseq)) {
                    fctx->err->desc = "Invalid sync-token";
                }
                else if (r == 3) {
                    /* Previous partial read token */
                    if (basemodseq > highestmodseq) {
                        fctx->err->desc = "Invalid sync-token";
                    }
                    else if (basemodseq < mailbox->i.deletedmodseq) {
                        fctx->err->desc = "Stale sync-token";
                    }
                }
                else {
                    /* Regular token */
                    if (syncmodseq < mailbox->i.deletedmodseq) {
                        fctx->err->desc = "Stale sync-token";
                    }
                }

                if (fctx->err->desc) {
                    /* DAV:valid-sync-token */
                    txn->error.precond = DAV_SYNC_TOKEN;
                    ret = HTTP_FORBIDDEN;
                }
            }
            else if (!xmlStrcmp(node->name, BAD_CAST "sync-level") &&
                (str = xmlNodeListGetString(inroot->doc, node->children, 1))) {
                if (!strcmp((char *) str, "infinity")) {
                    fctx->err->desc =
                        "This server DOES NOT support infinite depth requests";
                    ret = HTTP_SERVER_ERROR;
                }
                else if ((sscanf((char *) str, "%u", &fctx->depth) != 1) ||
                         (fctx->depth != 1)) {
                    fctx->err->desc = "Illegal sync-level";
                    ret = HTTP_BAD_REQUEST;
                }
            }
            else if (!xmlStrcmp(node->name, BAD_CAST "limit")) {
                for (node2 = node->children; node2; node2 = node2->next) {
                    if ((node2->type == XML_ELEMENT_NODE) &&
                        !xmlStrcmp(node2->name, BAD_CAST "nresults") &&
                        (!(str = xmlNodeListGetString(inroot->doc,
                                                      node2->children, 1)) ||
                         (sscanf((char *) str, "%u", &limit) != 1))) {
                        txn->error.precond = DAV_OVER_LIMIT;
                        ret = HTTP_FORBIDDEN;
                    }
                }
            }

            if (str) xmlFree(str);
            if (ret) goto done;
        }
    }

    /* Check Depth */
    if (!fctx->depth) {
        fctx->err->desc = "Illegal sync-level";
        ret = HTTP_BAD_REQUEST;
        goto done;
    }

    if (!syncmodseq) {
        /* Initial sync - set basemodseq in case client limits results */
        basemodseq = highestmodseq;
    }

    /* Construct array of records for sorting and/or fetching cached header */
    istate.mailbox = mailbox;
    istate.map = xzmalloc(mailbox->i.num_records *
                          sizeof(struct index_map));

    /* Find which resources we need to report */
    struct mailbox_iter *iter = mailbox_iter_init(mailbox, syncmodseq, 0);
    while ((record = mailbox_iter_step(iter))) {
        if ((unbind_flag >= 0) &&
            record->user_flags[unbind_flag / 32] & (1 << (unbind_flag & 31))) {
            /* Resource replaced by a PUT, COPY, or MOVE - ignore it */
            continue;
        }

        if ((record->modseq - syncmodseq == 1) &&
            (unchanged_flag >= 0) &&
            (record->user_flags[unchanged_flag / 32] &
             (1 << (unchanged_flag & 31)))) {
            /* Resource has just had VTIMEZONEs stripped - ignore it */
            continue;
        }

        if ((record->modseq <= basemodseq) &&
            (record->system_flags & FLAG_EXPUNGED)) {
            /* Initial sync - ignore unmapped resources */
            continue;
        }

        /* copy data into map (just like index.c - XXX helper fn? */
        istate.map[nresp].recno = record->recno;
        istate.map[nresp].uid = record->uid;
        istate.map[nresp].modseq = record->modseq;
        istate.map[nresp].system_flags = record->system_flags;
        for (i = 0; i < MAX_USER_FLAGS/32; i++)
            istate.map[nresp].user_flags[i] = record->user_flags[i];
        istate.map[nresp].cache_offset = record->cache_offset;

        nresp++;
    }
    mailbox_iter_done(&iter);

    if (limit < nresp) {
        /* Need to truncate the responses */
        struct index_map *map = istate.map;

        /* Sort the response records by modseq */
        qsort(map, nresp, sizeof(struct index_map),
              (int (*)(const void *, const void *)) &map_modseq_cmp);

        /* Our last response MUST be the last record with its modseq */
        for (nresp = limit;
             nresp && map[nresp-1].modseq == map[nresp].modseq;
             nresp--);

        if (!nresp) {
            /* DAV:number-of-matches-within-limits */
            fctx->err->desc = "Unable to truncate results";
            txn->error.precond = DAV_OVER_LIMIT;
            ret = HTTP_NO_STORAGE;
            goto done;
        }

        /* respmodseq will be modseq of last record we return */
        respmodseq = map[nresp-1].modseq;

        /* Tell client we truncated the responses */
        xml_add_response(fctx, HTTP_NO_STORAGE, DAV_OVER_LIMIT);
    }
    else {
        /* Full response - respmodseq will be highestmodseq of mailbox */
        respmodseq = highestmodseq;
    }

    /* XXX - this is crappy - re-reading the messages again */
    /* Report the resources within the client requested limit (if any) */
    struct index_record thisrecord;
    for (msgno = 1; msgno <= nresp; msgno++) {
        char *p, *resource = NULL;
        struct dav_data ddata;

        if (index_reload_record(&istate, msgno, &thisrecord))
            continue;

        /* Get resource filename from Content-Disposition header */
        if ((p = index_getheader(&istate, msgno, "Content-Disposition")) &&
            (p = strstr(p, "filename="))) {
            resource = p + 9;
        }
        if (!resource) continue;  /* No filename */

        if (*resource == '\"') {
            resource++;
            if ((p = strchr(resource, '\"'))) *p = '\0';
        }
        else if ((p = strchr(resource, ';'))) *p = '\0';

        memset(&ddata, 0, sizeof(struct dav_data));
        ddata.resource = resource;

        if (thisrecord.system_flags & FLAG_EXPUNGED) {
            /* report as NOT FOUND
               IMAP UID of 0 will cause index record to be ignored
               propfind_by_resource() will append our resource name */
            fctx->proc_by_resource(fctx, &ddata);
        }
        else {
            fctx->record = &thisrecord;
            ddata.imap_uid = thisrecord.uid;
            fctx->proc_by_resource(fctx, &ddata);
        }
    }

    /* Add sync-token element */
    if (respmodseq < basemodseq) {
        /* Client limited results of initial sync - include basemodseq */
        snprintf(tokenuri, MAX_MAILBOX_PATH,
                 SYNC_TOKEN_URL_SCHEME "%u-" MODSEQ_FMT "-" MODSEQ_FMT,
                 mailbox->i.uidvalidity, respmodseq, basemodseq);
    }
    else {
        snprintf(tokenuri, MAX_MAILBOX_PATH,
                 SYNC_TOKEN_URL_SCHEME "%u-" MODSEQ_FMT,
                 mailbox->i.uidvalidity, respmodseq);
    }
    xmlNewChild(fctx->root, NULL, BAD_CAST "sync-token", BAD_CAST tokenuri);

  done:
    if (istate.map) free(istate.map);
    mailbox_close(&mailbox);

    return (ret ? ret : HTTP_MULTI_STATUS);
}


int expand_property(xmlNodePtr inroot, struct propfind_ctx *fctx,
                    const char *href, parse_path_t parse_path,
                    const struct prop_entry *lprops,
                    xmlNodePtr root, int depth)
{
    int ret = 0, r;
    struct propfind_ctx prev_ctx;
    struct request_target_t req_tgt;

    memcpy(&prev_ctx, fctx, sizeof(struct propfind_ctx));
    memset(&req_tgt, 0, sizeof(struct request_target_t));

    fctx->mode = PROPFIND_EXPAND;
    if (href) {
        /* Parse the URL */
        parse_path(href, &req_tgt, &fctx->err->desc);

        fctx->req_tgt = &req_tgt;
    }
    fctx->lprops = lprops;
    fctx->elist = NULL;
    fctx->root = root;
    fctx->depth = depth;
    fctx->mailbox = NULL;

    ret = preload_proplist(inroot->children, fctx);
    if (ret) goto done;

    if (!fctx->req_tgt->collection && !fctx->depth) {
        /* Add response for principal or home-set collection */
        struct mailbox *mailbox = NULL;

        if (fctx->req_tgt->mbentry) {
            /* Open mailbox for reading */
            if ((r = mailbox_open_irl(fctx->req_tgt->mbentry->name, &mailbox))) {
                syslog(LOG_INFO, "mailbox_open_irl(%s) failed: %s",
                       fctx->req_tgt->mbentry->name, error_message(r));
                fctx->err->desc = error_message(r);
                ret = HTTP_SERVER_ERROR;
                goto done;
            }
            fctx->mailbox = mailbox;
        }

        xml_add_response(fctx, 0, 0);

        mailbox_close(&mailbox);
    }

    if (fctx->depth > 0) {
        /* Calendar collection(s) */

        if (fctx->req_tgt->collection) {
            /* Add response for target calendar collection */
            propfind_by_collection(fctx->req_tgt->mbentry->name, 0, 0, fctx);
        }
        else {
            /* Add responses for all calendar collections */
            int isadmin = httpd_userisadmin||httpd_userisproxyadmin;
            r = mboxlist_findall(&httpd_namespace, "*", isadmin, httpd_userid,
                                 httpd_authstate, propfind_by_collection, fctx);
        }

        if (fctx->davdb) fctx->close_db(fctx->davdb);

        ret = *fctx->ret;
    }

  done:
    /* Free the entry list */
    while (fctx->elist) {
        struct propfind_entry_list *freeme = fctx->elist;
        fctx->elist = freeme->next;
        xmlFree(freeme->name);
        free(freeme);
    }

    free(req_tgt.userid);

    fctx->mailbox = prev_ctx.mailbox;
    fctx->depth = prev_ctx.depth;
    fctx->root = prev_ctx.root;
    fctx->elist = prev_ctx.elist;
    fctx->lprops = prev_ctx.lprops;
    fctx->req_tgt = prev_ctx.req_tgt;

    if (root != fctx->root) {
        /* Move any defined namespaces up to the previous parent */
        xmlNsPtr nsDef;

        if (fctx->root->nsDef) {
            /* Find last nsDef in list */
            for (nsDef = fctx->root->nsDef; nsDef->next; nsDef = nsDef->next);
            nsDef->next = root->nsDef;
        }
        else fctx->root->nsDef = root->nsDef;
        root->nsDef = NULL;
    }

    return ret;
}



/* DAV:expand-property REPORT */
int report_expand_prop(struct transaction_t *txn __attribute__((unused)),
                       struct meth_params *rparams __attribute__((unused)),
                       xmlNodePtr inroot, struct propfind_ctx *fctx)
{
    int ret = expand_property(inroot, fctx, NULL, NULL, fctx->lprops,
                              fctx->root, fctx->depth);

    return (ret ? ret : HTTP_MULTI_STATUS);
}


/* DAV:acl-principal-prop-set REPORT */
int report_acl_prin_prop(struct transaction_t *txn __attribute__((unused)),
                         struct meth_params *rparams __attribute__((unused)),
                         xmlNodePtr inroot, struct propfind_ctx *fctx)
{
    int ret = 0;
    struct request_target_t req_tgt;
    mbentry_t *mbentry = fctx->req_tgt->mbentry;
    char *userid, *nextid;
    xmlNodePtr cur;

    /* Generate URL for user principal collection */
    buf_reset(&fctx->buf);
    buf_printf(&fctx->buf, "%s/user/", namespace_principal.prefix);

    /* Allowed properties are for principals, NOT the request URL */
    memset(&req_tgt, 0, sizeof(struct request_target_t));
    principal_parse_path(buf_cstring(&fctx->buf), &req_tgt, &fctx->err->desc);
    fctx->req_tgt = &req_tgt;
    fctx->lprops = principal_props;
    fctx->proc_by_resource = &propfind_by_resource;

    /* Parse children element of report */
    for (cur = inroot->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE &&
            !xmlStrcmp(cur->name, BAD_CAST "prop")) {

            if ((ret = preload_proplist(cur->children, fctx))) goto done;
            break;
        }
    }

    /* Parse the ACL string (userid/rights pairs) */
    for (userid = mbentry->acl; userid; userid = nextid) {
        char *rightstr;

        rightstr = strchr(userid, '\t');
        if (!rightstr) break;
        *rightstr++ = '\0';

        nextid = strchr(rightstr, '\t');
        if (!nextid) break;
        *nextid++ = '\0';

        if (strcmp(userid, "anyone") && strcmp(userid, "anonymous")) {
            /* Add userid to principal URL */
            strcpy(req_tgt.tail, userid);
            req_tgt.userid = xstrdup(userid);

            /* Add response for URL */
            xml_add_response(fctx, 0, 0);

            free(req_tgt.userid);
        }
    }

  done:
    return (ret ? ret : HTTP_MULTI_STATUS);
}


struct search_crit {
    struct strlist *props;
    xmlChar *match;
    struct search_crit *next;
};


/* mboxlist_findall() callback to find user principals (has Inbox) */
static int principal_search(const char *mboxname,
                            int matchlen __attribute__((unused)),
                            int maycreate __attribute__((unused)),
                            void *rock)
{
    struct propfind_ctx *fctx = (struct propfind_ctx *) rock;
    const char *userid = mboxname_to_userid(mboxname);
    struct search_crit *search_crit;
    size_t len;
    char *p;

    for (search_crit = (struct search_crit *) fctx->filter_crit;
         search_crit; search_crit = search_crit->next) {
        struct strlist *prop;

        for (prop = search_crit->props; prop; prop = prop->next) {
            if (!strcmp(prop->s, "displayname")) {
                if (!xmlStrcasestr(BAD_CAST userid,
                                   search_crit->match)) return 0;
            }
            else if (!strcmp(prop->s, "calendar-user-address-set")) {
                char email[MAX_MAILBOX_NAME+1];

                snprintf(email, MAX_MAILBOX_NAME, "%s@%s",
                         userid, config_servername);
                if (!xmlStrcasestr(BAD_CAST email,
                                   search_crit->match)) return 0;
            }
            else if (!strcmp(prop->s, "calendar-user-type")) {
                if (!xmlStrcasestr(BAD_CAST "INDIVIDUAL",
                                   search_crit->match)) return 0;
            }
        }
    }

    /* Append principal name to URL path */
    len = strlen(namespace_principal.prefix);
    p = fctx->req_tgt->path + len;
    len += strlcpy(p, "/user/", MAX_MAILBOX_PATH - len);
    p = fctx->req_tgt->path + len;
    strlcpy(p, userid, MAX_MAILBOX_PATH - len);
    strlcpy(p, "/", MAX_MAILBOX_PATH - len);

    free(fctx->req_tgt->userid);
    fctx->req_tgt->userid = xstrdup(userid);

    return xml_add_response(fctx, 0, 0);
}


static const struct prop_entry prin_search_props[] = {

    /* WebDAV (RFC 4918) properties */
    { "displayname", NS_DAV, 0, NULL, NULL, NULL },

    /* CalDAV Scheduling (RFC 6638) properties */
    { "calendar-user-address-set", NS_CALDAV, 0, NULL, NULL, NULL },
    { "calendar-user-type", NS_CALDAV, 0, NULL, NULL, NULL },

    { NULL, 0, 0, NULL, NULL, NULL }
};


/* DAV:principal-property-search REPORT */
static int report_prin_prop_search(struct transaction_t *txn,
                                   struct meth_params *rparams __attribute__((unused)),
                                   xmlNodePtr inroot,
                                   struct propfind_ctx *fctx)
{
    int ret = 0;
    xmlNodePtr node;
    struct search_crit *search_crit, *next;
    unsigned apply_prin_set = 0;

    /* Parse children element of report */
    fctx->filter_crit = NULL;
    for (node = inroot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            if (!xmlStrcmp(node->name, BAD_CAST "property-search")) {
                xmlNodePtr search;

                search_crit = xzmalloc(sizeof(struct search_crit));
                search_crit->next = fctx->filter_crit;
                fctx->filter_crit = search_crit;

                for (search = node->children; search; search = search->next) {
                    if (search->type == XML_ELEMENT_NODE) {
                        if (!xmlStrcmp(search->name, BAD_CAST "prop")) {
                            xmlNodePtr prop;

                            for (prop = search->children;
                                 prop; prop = prop->next) {
                                if (prop->type == XML_ELEMENT_NODE) {
                                    const struct prop_entry *entry;

                                    for (entry = prin_search_props;
                                         entry->name &&
                                             xmlStrcmp(prop->name,
                                                       BAD_CAST entry->name);
                                         entry++);

                                    if (!entry->name) {
                                        txn->error.desc =
                                            "Unsupported XML search prop";
                                        ret = HTTP_BAD_REQUEST;
                                        goto done;
                                    }
                                    else {
                                        appendstrlist(&search_crit->props,
                                                      (char *) entry->name);
                                    }
                                }
                            }
                        }
                        else if (!xmlStrcmp(search->name, BAD_CAST "match")) {
                            if (search_crit->match) {
                                txn->error.desc =
                                    "Too many DAV:match XML elements";
                                ret = HTTP_BAD_REQUEST;
                                goto done;
                            }

                            search_crit->match =
                                xmlNodeListGetString(inroot->doc,
                                                     search->children, 1);
                        }
                        else {
                            txn->error.desc = "Unknown XML element";
                            ret = HTTP_BAD_REQUEST;
                            goto done;
                        }
                    }
                }

                if (!search_crit->props) {
                    txn->error.desc = "Missing DAV:prop XML element";
                    ret = HTTP_BAD_REQUEST;
                    goto done;
                }
                if (!search_crit->match) {
                    txn->error.desc = "Missing DAV:match XML element";
                    ret = HTTP_BAD_REQUEST;
                    goto done;
                }
            }
            else if (!xmlStrcmp(node->name, BAD_CAST "prop")) {
                /* Already parsed in meth_report() */
            }
            else if (!xmlStrcmp(node->name,
                                BAD_CAST "apply-to-principal-collection-set")) {
                apply_prin_set = 1;
            }
            else {
                txn->error.desc = "Unknown XML element";
                ret = HTTP_BAD_REQUEST;
                goto done;
            }
        }
    }

    if (!fctx->filter_crit) {
        txn->error.desc = "Missing DAV:property-search XML element";
        ret = HTTP_BAD_REQUEST;
        goto done;
    }

    /* Only search DAV:principal-collection-set */
    if (apply_prin_set || !fctx->req_tgt->userid) {
        /* XXX  Do LDAP/SQL lookup of CN/email-address(es) here */

        /* Add responses for all users with INBOX on server (as admin) */
        ret = mboxlist_findall(&httpd_namespace, "user.%", 1, httpd_userid,
                               httpd_authstate, principal_search, fctx);
    }

  done:
    for (search_crit = fctx->filter_crit; search_crit; search_crit = next) {
        next = search_crit->next;

        if (search_crit->match) xmlFree(search_crit->match);
        freestrlist(search_crit->props);
        free(search_crit);
    }

    return (ret ? ret : HTTP_MULTI_STATUS);
}


/* DAV:principal-search-property-set REPORT */
static int report_prin_search_prop_set(struct transaction_t *txn,
                                       struct meth_params *rparams __attribute__((unused)),
                                       xmlNodePtr inroot,
                                       struct propfind_ctx *fctx)
{
    xmlNodePtr node;
    const struct prop_entry *entry;

    /* Look for child elements in request */
    for (node = inroot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            txn->error.desc =
                "DAV:principal-search-property-set XML element MUST be empty";
            return HTTP_BAD_REQUEST;
        }
    }

    for (entry = prin_search_props; entry->name; entry++) {
        node = xmlNewChild(fctx->root, NULL,
                           BAD_CAST "principal-search-property", NULL);
        node = xmlNewChild(node, NULL, BAD_CAST "prop", NULL);
        ensure_ns(fctx->ns, entry->ns, fctx->root,
                  known_namespaces[entry->ns].href,
                  known_namespaces[entry->ns].prefix);
        xmlNewChild(node, fctx->ns[entry->ns], BAD_CAST entry->name, NULL);
    }

    return HTTP_OK;
}


/* Perform a REPORT request */
int meth_report(struct transaction_t *txn, void *params)
{
    struct meth_params *rparams = (struct meth_params *) params;
    int ret = 0, r;
    const char **hdr;
    unsigned depth = 0;
    xmlNodePtr inroot = NULL, outroot = NULL, cur, prop = NULL, props = NULL;
    const struct report_type_t *report = NULL;
    xmlNsPtr ns[NUM_NAMESPACE];
    struct hash_table ns_table = { 0, NULL, NULL };
    struct propfind_ctx fctx;
    struct propfind_entry_list *elist = NULL;

    memset(&fctx, 0, sizeof(struct propfind_ctx));

    /* Parse the path */
    if ((r = rparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    /* Make sure method is allowed */
    if (!(txn->req_tgt.allow & ALLOW_DAV)) return HTTP_NOT_ALLOWED;

    /* Check Depth */
    if ((hdr = spool_getheader(txn->req_hdrs, "Depth"))) {
        if (!strcmp(hdr[0], "infinity")) {
            depth = 2;
        }
        else if ((sscanf(hdr[0], "%u", &depth) != 1) || (depth > 1)) {
            txn->error.desc = "Illegal Depth value\r\n";
            return HTTP_BAD_REQUEST;
        }
    }

    /* Parse the REPORT body */
    ret = parse_xml_body(txn, &inroot);
    if (!ret && !inroot) {
        txn->error.desc = "Missing request body\r\n";
        return HTTP_BAD_REQUEST;
    }
    if (ret) goto done;

    /* Add report type to our header cache */
    spool_cache_header(xstrdup(":type"), xstrdup((const char *) inroot->name),
                       txn->req_hdrs);

    /* Check the report type against our supported list */
    for (report = rparams->reports; report && report->name; report++) {
        if (!xmlStrcmp(inroot->name, BAD_CAST report->name)) break;
    }
    if (!report || !report->name) {
        syslog(LOG_WARNING, "REPORT %s", inroot->name);
        /* DAV:supported-report */
        txn->error.precond = DAV_SUPP_REPORT;
        ret = HTTP_FORBIDDEN;
        goto done;
    }

    /* Check any depth limit */
    if (depth && (report->flags & REPORT_DEPTH_ZERO)) {
        txn->error.desc = "Depth header field MUST have value zero (0)";
        ret = HTTP_BAD_REQUEST;
        goto done;
    }

    /* Normalize depth so that:
     * 0 = home-set collection, 1+ = calendar collection, 2+ = calendar resource
     */
    if (txn->req_tgt.collection) depth++;
    if (txn->req_tgt.resource) depth++;

    /* Check ACL and location of mailbox */
    if (report->flags & REPORT_NEED_MBOX) {
        int rights;

        /* Check ACL for current user */
        rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);
        if ((rights & report->reqd_privs) != report->reqd_privs) {
            if (report->reqd_privs == DACL_READFB) ret = HTTP_NOT_FOUND;
            else {
                /* DAV:need-privileges */
                txn->error.precond = DAV_NEED_PRIVS;
                txn->error.resource = txn->req_tgt.path;
                txn->error.rights = report->reqd_privs;
                ret = HTTP_NO_PRIVS;
            }
            goto done;
        }

        if (txn->req_tgt.mbentry->server) {
            /* Remote mailbox */
            struct backend *be;

            be = proxy_findserver(txn->req_tgt.mbentry->server,
                                  &http_protocol, proxy_userid,
                                  &backend_cached, NULL, NULL, httpd_in);
            if (!be) ret = HTTP_UNAVAILABLE;
            else ret = http_pipe_req_resp(be, txn);
            goto done;
        }

        /* Local Mailbox */
    }

    /* Principal or Local Mailbox */

    if (report->flags & (REPORT_NEED_PROPS | REPORT_ALLOW_PROPS)) {
        /* Parse children element of report */
        for (cur = inroot->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE) {
                if (!xmlStrcmp(cur->name, BAD_CAST "allprop")) {
                    fctx.mode = PROPFIND_ALL;
                    prop = cur;
                    break;
                }
                else if (!xmlStrcmp(cur->name, BAD_CAST "propname")) {
                    fctx.mode = PROPFIND_NAME;
                    fctx.prefer = PREFER_MIN;  /* Don't want 404 (Not Found) */
                    prop = cur;
                    break;
                }
                else if (!xmlStrcmp(cur->name, BAD_CAST "prop")) {
                    fctx.mode = PROPFIND_PROP;
                    prop = cur;
                    props = cur->children;
                    break;
                }
            }
        }

        if (!prop && (report->flags & REPORT_NEED_PROPS)) {
            txn->error.desc = "Missing <prop> element in REPORT\r\n";
            ret = HTTP_BAD_REQUEST;
            goto done;
        }
    }

    /* Start construction of our multistatus response */
    if (report->resp_root &&
        !(outroot = init_xml_response(report->resp_root, NS_DAV, inroot, ns))) {
        txn->error.desc = "Unable to create XML response\r\n";
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Populate our propfind context */
    fctx.req_tgt = &txn->req_tgt;
    fctx.depth = depth;
    fctx.prefer |= get_preferences(txn);
    fctx.req_hdrs = txn->req_hdrs;
    fctx.userid = proxy_userid;
    fctx.userisadmin = httpd_userisadmin;
    fctx.authstate = httpd_authstate;
    fctx.mailbox = NULL;
    fctx.record = NULL;
    fctx.reqd_privs = report->reqd_privs;
    fctx.elist = NULL;
    fctx.lprops = rparams->lprops;
    fctx.root = outroot;
    fctx.ns = ns;
    fctx.ns_table = &ns_table;
    fctx.err = &txn->error;
    fctx.ret = &ret;
    fctx.fetcheddata = 0;

    /* Parse the list of properties and build a list of callbacks */
    if (fctx.mode) {
        fctx.proc_by_resource = &propfind_by_resource;
        ret = preload_proplist(props, &fctx);
    }

    /* Process the requested report */
    if (!ret) ret = (*report->proc)(txn, rparams, inroot, &fctx);

    /* Output the XML response */
    if (outroot) {
        switch (ret) {
        case HTTP_OK:
        case HTTP_MULTI_STATUS:
            /* iCalendar data in response should not be transformed */
            if (fctx.fetcheddata) txn->flags.cc |= CC_NOTRANSFORM;

            xml_response(ret, txn, outroot->doc);

            ret = 0;
            break;

        default:
            break;
        }
    }

  done:
    /* Free the entry list */
    elist = fctx.elist;
    while (elist) {
        struct propfind_entry_list *freeme = elist;
        elist = elist->next;
        xmlFree(freeme->name);
        free(freeme);
    }

    buf_free(&fctx.buf);

    free_hash_table(&ns_table, NULL);

    if (inroot) xmlFreeDoc(inroot->doc);
    if (outroot) xmlFreeDoc(outroot->doc);

    return ret;
}


/* Perform a UNLOCK request
 *
 * preconditions:
 *   DAV:need-privileges
 *   DAV:lock-token-matches-request-uri
 */
int meth_unlock(struct transaction_t *txn, void *params)
{
    struct meth_params *lparams = (struct meth_params *) params;
    int ret = HTTP_NO_CONTENT, r, precond;
    const char **hdr, *token;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record record;
    const char *etag;
    time_t lastmod;
    size_t len;
    void *davdb = NULL;

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    if ((r = lparams->parse_path(txn->req_uri->path,
                                 &txn->req_tgt, &txn->error.desc))) return r;

    /* Make sure method is allowed (only allowed on resources) */
    if (!(txn->req_tgt.allow & ALLOW_WRITE)) return HTTP_NOT_ALLOWED;

    /* Check for mandatory Lock-Token header */
    if (!(hdr = spool_getheader(txn->req_hdrs, "Lock-Token"))) {
        txn->error.desc = "Missing Lock-Token header";
        return HTTP_BAD_REQUEST;
    }
    token = hdr[0];

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, proxy_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Open mailbox for reading */
    r = mailbox_open_irl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = lparams->davdb.open_db(mailbox);
    lparams->davdb.begin_transaction(davdb);

    /* Find message UID for the resource, if exists */
    lparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void **) &ddata, 0);
    if (!ddata->rowid) {
        ret = HTTP_NOT_FOUND;
        goto done;
    }

    /* Check if resource is locked */
    if (ddata->lock_expire <= time(NULL)) {
        /* DAV:lock-token-matches-request-uri */
        txn->error.precond = DAV_BAD_LOCK_TOKEN;
        ret = HTTP_CONFLICT;
        goto done;
    }

    /* Check if current user owns the lock */
    if (strcmp(ddata->lock_ownerid, httpd_userid)) {
        /* Check ACL for current user */
        int rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry->acl);

        if (!(rights & DACL_ADMIN)) {
            /* DAV:need-privileges */
            txn->error.precond = DAV_NEED_PRIVS;
            txn->error.resource = txn->req_tgt.path;
            txn->error.rights = DACL_ADMIN;
            ret = HTTP_NO_PRIVS;
            goto done;
        }
    }

    /* Check if lock token matches */
    len = strlen(ddata->lock_token);
    if (token[0] != '<' || strlen(token) != len+2 || token[len+1] != '>' ||
        strncmp(token+1, ddata->lock_token, len)) {
        /* DAV:lock-token-matches-request-uri */
        txn->error.precond = DAV_BAD_LOCK_TOKEN;
        ret = HTTP_CONFLICT;
        goto done;
    }

    if (ddata->imap_uid) {
        /* Mapped URL - Fetch index record for the resource */
        r = mailbox_find_index_record(mailbox, ddata->imap_uid, &record);
        if (r) {
            txn->error.desc = error_message(r);
            ret = HTTP_SERVER_ERROR;
            goto done;
        }

        etag = message_guid_encode(&record.guid);
        lastmod = record.internaldate;
    }
    else {
        /* Unmapped URL (empty resource) */
        etag = NULL_ETAG;
        lastmod = ddata->creationdate;
    }

    /* Check any preconditions */
    precond = lparams->check_precond(txn, ddata, etag, lastmod);

    if (precond != HTTP_OK) {
        /* We failed a precondition - don't perform the request */
        ret = precond;
        goto done;
    }

    if (ddata->imap_uid) {
        /* Mapped URL - Remove the lock */
        ddata->lock_token = NULL;
        ddata->lock_owner = NULL;
        ddata->lock_ownerid = NULL;
        ddata->lock_expire = 0;

        lparams->davdb.write_resourceLOCKONLY(davdb, ddata);
    }
    else {
        /* Unmapped URL - Treat as lock-null and delete mapping entry */
        lparams->davdb.delete_resourceLOCKONLY(davdb, ddata->rowid);
    }

  done:
    if (davdb) {
        /* XXX error handling abort */
        lparams->davdb.commit_transaction(davdb);
        lparams->davdb.close_db(davdb);
    }
    mailbox_close(&mailbox);

    return ret;
}


static char *strnchr(const char *s, int c, size_t n)
{
    if (!s) return NULL;

    for (; n; n--, s++) if (*s == c) return ((char *) s);

    return NULL;
}


int dav_store_resource(struct transaction_t *txn,
                       const char *data, size_t datalen,
                       struct mailbox *mailbox, struct index_record *oldrecord,
                       strarray_t *imapflags)
{
    int ret = HTTP_CREATED, r;
    hdrcache_t hdrcache = txn->req_hdrs;
    struct stagemsg *stage;
    FILE *f = NULL;
    const char **hdr, *cte;
    quota_t qdiffs[QUOTA_NUMRESOURCES] = QUOTA_DIFFS_DONTCARE_INITIALIZER;
    time_t now = time(NULL);
    struct appendstate as;

    /* Prepare to stage the message */
    if (!(f = append_newstage(mailbox->name, now, 0, &stage))) {
        syslog(LOG_ERR, "append_newstage(%s) failed", mailbox->name);
        txn->error.desc = "append_newstage() failed\r\n";
        return HTTP_SERVER_ERROR;
    }

    /* Create RFC 5322 header for resource */
    if ((hdr = spool_getheader(hdrcache, "User-Agent"))) {
        fprintf(f, "User-Agent: %s\r\n", hdr[0]);
    }

    if ((hdr = spool_getheader(hdrcache, "From"))) {
        fprintf(f, "From: %s\r\n", hdr[0]);
    }
    else {
        char *mimehdr;

        assert(!buf_len(&txn->buf));
        if (strchr(proxy_userid, '@')) {
            /* XXX  This needs to be done via an LDAP/DB lookup */
            buf_printf(&txn->buf, "<%s>", proxy_userid);
        }
        else {
            buf_printf(&txn->buf, "<%s@%s>", proxy_userid, config_servername);
        }

        mimehdr = charset_encode_mimeheader(buf_cstring(&txn->buf),
                                            buf_len(&txn->buf));
        fprintf(f, "From: %s\r\n", mimehdr);
        free(mimehdr);
        buf_reset(&txn->buf);
    }

    if ((hdr = spool_getheader(hdrcache, "Subject"))) {
        fprintf(f, "Subject: %s\r\n", hdr[0]);
    }

    if ((hdr = spool_getheader(hdrcache, "Date"))) {
        fprintf(f, "Date: %s\r\n", hdr[0]);
    }
    else {
        char datestr[80];
        time_to_rfc822(now, datestr, sizeof(datestr));
        fprintf(f, "Date: %s\r\n", datestr);
    }

    if ((hdr = spool_getheader(hdrcache, "Message-ID"))) {
        fprintf(f, "Message-ID: %s\r\n", hdr[0]);
    }

    if ((hdr = spool_getheader(hdrcache, "Content-Type"))) {
        fprintf(f, "Content-Type: %s\r\n", hdr[0]);
    }
    else fputs("Content-Type: application/octet-stream\r\n", f);

    if (!datalen) {
        datalen = strlen(data);
        cte = "8bit";
    }
    else {
        cte = strnchr(data, '\0', datalen) ? "binary" : "8bit";
    }
    fprintf(f, "Content-Transfer-Encoding: %s\r\n", cte);

    if ((hdr = spool_getheader(hdrcache, "Content-Disposition"))) {
        fprintf(f, "Content-Disposition: %s\r\n", hdr[0]);
    }

    if ((hdr = spool_getheader(hdrcache, "Content-Description"))) {
        fprintf(f, "Content-Description: %s\r\n", hdr[0]);
    }

    fprintf(f, "Content-Length: %u\r\n", (unsigned) datalen);

    fputs("MIME-Version: 1.0\r\n\r\n", f);

    /* Write the data to the file */
    fwrite(data, datalen, 1, f);
    qdiffs[QUOTA_STORAGE] = ftell(f);

    fclose(f);

    qdiffs[QUOTA_MESSAGE] = 1;

    /* Prepare to append the message to the mailbox */
    if ((r = append_setup_mbox(&as, mailbox, httpd_userid, httpd_authstate,
                          0, qdiffs, 0, 0, EVENT_MESSAGE_NEW|EVENT_CALENDAR))) {
        syslog(LOG_ERR, "append_setup(%s) failed: %s",
               mailbox->name, error_message(r));
        ret = HTTP_SERVER_ERROR;
        txn->error.desc = "append_setup() failed\r\n";
    }
    else {
        struct body *body = NULL;

        strarray_t *flaglist = NULL;
        struct entryattlist *annots = NULL;

        if (oldrecord) {
            flaglist = mailbox_extract_flags(mailbox, oldrecord, httpd_userid);
            annots = mailbox_extract_annots(mailbox, oldrecord);
        }

        /* XXX - casemerge?  Doesn't matter with flags */
        if (imapflags) strarray_cat(flaglist, imapflags);

        /* Append the message to the mailbox */
        if ((r = append_fromstage(&as, &body, stage, now, flaglist, 0, annots))) {
            syslog(LOG_ERR, "append_fromstage(%s) failed: %s",
                   mailbox->name, error_message(r));
            ret = HTTP_SERVER_ERROR;
            txn->error.desc = "append_fromstage() failed\r\n";
        }
        if (body) {
            message_free_body(body);
            free(body);
        }
        strarray_free(flaglist);
        freeentryatts(annots);

        if (r) append_abort(&as);
        else {
            /* Commit the append to the mailbox */
            if ((r = append_commit(&as))) {
                syslog(LOG_ERR, "append_commit(%s) failed: %s",
                       mailbox->name, error_message(r));
                ret = HTTP_SERVER_ERROR;
                txn->error.desc = "append_commit() failed\r\n";
            }
            else {
                /* Read index record for new message (always the last one) */
                struct index_record newrecord;
                memset(&newrecord, 0, sizeof(struct index_record));
                newrecord.recno = mailbox->i.num_records;
                newrecord.uid = mailbox->i.last_uid;

                mailbox_reload_index_record(mailbox, &newrecord);

                if (oldrecord) {
                    /* Now that we have the replacement message in place
                       expunge the old one. */
                    int userflag;

                    ret = HTTP_NO_CONTENT;

                    /* Perform the actual expunge */
                    r = mailbox_user_flag(mailbox, DFLAG_UNBIND, &userflag, 1);
                    if (!r) {
                        oldrecord->user_flags[userflag/32] |= 1 << (userflag & 31);
                        oldrecord->system_flags |= FLAG_EXPUNGED;
                        r = mailbox_rewrite_index_record(mailbox, oldrecord);
                    }
                    if (r) {
                        syslog(LOG_ERR, "expunging record (%s) failed: %s",
                               mailbox->name, error_message(r));
                        txn->error.desc = error_message(r);
                        ret = HTTP_SERVER_ERROR;
                    }
                }

                if (!r) {
                    struct resp_body_t *resp_body = &txn->resp_body;
                    static char *newetag;

                    if (newetag) free(newetag);
                    newetag = xstrdupnull(message_guid_encode(&newrecord.guid));

                    /* Tell client about the new resource */
                    resp_body->lastmod = newrecord.internaldate;
                    resp_body->etag = newetag;
                }
            }
        }
    }

    append_removestage(stage);

    return ret;
}


static int server_info(struct transaction_t *txn)
{
    int precond;
    static struct message_guid prev_guid;
    struct message_guid guid;
    const char *etag;
    static time_t compile_time = 0, lastmod;
    struct stat sbuf;
    static xmlChar *buf = NULL;
    static int bufsiz = 0;

    if (!httpd_userid) return HTTP_UNAUTHORIZED;

    /* Initialize */
    if (!compile_time) {
        compile_time = calc_compile_time(__TIME__, __DATE__);
        message_guid_set_null(&prev_guid);
    }

    /* Generate ETag based on compile date/time of this source file,
       the number of available RSCALEs and the config file size/mtime */
    stat(config_filename, &sbuf);
    lastmod = MAX(compile_time, sbuf.st_mtime);

    assert(!buf_len(&txn->buf));
    buf_printf(&txn->buf, "%ld-%ld-%ld", (long) compile_time,
               sbuf.st_mtime, sbuf.st_size);

    message_guid_generate(&guid, buf_cstring(&txn->buf), buf_len(&txn->buf));
    etag = message_guid_encode(&guid);

    /* Check any preconditions, including range request */
    txn->flags.ranges = 1;
    precond = check_precond(txn, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
    case HTTP_PARTIAL:
    case HTTP_NOT_MODIFIED:
        /* Fill in Etag,  Last-Modified, and Expires */
        txn->resp_body.etag = etag;
        txn->resp_body.lastmod = lastmod;
        txn->resp_body.maxage = 86400;  /* 24 hrs */
        txn->flags.cc |= CC_MAXAGE;

        if (precond != HTTP_NOT_MODIFIED) break;

    default:
        /* We failed a precondition - don't perform the request */
        return precond;
    }

    if (!message_guid_equal(&prev_guid, &guid)) {
        const char **hdr = spool_getheader(txn->req_hdrs, "Host");
        xmlNodePtr root, node, services, service;
        xmlNsPtr ns[NUM_NAMESPACE];

        /* Start construction of our query-result */
        if (!(root = init_xml_response("server-info", NS_DAV, NULL, ns))) {
            txn->error.desc = "Unable to create XML response";
            return HTTP_SERVER_ERROR;
        }

        /* Add global DAV features */
        node = xmlNewChild(root, NULL, BAD_CAST "features", NULL);
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature", BAD_CAST "class-1");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature", BAD_CAST "class-2");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature", BAD_CAST "class-3");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature",
                    BAD_CAST "access-control");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature",
                    BAD_CAST "extended-mkcol");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature",
                    BAD_CAST "quota");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature",
                    BAD_CAST "sync-collection");
        xmlNewChild(node, ns[NS_DAV], BAD_CAST "feature",
                    BAD_CAST "add-member");

        services = xmlNewChild(root, NULL, BAD_CAST "services", NULL);

        if (namespace_calendar.enabled) {
            service = xmlNewChild(services, NULL, BAD_CAST "service", NULL);
            ensure_ns(ns, NS_CALDAV, service, XML_NS_CALDAV, "C");
            xmlNewChild(service, ns[NS_CALDAV],
                        BAD_CAST "name", BAD_CAST "caldav");

            /* Add href */
            buf_reset(&txn->buf);
            buf_printf(&txn->buf, "%s://%s", https? "https" : "http", hdr[0]);
            buf_appendcstr(&txn->buf, namespace_calendar.prefix);
            xml_add_href(service, ns[NS_DAV], buf_cstring(&txn->buf));

            /* Add CalDAV features */
            node = xmlNewChild(service, NULL, BAD_CAST "features", NULL);
            xmlNewChild(node, ns[NS_CALDAV], BAD_CAST "feature",
                        BAD_CAST "calendar-access");
            if (namespace_calendar.allow & ALLOW_CAL_AVAIL)
                xmlNewChild(node, ns[NS_CALDAV], BAD_CAST "feature",
                            BAD_CAST "calendar-availability");
            if (namespace_calendar.allow & ALLOW_CAL_SCHED)
                xmlNewChild(node, ns[NS_CALDAV], BAD_CAST "feature",
                            BAD_CAST "calendar-auto-schedule");
            if (namespace_calendar.allow & ALLOW_CAL_NOTZ)
                xmlNewChild(node, ns[NS_CALDAV], BAD_CAST "feature",
                            BAD_CAST "calendar-no-timezone");
            if (namespace_calendar.allow & ALLOW_CAL_ATTACH) {
                xmlNewChild(node, ns[NS_CALDAV], BAD_CAST "feature",
                            BAD_CAST "calendar-managed-attachments");
                xmlNewChild(node, ns[NS_CALDAV], BAD_CAST "feature",
                            BAD_CAST "calendar-managed-attachments-no-recurrence");
            }
#if 0
            /* Add properties */
            {
                struct propfind_ctx fctx;
                struct request_target_t req_tgt;
                struct mailbox mailbox;
                struct index_record record;
                struct propstat propstat[PROPSTAT_OK+1];

                /* Setup a dummy propfind_ctx and propstat */
                memset(&fctx, 0, sizeof(struct propfind_ctx));
                memset(&req_tgt, 0, sizeof(struct request_target_t));
                fctx.req_tgt = &req_tgt;
                fctx.req_tgt->collection = "";
                fctx.req_hdrs = txn->req_hdrs;
                fctx.mailbox = &mailbox;
                fctx.mailbox->name = "";
                fctx.record = &record;
                fctx.ns = ns;

                propstat[PROPSTAT_OK].root = xmlNewNode(NULL, BAD_CAST "");
                node = xmlNewChild(propstat[PROPSTAT_OK].root,
                                   ns[NS_DAV], BAD_CAST "properties", NULL);

                propfind_suplock(BAD_CAST "supportedlock", ns[NS_DAV],
                                 &fctx, NULL, &propstat[PROPSTAT_OK], NULL);
                propfind_aclrestrict(BAD_CAST "acl-restrictions", ns[NS_DAV],
                                     &fctx, NULL, &propstat[PROPSTAT_OK], NULL);
                propfind_suppcaldata(BAD_CAST "supported-calendar-data",
                                     ns[NS_CALDAV],
                                     &fctx, NULL, &propstat[PROPSTAT_OK], NULL);
                propfind_calcompset(BAD_CAST "supported-calendar-component-sets",
                                    ns[NS_CALDAV],
                                    &fctx, NULL, &propstat[PROPSTAT_OK], NULL);
                propfind_rscaleset(BAD_CAST "supported-rscale-set", ns[NS_CALDAV],
                                   &fctx, NULL, &propstat[PROPSTAT_OK], NULL);
                propfind_maxsize(BAD_CAST "max-resource-size", ns[NS_CALDAV],
                                 &fctx, NULL, &propstat[PROPSTAT_OK], NULL);
                propfind_minmaxdate(BAD_CAST "min-date-time", ns[NS_CALDAV],
                                    &fctx, NULL, &propstat[PROPSTAT_OK],
                                    &caldav_epoch);
                propfind_minmaxdate(BAD_CAST "max-date-time", ns[NS_CALDAV],
                                    &fctx, NULL, &propstat[PROPSTAT_OK],
                                    &caldav_eternity);

                /* Unlink properties from propstat and link to service */
                xmlUnlinkNode(node);
                xmlAddChild(service, node);

                /* Cleanup */
                xmlFreeNode(propstat[PROPSTAT_OK].root);
                buf_free(&fctx.buf);
            }
#endif
        }

        if (namespace_addressbook.enabled) {
            service = xmlNewChild(services, NULL, BAD_CAST "service", NULL);
            ensure_ns(ns, NS_CARDDAV, service, XML_NS_CARDDAV, "A");
            xmlNewChild(service, ns[NS_CARDDAV],
                        BAD_CAST "name", BAD_CAST "carddav");

            /* Add href */
            buf_reset(&txn->buf);
            buf_printf(&txn->buf, "%s://%s", https? "https" : "http", hdr[0]);
            buf_appendcstr(&txn->buf, namespace_addressbook.prefix);
            xml_add_href(service, ns[NS_DAV], buf_cstring(&txn->buf));

            /* Add CardDAV features */
            node = xmlNewChild(service, NULL, BAD_CAST "features", NULL);
            xmlNewChild(node, ns[NS_CARDDAV], BAD_CAST "feature",
                        BAD_CAST "addressbook");
        }

        /* Dump XML response tree into a text buffer */
        if (buf) xmlFree(buf);
        xmlDocDumpFormatMemoryEnc(root->doc, &buf, &bufsiz, "utf-8", 1);
        xmlFreeDoc(root->doc);

        if (!buf) {
            txn->error.desc = "Error dumping XML tree";
            return HTTP_SERVER_ERROR;
        }

        message_guid_copy(&prev_guid, &guid);
    }

    /* Output the XML response */
    txn->resp_body.type = "application/xml; charset=utf-8";
    write_body(precond, txn, (char *) buf, bufsiz);

    return 0;
}
