/* mboxname.c -- Mailbox list manipulation routines
 *
 * Copyright (c) 1994-2008 Carnegie Mellon University.  All rights reserved.
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
 */

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "assert.h"
#include "byteorder64.h"
#include "crc32.h"
#include "exitcodes.h"
#include "glob.h"
#include "global.h"
#include "mailbox.h"
#include "map.h"
#include "retry.h"
#include "user.h"
#include "util.h"
#include "xmalloc.h"

/* generated headers are not necessarily in current directory */
#include "imap/imap_err.h"

#include "mboxname.h"
#include "mboxlist.h"
#include "cyr_lock.h"

struct mboxlocklist {
    struct mboxlocklist *next;
    struct mboxlock l;
    int nopen;
};

static struct mboxlocklist *open_mboxlocks = NULL;

static struct namespace *admin_namespace;

struct mbname_parts {
    /* master data */
    strarray_t *boxes;
    time_t is_deleted;
    char *localpart;
    char *domain;

    /* actual namespace */
    const struct namespace *extns;
    char *extuserid;

    /* cache data */
    char *userid;
    char *intname;
    char *extname;
    char *recipient;
};

#define XX 127
/*
 * Table for decoding modified base64 for IMAP UTF-7 mailbox names
 */
static const char index_mod64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, 63,XX,XX,XX,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};
#define CHARMOD64(c)  (index_mod64[(unsigned char)(c)])

#define FNAME_SHAREDPREFIX "shared"


static struct mboxlocklist *create_lockitem(const char *name)
{
    struct mboxlocklist *item = xmalloc(sizeof(struct mboxlocklist));
    item->next = open_mboxlocks;
    open_mboxlocks = item;

    item->nopen = 1;
    item->l.name = xstrdup(name);
    item->l.lock_fd = -1;
    item->l.locktype = 0;

    return item;
}

static struct mboxlocklist *find_lockitem(const char *name)
{
    struct mboxlocklist *item;

    for (item = open_mboxlocks; item; item = item->next) {
        if (!strcmp(name, item->l.name))
            return item;
    }

    return NULL;
}

static void remove_lockitem(struct mboxlocklist *remitem)
{
    struct mboxlocklist *item;
    struct mboxlocklist *previtem = NULL;

    for (item = open_mboxlocks; item; item = item->next) {
        if (item == remitem) {
            if (previtem)
                previtem->next = item->next;
            else
                open_mboxlocks = item->next;
            if (item->l.lock_fd != -1) {
                if (item->l.locktype)
                    lock_unlock(item->l.lock_fd, item->l.name);
                close(item->l.lock_fd);
            }
            free(item->l.name);
            free(item);
            return;
        }
        previtem = item;
    }

    fatal("didn't find item in list", EC_SOFTWARE);
}

/* name locking support */

EXPORTED int mboxname_lock(const char *mboxname, struct mboxlock **mboxlockptr,
                  int locktype_and_flags)
{
    const char *fname;
    int r = 0;
    struct mboxlocklist *lockitem;
    int nonblock;
    int locktype;

    nonblock = !!(locktype_and_flags & LOCK_NONBLOCK);
    locktype = (locktype_and_flags & ~LOCK_NONBLOCK);

    fname = mboxname_lockpath(mboxname);
    if (!fname)
        return IMAP_MAILBOX_BADNAME;

    lockitem = find_lockitem(mboxname);

    /* already open?  just use this one */
    if (lockitem) {
        /* can't change locktype! */
        if (lockitem->l.locktype != locktype)
            return IMAP_MAILBOX_LOCKED;

        lockitem->nopen++;
        goto done;
    }

    lockitem = create_lockitem(mboxname);

    /* assume success, and only create directory on failure.
     * More efficient on a common codepath */
    lockitem->l.lock_fd = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (lockitem->l.lock_fd == -1) {
        if (cyrus_mkdir(fname, 0755) == -1) {
            r = IMAP_IOERROR;
            goto done;
        }
        lockitem->l.lock_fd = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0666);
    }
    /* but if it still didn't succeed, we have problems */
    if (lockitem->l.lock_fd == -1) {
        r = IMAP_IOERROR;
        goto done;
    }

    r = lock_setlock(lockitem->l.lock_fd,
                     locktype == LOCK_EXCLUSIVE,
                     nonblock, fname);
    if (!r) lockitem->l.locktype = locktype;
    else if (errno == EWOULDBLOCK) r = IMAP_MAILBOX_LOCKED;
    else r = errno;

done:
    if (r) remove_lockitem(lockitem);
    else *mboxlockptr = &lockitem->l;

    return r;
}

EXPORTED void mboxname_release(struct mboxlock **mboxlockptr)
{
    struct mboxlocklist *lockitem;
    struct mboxlock *lock = *mboxlockptr;

    lockitem = find_lockitem(lock->name);
    assert(lockitem && &lockitem->l == lock);

    *mboxlockptr = NULL;

    if (lockitem->nopen > 1) {
        lockitem->nopen--;
        return;
    }

    remove_lockitem(lockitem);
}

/******************** mbname stuff **********************/

static void _mbdirty(mbname_t *mbname)
{
    free(mbname->userid);
    free(mbname->intname);
    free(mbname->extname);
    free(mbname->recipient);

    mbname->userid = NULL;
    mbname->intname = NULL;
    mbname->extname = NULL;
    mbname->recipient = NULL;
}

EXPORTED void mbname_downcaseuser(mbname_t *mbname)
{
    _mbdirty(mbname);
    if (mbname->localpart) lcase(mbname->localpart);
    if (mbname->domain) lcase(mbname->domain);
}

EXPORTED void mbname_set_localpart(mbname_t *mbname, const char *localpart)
{
    _mbdirty(mbname);
    free(mbname->localpart);
    mbname->localpart = xstrdupnull(localpart);
}

EXPORTED void mbname_set_domain(mbname_t *mbname, const char *domain)
{
    _mbdirty(mbname);
    free(mbname->domain);
    mbname->domain = strcmpsafe(domain, config_defdomain) ? xstrdupnull(domain) : NULL;
}

EXPORTED void mbname_set_boxes(mbname_t *mbname, const strarray_t *boxes)
{
    _mbdirty(mbname);
    strarray_free(mbname->boxes);
    if (boxes)
        mbname->boxes = strarray_dup(boxes);
    else
        mbname->boxes = NULL;
}

EXPORTED void mbname_push_boxes(mbname_t *mbname, const char *item)
{
    _mbdirty(mbname);
    if (!mbname->boxes) mbname->boxes = strarray_new();
    strarray_push(mbname->boxes, item);
}

EXPORTED char *mbname_pop_boxes(mbname_t *mbname)
{
    _mbdirty(mbname);
    if (!mbname->boxes) mbname->boxes = strarray_new();
    return strarray_pop(mbname->boxes);
}

EXPORTED void mbname_truncate_boxes(mbname_t *mbname, size_t len)
{
    _mbdirty(mbname);
    if (!mbname->boxes) mbname->boxes = strarray_new();
    strarray_truncate(mbname->boxes, len);
}

EXPORTED void mbname_set_isdeleted(mbname_t *mbname, time_t isdel)
{
    _mbdirty(mbname);
    mbname->is_deleted = isdel;
}

EXPORTED mbname_t *mbname_from_userid(const char *userid)
{
    mbname_t *mbname = xzmalloc(sizeof(mbname_t));
    const char *p;

    if (!userid)
        return mbname;

    if (!*userid)
        return mbname; // empty string, *sigh*

    mbname->userid = xstrdup(userid); // may as well cache it

    p = strchr(userid, '@');
    if (p) {
        mbname->localpart = xstrndup(userid, p - userid);
        const char *domain = p+1;
        mbname->domain = strcmpsafe(domain, config_defdomain) ? xstrdupnull(domain) : NULL;
    }
    else {
        mbname->localpart = xstrdup(userid);
    }

    return mbname;
}

EXPORTED mbname_t *mbname_from_recipient(const char *recipient, const struct namespace *ns)
{
    mbname_t *mbname = xzmalloc(sizeof(mbname_t));

    if (!recipient)
        return mbname;

    mbname->recipient = xstrdup(recipient); // may as well cache it
    mbname->extns = ns;

    const char *at = strchr(recipient, '@');
    if (at) {
        mbname->localpart = xstrndup(recipient, at - recipient);
        const char *domain = at+1;
        if (config_virtdomains && strcmpsafe(domain, config_defdomain))
            mbname->domain = xstrdupnull(domain);
        /* otherwise we ignore domain entirely */
    }
    else {
        mbname->localpart = xstrdup(recipient);
    }

    char *plus = strchr(mbname->localpart, '+');
    if (plus) {
        char sep[2];
        sep[0] = ns->hier_sep;
        sep[1] = '\0';
        *plus = '\0';
        mbname->boxes = strarray_split(plus+1, sep, /*flags*/0);
    }

    return mbname;
}

EXPORTED mbname_t *mbname_from_extsub(const char *subfolder, const struct namespace *ns, const char *userid)
{
    mbname_t *mbname = mbname_from_userid(userid);

    if (!subfolder)
        return mbname;

    /* we know boxes isn't set already */
    assert(!mbname->boxes);
    char sep[2];
    sep[0] = ns->hier_sep;
    sep[1] = '\0';
    mbname->boxes = strarray_split(subfolder, sep, /*flags*/0);

    return mbname;
}

EXPORTED mbname_t *mbname_dup(const mbname_t *orig)
{
    mbname_t *mbname = xzmalloc(sizeof(mbname_t));

    mbname->localpart = xstrdupnull(orig->localpart);
    mbname->domain = xstrdupnull(orig->domain);
    mbname->is_deleted = orig->is_deleted;
    if (orig->boxes) mbname->boxes = strarray_dup(orig->boxes);

    return mbname;
}

static void _add_dots(char *p)
{
    for (; *p; p++) {
        if (*p == '^') *p = '.';
    }
}

static void _rm_dots(char *p)
{
    for (; *p; p++) {
        if (*p == '.') *p = '^';
    }
}

EXPORTED mbname_t *mbname_from_intname(const char *intname)
{
    mbname_t *mbname = xzmalloc(sizeof(mbname_t));
    const char *p;

    if (!intname)
        return mbname;

    if (!*intname)
        return mbname; // empty string, *sigh*

    const char *dp = config_getstring(IMAPOPT_DELETEDPREFIX);

    mbname->intname = xstrdup(intname); // may as well cache it

    p = strchr(intname, '!');
    if (p) {
        mbname->domain = xstrndup(intname, p - intname);
        if (!strcmpsafe(mbname->domain, config_defdomain)) {
            free(mbname->domain);
            mbname->domain = NULL;
        }
        intname = p+1;
    }

    mbname->boxes = strarray_split(intname, ".", 0);
    int i;
    for (i = 0; i < mbname->boxes->count; i++) {
        _add_dots(mbname->boxes->data[i]);
    }

    if (!strarray_size(mbname->boxes))
        return mbname;

    if (strarray_size(mbname->boxes) > 2 && !strcmp(strarray_nth(mbname->boxes, 0), dp)) {
        free(strarray_shift(mbname->boxes));
        char *delval = strarray_pop(mbname->boxes);
        mbname->is_deleted = strtoul(delval, NULL, 16);
        free(delval);
    }

    if (strarray_size(mbname->boxes) > 1 && !strcmp(strarray_nth(mbname->boxes, 0), "user")) {
        free(strarray_shift(mbname->boxes));
        mbname->localpart = strarray_shift(mbname->boxes);
    }

    return mbname;
}

EXPORTED mbname_t *mbname_from_extname(const char *extname, const struct namespace *ns, const char *userid)
{
    mbname_t *mbname = xzmalloc(sizeof(mbname_t));
    char sepstr[2];
    const char *domain = NULL;

    if (!extname)
        return mbname;

    if (!*extname)
        return mbname; // empty string, *sigh*

    mbname_t *userparts = mbname_from_userid(userid);

    mbname->extname = xstrdup(extname); // may as well cache it

    sepstr[0] = ns->hier_sep;
    sepstr[1] = '\0';

    char *p = config_virtdomains ? strchr(mbname->extname, '@') : NULL;
    if (p) {
        domain = p+1;
        if (!strcmpsafe(domain, config_defdomain))
            domain = NULL;
        *p = '\0'; /* temporary */
    }
    mbname->boxes = strarray_split(mbname->extname, sepstr, 0);
    if (p) *p = '@'; /* repair */

    if (!strarray_size(mbname->boxes))
        goto done;

    if (ns->isalt) {
        /* admin can't be in here, so we can ignore that :) - and hence also
         * the DELETED namespace */
        const char *up = config_getstring(IMAPOPT_USERPREFIX);
        const char *sp = config_getstring(IMAPOPT_SHAREDPREFIX);

        if (!strcmp(strarray_nth(mbname->boxes, 0), up)) {
            /* other user namespace */
            free(strarray_shift(mbname->boxes));
            /* XXX - cross domain support.  For now, it's always in the
             * userid's domain, if any */
            mbname->localpart = strarray_shift(mbname->boxes);
            mbname->domain = xstrdupnull(mbname_domain(userparts));
            goto done;
        }

        if (!strcmp(strarray_nth(mbname->boxes, 0), sp)) {
            /* shared namespace, no user */
            free(strarray_shift(mbname->boxes));
            goto done;
        }

        /* everything else belongs to the userid */
        mbname->localpart = xstrdupnull(mbname_localpart(userparts));
        mbname->domain = xstrdupnull(mbname_domain(userparts));
        /* special case pure inbox with case, because horrible */
        if (strarray_size(mbname->boxes) == 1 && !strcasecmp(strarray_nth(mbname->boxes, 0), "INBOX"))
            free(strarray_shift(mbname->boxes));

        goto done;
    }

    const char *dp = config_getstring(IMAPOPT_DELETEDPREFIX);

    /* special inbox with insensitivity still, because horrible */
    if (!strcasecmp(strarray_nth(mbname->boxes, 0), "INBOX")) {
        free(strarray_shift(mbname->boxes));
        mbname->localpart = xstrdupnull(mbname_localpart(userparts));
        mbname->domain = xstrdupnull(mbname_domain(userparts));
        goto done;
    }

    /* deleted prefix first */
    if (!strcmp(strarray_nth(mbname->boxes, 0), dp)) {
        free(strarray_shift(mbname->boxes));
        char *delval = strarray_pop(mbname->boxes);
        if (!delval)
            goto done;
        mbname->is_deleted = strtoul(delval, NULL, 16);
        free(delval);
    }

    if (!strarray_size(mbname->boxes))
        goto done;

    /* now look for user */
    if (!strcmp(strarray_nth(mbname->boxes, 0), "user")) {
        free(strarray_shift(mbname->boxes));
        mbname->localpart = strarray_shift(mbname->boxes);
        mbname->domain = xstrdupnull(domain ? domain : mbname_domain(userparts));
    }

    /* the rest is just in boxes */
 done:
    mbname_free(&userparts);

    return mbname;
}

EXPORTED void mbname_free(mbname_t **mbnamep)
{
    mbname_t *mbname = *mbnamep;
    if (!mbname) return;

    *mbnamep = NULL;

    strarray_free(mbname->boxes);
    free(mbname->localpart);
    free(mbname->domain);

    /* cached values */
    free(mbname->userid);
    free(mbname->intname);
    free(mbname->extname);
    free(mbname->extuserid);
    free(mbname->recipient);

    /* thing itself */
    free(mbname);
}

EXPORTED char *mboxname_to_userid(const char *intname)
{
    mbname_t *mbname = mbname_from_intname(intname);
    char *res = xstrdupnull(mbname_userid(mbname));
    mbname_free(&mbname);
    return res;
}

EXPORTED char *mboxname_from_external(const char *extname, const struct namespace *ns, const char *userid)
{
    mbname_t *mbname = mbname_from_extname(extname, ns, userid);
    char *res = xstrdupnull(mbname_intname(mbname));
    mbname_free(&mbname);
    return res;
}

EXPORTED char *mboxname_to_external(const char *intname, const struct namespace *ns, const char *userid)
{
    mbname_t *mbname = mbname_from_intname(intname);
    char *res = xstrdupnull(mbname_extname(mbname, ns, userid));
    mbname_free(&mbname);
    return res;
}

/* all mailboxes have an internal name representation, so this
 * function should never return a NULL.
 */
EXPORTED const char *mbname_intname(const mbname_t *mbname)
{
    if (mbname->intname)
        return mbname->intname;

    struct buf buf = BUF_INITIALIZER;
    const char *dp = config_getstring(IMAPOPT_DELETEDPREFIX);
    int sep = 0;
    int i;

    strarray_t *boxes = strarray_dup(mbname_boxes(mbname));

    if (mbname->domain) {
        buf_appendcstr(&buf, mbname->domain);
        buf_putc(&buf, '!');
    }

    if (mbname->is_deleted) {
        buf_appendcstr(&buf, dp);
        sep = 1;
    }

    if (mbname->localpart) {
        if (sep) buf_putc(&buf, '.');
        buf_appendcstr(&buf, "user.");
        char *lp = xstrdup(mbname->localpart);
        _rm_dots(lp);
        buf_appendcstr(&buf, lp);
        free(lp);
        sep = 1;
    }

    for (i = 0; i < strarray_size(boxes); i++) {
        if (sep) buf_putc(&buf, '.');
        char *lp = xstrdup(strarray_nth(boxes, i));
        _rm_dots(lp);
        buf_appendcstr(&buf, lp);
        free(lp);
        sep = 1;
    }

    if (mbname->is_deleted) {
        if (sep) buf_putc(&buf, '.');
        buf_printf(&buf, "%X", (unsigned)mbname->is_deleted);
        sep = 1;
    }

    mbname_t *backdoor = (mbname_t *)mbname;
    backdoor->intname = buf_release(&buf);

    buf_free(&buf);
    strarray_free(boxes);

    return mbname->intname;
}

/* A userid may or may not have a domain - it's just localpart if the
 * domain is unspecified or config_defdomain.  It totally ignores any parts.
 * It's always NULL if there's no localpart
 */
EXPORTED const char *mbname_userid(const mbname_t *mbname)
{
    if (!mbname->localpart)
        return NULL;

    if (mbname->userid)
        return mbname->userid;

    struct buf buf = BUF_INITIALIZER;

    buf_appendcstr(&buf, mbname->localpart);

    if (mbname->domain) {
        buf_putc(&buf, '@');
        buf_appendcstr(&buf, mbname->domain);
    }

    mbname_t *backdoor = (mbname_t *)mbname;
    backdoor->userid = buf_release(&buf);

    buf_free(&buf);

    return mbname->userid;
}

/* A "recipient" is a full username in external form (including domain) with an optional
 * +addressed mailbox in external form, no INBOX prefix (since they can only be mailboxes
 * owned by the user.
 *
 * shared folders (no user) are prefixed with a +, i.e. +shared@domain.com
 *
 * DELETED folders have no recipient, ever.
 */
EXPORTED const char *mbname_recipient(const mbname_t *mbname, const struct namespace *ns)
{
    if (mbname->is_deleted) return NULL;

    /* gotta match up! */
    if (mbname->recipient && ns == mbname->extns)
        return mbname->recipient;

    struct buf buf = BUF_INITIALIZER;

    if (mbname->localpart)
        buf_appendcstr(&buf, mbname->localpart);

    int i;
    for (i = 0; i < strarray_size(mbname->boxes); i++) {
        buf_putc(&buf, i ? ns->hier_sep : '+');
        buf_appendcstr(&buf, strarray_nth(mbname->boxes, i));
    }

    buf_putc(&buf, '@');
    buf_appendcstr(&buf, mbname->domain ? mbname->domain : config_defdomain);

    mbname_t *backdoor = (mbname_t *)mbname;
    free(backdoor->recipient);
    backdoor->recipient = buf_release(&buf);
    backdoor->extns = ns;

    buf_free(&buf);

    return mbname->recipient;
}

/* This is one of the most complex parts of the code - generating an external
 * name based on the namespace, the 'isadmin' status, and of course the current
 * user.  There are some interesting things to look out for:
 *
 * Due to ambiguity, some names won't be representable in the external namespace,
 * so this function can return a NULL in those cases.
 */
EXPORTED const char *mbname_extname(const mbname_t *mbname, const struct namespace *ns, const char *userid)
{
    /* gotta match up! */
    if (mbname->extname && ns == mbname->extns && !strcmpsafe(userid, mbname->extuserid))
        return mbname->extname;

    mbname_t *userparts = mbname_from_userid(userid);
    strarray_t *boxes = strarray_dup(mbname_boxes(mbname));

    struct buf buf = BUF_INITIALIZER;

    if (ns->isalt) {
        const char *up = config_getstring(IMAPOPT_USERPREFIX);
        const char *sp = config_getstring(IMAPOPT_SHAREDPREFIX);

        /* DELETED mailboxes have no extname in alt namespace.
         * There's also no need to display domains, because admins
         * are never in altnamespace, and only admins can see domains */
        if (mbname->is_deleted)
            goto done;

        /* shared */
        if (!mbname->localpart) {
            if (strarray_size(boxes) == 1 && !strcmp(strarray_nth(boxes, 0), "user")) {
                /* special case user all by itself */
                buf_appendcstr(&buf, up);
                goto end;
            }
            buf_appendcstr(&buf, sp);
            int i;
            for (i = 0; i < strarray_size(boxes); i++) {
                buf_putc(&buf, ns->hier_sep);
                buf_appendcstr(&buf, strarray_nth(boxes, i));
            }
            goto end;
        }

        /* other users */
        if (strcmpsafe(mbname_userid(mbname), userid)) {
            buf_appendcstr(&buf, up);
            buf_putc(&buf, ns->hier_sep);
            buf_appendcstr(&buf, mbname->localpart);
            int i;
            for (i = 0; i < strarray_size(boxes); i++) {
                buf_putc(&buf, ns->hier_sep);
                buf_appendcstr(&buf, strarray_nth(boxes, i));
            }
            goto end;
        }

        /* own user */
        if (!strarray_size(boxes)) {
            buf_appendcstr(&buf, "INBOX");
            goto end;
        }

        /* invalid names - anything exactly 'inbox' can't be displayed because
         * select would be ambiguous */
        if (strarray_size(boxes) == 1 && !strcasecmp(strarray_nth(boxes, 0), "INBOX"))
            goto done;

        /* likewise anything exactly matching the user or shared prefixes, both top level
         * or with children */
        if (!strcmp(strarray_nth(boxes, 0), up))
            goto done;
        if (!strcmp(strarray_nth(boxes, 0), sp))
            goto done;

        int i;
        for (i = 0; i < strarray_size(boxes); i++) {
            if (i) buf_putc(&buf, ns->hier_sep);
            buf_appendcstr(&buf, strarray_nth(boxes, i));
        }

        goto end;
    }

    if (mbname->is_deleted) {
        buf_appendcstr(&buf, config_getstring(IMAPOPT_DELETEDPREFIX));
        buf_putc(&buf, ns->hier_sep);
    }

    /* shared */
    if (!mbname->localpart) {
        /* invalid names - not sure it's even possible, but hey */
        if (!strarray_size(boxes))
            goto done;
        if (!strcasecmp(strarray_nth(boxes, 0), "INBOX"))
            goto done;
        /* note "user" precisely appears here, but no need to special case it
         * since the output is the same */
        int i;
        for (i = 0; i < strarray_size(boxes); i++) {
            if (i) buf_putc(&buf, ns->hier_sep);
            buf_appendcstr(&buf, strarray_nth(boxes, i));
        }
        goto end;
    }

    /* other users */
    if (strcmpsafe(mbname_userid(mbname), userid)) {
        buf_appendcstr(&buf, "user");
        buf_putc(&buf, ns->hier_sep);
        buf_appendcstr(&buf, mbname->localpart);
        int i;
        for (i = 0; i < strarray_size(boxes); i++) {
            buf_putc(&buf, ns->hier_sep);
            buf_appendcstr(&buf, strarray_nth(boxes, i));
        }
        goto end;
    }

    buf_appendcstr(&buf, "INBOX");
    int i;
    for (i = 0; i < strarray_size(boxes); i++) {
       buf_putc(&buf, ns->hier_sep);
       buf_appendcstr(&buf, strarray_nth(boxes, i));
    }

 end:

    /* note: kinda bogus in altnamespace, meh */
    if (mbname->is_deleted) {
        buf_putc(&buf, ns->hier_sep);
        buf_printf(&buf, "%X", (unsigned)mbname->is_deleted);
    }

    if (mbname->domain && strcmpsafe(mbname->domain, userparts->domain)) {
        buf_putc(&buf, '@');
        buf_appendcstr(&buf, mbname->domain);
    }

    mbname_t *backdoor = (mbname_t *)mbname;
    free(backdoor->extname);
    backdoor->extname = buf_release(&buf);
    backdoor->extns = ns;
    free(backdoor->extuserid);
    backdoor->extuserid = xstrdupnull(userid);

 done:

    buf_free(&buf);
    mbname_free(&userparts);
    strarray_free(boxes);

    return mbname->extname;
}

EXPORTED const char *mbname_domain(const mbname_t *mbname)
{
    return mbname->domain;
}

EXPORTED const char *mbname_localpart(const mbname_t *mbname)
{
    return mbname->localpart;
}

EXPORTED time_t mbname_isdeleted(const mbname_t *mbname)
{
    return mbname->is_deleted;
}

EXPORTED const strarray_t *mbname_boxes(const mbname_t *mbname)
{
    if (!mbname->boxes) {
        mbname_t *backdoor = (mbname_t *)mbname;
        backdoor->boxes = strarray_new();
    }
    return mbname->boxes;
}

/*
 * Create namespace based on config options.
 */
EXPORTED int mboxname_init_namespace(struct namespace *namespace, int isadmin)
{
    const char *prefix;

    assert(namespace != NULL);

    namespace->isadmin = isadmin;

    namespace->hier_sep =
        config_getswitch(IMAPOPT_UNIXHIERARCHYSEP) ? '/' : '.';
    namespace->isalt = !isadmin && config_getswitch(IMAPOPT_ALTNAMESPACE);

    namespace->accessible[NAMESPACE_INBOX] = 1;
    namespace->accessible[NAMESPACE_USER] = !config_getswitch(IMAPOPT_DISABLE_USER_NAMESPACE);
    namespace->accessible[NAMESPACE_SHARED] = !config_getswitch(IMAPOPT_DISABLE_SHARED_NAMESPACE);

    if (namespace->isalt) {
        /* alternate namespace */
        strcpy(namespace->prefix[NAMESPACE_INBOX], "");

        prefix = config_getstring(IMAPOPT_USERPREFIX);
        if (!prefix || strlen(prefix) == 0 ||
            strlen(prefix) >= MAX_NAMESPACE_PREFIX ||
            strchr(prefix,namespace->hier_sep) != NULL)
            return IMAP_NAMESPACE_BADPREFIX;
        sprintf(namespace->prefix[NAMESPACE_USER], "%.*s%c",
                MAX_NAMESPACE_PREFIX-1, prefix, namespace->hier_sep);

        prefix = config_getstring(IMAPOPT_SHAREDPREFIX);
        if (!prefix || strlen(prefix) == 0 ||
            strlen(prefix) >= MAX_NAMESPACE_PREFIX ||
            strchr(prefix, namespace->hier_sep) != NULL ||
            !strncmp(namespace->prefix[NAMESPACE_USER], prefix, strlen(prefix)))
            return IMAP_NAMESPACE_BADPREFIX;

        if (!isadmin) {
            sprintf(namespace->prefix[NAMESPACE_SHARED], "%.*s%c",
                MAX_NAMESPACE_PREFIX-1, prefix, namespace->hier_sep);
        }
    }

    else {
        /* standard namespace */
        sprintf(namespace->prefix[NAMESPACE_INBOX], "%s%c",
                "INBOX", namespace->hier_sep);
        sprintf(namespace->prefix[NAMESPACE_USER], "%s%c",
                "user", namespace->hier_sep);
        strcpy(namespace->prefix[NAMESPACE_SHARED], "");
    }

    return 0;
}

EXPORTED struct namespace *mboxname_get_adminnamespace()
{
    static struct namespace ns;
    if (!admin_namespace) {
        mboxname_init_namespace(&ns, /*isadmin*/1);
        admin_namespace = &ns;
    }
    return admin_namespace;
}

/*
 * Return nonzero if 'userid' owns the (internal) mailbox 'name'.
 */
EXPORTED int mboxname_userownsmailbox(const char *userid, const char *name)
{
    mbname_t *mbname = mbname_from_intname(name);
    int res = !strcmpsafe(mbname_userid(mbname), userid);
    mbname_free(&mbname);

    return res;
}

/*
 * If (internal) mailbox 'name' is a user's mailbox (optionally INBOX),
 * returns the userid, otherwise returns NULL.
 */
EXPORTED int mboxname_isusermailbox(const char *name, int isinbox)
{
    mbname_t *mbname = mbname_from_intname(name);
    int res = 0;

    if (mbname_localpart(mbname) && !mbname_isdeleted(mbname)) {
        if (!isinbox || !strarray_size(mbname_boxes(mbname)))
            res = 1;
    }

    mbname_free(&mbname);
    return res;
}

/*
 * If (internal) mailbox 'name' is a DELETED mailbox
 * returns boolean
 */
EXPORTED int mboxname_isdeletedmailbox(const char *name, time_t *timestampp)
{
    mbname_t *mbname = mbname_from_intname(name);
    time_t res = mbname_isdeleted(mbname);
    mbname_free(&mbname);

    if (timestampp)
        *timestampp = res;

    return res ? 1 : 0;
}

/*
 * If (internal) mailbox 'name' is a CALENDAR mailbox
 * returns boolean
 */
EXPORTED int mboxname_iscalendarmailbox(const char *name, int mbtype)
{
    if (mbtype & MBTYPE_CALENDAR) return 1;  /* Only works on backends */
    int res = 0;

    mbname_t *mbname = mbname_from_intname(name);
    const strarray_t *boxes = mbname_boxes(mbname);
    const char *prefix = config_getstring(IMAPOPT_CALENDARPREFIX);
    if (strarray_size(boxes) && !strcmpsafe(prefix, strarray_nth(boxes, 0)))
        res = 1;

    mbname_free(&mbname);
    return res;
}

/*
 * If (internal) mailbox 'name' is a ADDRESSBOOK mailbox
 * returns boolean
 */
EXPORTED int mboxname_isaddressbookmailbox(const char *name, int mbtype)
{
    if (mbtype & MBTYPE_ADDRESSBOOK) return 1;  /* Only works on backends */
    int res = 0;

    mbname_t *mbname = mbname_from_intname(name);
    const strarray_t *boxes = mbname_boxes(mbname);
    const char *prefix = config_getstring(IMAPOPT_ADDRESSBOOKPREFIX);
    if (strarray_size(boxes) && !strcmpsafe(prefix, strarray_nth(boxes, 0)))
        res = 1;

    mbname_free(&mbname);
    return res;
}

/*
 * If (internal) mailbox 'name' is a user's "Notes" mailbox
 * returns boolean
 */
EXPORTED int mboxname_isnotesmailbox(const char *name, int mbtype __attribute__((unused)))
{
    int res = 0;

    mbname_t *mbname = mbname_from_intname(name);
    const strarray_t *boxes = mbname_boxes(mbname);
    const char *prefix = config_getstring(IMAPOPT_NOTESMAILBOX);
    if (strarray_size(boxes) && !strcmpsafe(prefix, strarray_nth(boxes, 0)))
        res = 1;

    mbname_free(&mbname);
    return res;
}

EXPORTED char *mboxname_user_mbox(const char *userid, const char *subfolder)
{
    if (!userid) return NULL;

    mbname_t *mbname = mbname_from_userid(userid);

    if (subfolder) {
        strarray_t *bits = strarray_split(subfolder, ".", 0);
        mbname_set_boxes(mbname, bits);
        strarray_free(bits);
    }

    char *res = xstrdup(mbname_intname(mbname));
    mbname_free(&mbname);

    return res;
}

EXPORTED char *mboxname_abook(const char *userid, const char *collection)
{
    mbname_t *mbname = mbname_from_userid(userid);

    mbname_push_boxes(mbname, config_getstring(IMAPOPT_ADDRESSBOOKPREFIX));
    if (collection) mbname_push_boxes(mbname, collection);

    char *res = xstrdup(mbname_intname(mbname));
    mbname_free(&mbname);

    return res;
}

EXPORTED char *mboxname_cal(const char *userid, const char *collection)
{
    mbname_t *mbname = mbname_from_userid(userid);

    mbname_push_boxes(mbname, config_getstring(IMAPOPT_CALENDARPREFIX));
    if (collection) mbname_push_boxes(mbname, collection);

    char *res = xstrdup(mbname_intname(mbname));
    mbname_free(&mbname);

    return res;
}

/*
 * Check whether two parts have the same userid.
 * Returns: 1 if the userids are the same, 0 if not.
 */
EXPORTED int mbname_same_userid(const mbname_t *a, const mbname_t *b)
{
    int r;

    r = strcmpsafe(a->domain, b->domain);
    if (!r)
        r = strcmpsafe(a->localpart, b->localpart);
    return !r;
}

/*
 * Check whether two mboxnames have the same userid.
 * Needed for some corner cases in the COPY command.
 * Returns: 1 if the userids are the same, 0 if not,
 *          or negative error.
 */
EXPORTED int mboxname_same_userid(const char *name1, const char *name2)
{
    int r;
    mbname_t *p1 = mbname_from_intname(name1);
    mbname_t *p2 = mbname_from_intname(name2);

    r = mbname_same_userid(p1, p2);

    mbname_free(&p1);
    mbname_free(&p2);

    return r;
}

/*
 * Apply site policy restrictions on mailbox names.
 * Restrictions are hardwired for now.
 */
#define GOODCHARS " #$'+,-.0123456789:=@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~"
HIDDEN int mboxname_policycheck(const char *name)
{
    const char *p;
    int sawutf7 = 0;
    unsigned c1, c2, c3, c4, c5, c6, c7, c8;
    int ucs4;
    int unixsep;
    int namelen = strlen(name);

    unixsep = config_getswitch(IMAPOPT_UNIXHIERARCHYSEP);

    /* Skip policy check on mailbox created in delayed delete namespace
     * assuming the mailbox existed before and was OK then.
     * This should allow mailboxes that are extremely long to be
     * deleted when delayed_delete is enabled.
     * A thorough fix might remove the prefix and timestamp
     * then continue with the check
     */
    if (mboxname_isdeletedmailbox(name, NULL))
        return 0;

    if (namelen > MAX_MAILBOX_NAME)
        return IMAP_MAILBOX_BADNAME;

    /* find the virtual domain, if any.  We don't sanity check domain
       names yet - maybe we should */
    p = strchr(name, '!');
    if (p) {
        if (config_virtdomains) {
            name = p + 1;
            namelen = strlen(name);
        }
        else
            return IMAP_MAILBOX_BADNAME;
    }

    /* bad mbox patterns */
    // empty name
    if (!name[0]) return IMAP_MAILBOX_BADNAME;
    // leading dot
    if (name[0] == '.') return IMAP_MAILBOX_BADNAME;
    // leading ~
    if (name[0] == '~') return IMAP_MAILBOX_BADNAME;
    // trailing dot
    if (name[namelen-1] == '.') return IMAP_MAILBOX_BADNAME;
    // double dot (zero length path item)
    if (strstr(name, "..")) return IMAP_MAILBOX_BADNAME;
    // non-" " whitespace
    if (strchr(name, '\r')) return IMAP_MAILBOX_BADNAME;
    if (strchr(name, '\n')) return IMAP_MAILBOX_BADNAME;
    if (strchr(name, '\t')) return IMAP_MAILBOX_BADNAME;
    // top level user
    if (!strcmp(name, "user")) return IMAP_MAILBOX_BADNAME;
    // special users
    if (!strcmp(name, "user.anyone")) return IMAP_MAILBOX_BADNAME;
    if (!strcmp(name, "user.anonymous")) return IMAP_MAILBOX_BADNAME;

    while (*name) {
        if (*name == '&') {
            /* Modified UTF-7 */
            name++;
            while (*name != '-') {
                if (sawutf7) {
                    /* Two adjacent utf7 sequences */
                    return IMAP_MAILBOX_BADNAME;
                }

                if ((c1 = CHARMOD64(*name++)) == XX ||
                    (c2 = CHARMOD64(*name++)) == XX ||
                    (c3 = CHARMOD64(*name++)) == XX) {
                    /* Non-base64 character */
                    return IMAP_MAILBOX_BADNAME;
                }
                ucs4 = (c1 << 10) | (c2 << 4) | (c3 >> 2);
                if ((ucs4 & 0xff80) == 0 || (ucs4 & 0xf800) == 0xd800) {
                    /* US-ASCII or multi-word character */
                    return IMAP_MAILBOX_BADNAME;
                }
                if (*name == '-') {
                    /* Trailing bits not zero */
                    if (c3 & 0x03) return IMAP_MAILBOX_BADNAME;

                    /* End of UTF-7 sequence */
                    break;
                }

                if ((c4 = CHARMOD64(*name++)) == XX ||
                    (c5 = CHARMOD64(*name++)) == XX ||
                    (c6 = CHARMOD64(*name++)) == XX) {
                    /* Non-base64 character */
                    return IMAP_MAILBOX_BADNAME;
                }
                ucs4 = ((c3 & 0x03) << 14) | (c4 << 8) | (c5 << 2) | (c6 >> 4);
                if ((ucs4 & 0xff80) == 0 || (ucs4 & 0xf800) == 0xd800) {
                    /* US-ASCII or multi-word character */
                    return IMAP_MAILBOX_BADNAME;
                }
                if (*name == '-') {
                    /* Trailing bits not zero */
                    if (c6 & 0x0f) return IMAP_MAILBOX_BADNAME;

                    /* End of UTF-7 sequence */
                    break;
                }

                if ((c7 = CHARMOD64(*name++)) == XX ||
                    (c8 = CHARMOD64(*name++)) == XX) {
                    /* Non-base64 character */
                    return IMAP_MAILBOX_BADNAME;
                }
                ucs4 = ((c6 & 0x0f) << 12) | (c7 << 6) | c8;
                if ((ucs4 & 0xff80) == 0 || (ucs4 & 0xf800) == 0xd800) {
                    /* US-ASCII or multi-word character */
                    return IMAP_MAILBOX_BADNAME;
                }
            }

            if (name[-1] == '&') sawutf7 = 0; /* '&-' is sequence for '&' */
            else sawutf7 = 1;

            name++;             /* Skip over terminating '-' */
        }
        else {
            /* If we're using unixhierarchysep, DOTCHAR is allowed */
            if (!strchr(GOODCHARS, *name) &&
                !(unixsep && *name == DOTCHAR))
                return IMAP_MAILBOX_BADNAME;
            /* If we're not using virtdomains, '@' is not permitted in the mboxname */
            if (!config_virtdomains && *name == '@')
                return IMAP_MAILBOX_BADNAME;
            name++;
            sawutf7 = 0;
        }
    }
    return 0;
}

EXPORTED int mboxname_is_prefix(const char *longstr, const char *shortstr)
{
    int longlen = strlen(longstr);
    int shortlen = strlen(shortstr);

    /* can't be a child */
    if (longlen < shortlen)
        return 0;

    /* don't match along same length */
    if (strncmp(longstr, shortstr, shortlen))
        return 0;

    /* longer, and not a separator */
    if (longlen > shortlen && longstr[shortlen] != '.')
        return 0;

    /* it's a match! */
    return 1;
}


static void mboxname_hash(char *dest, size_t destlen, const char *root, const char *name)
{
    mbname_t *mbname = mbname_from_intname(name);
    struct buf buf = BUF_INITIALIZER;

    buf_setcstr(&buf, root);

    const char *domain = mbname_domain(mbname);
    strarray_t *boxes = strarray_dup(mbname_boxes(mbname));

    if (domain) {
        if (config_hashimapspool) {
            char c = dir_hash_c(domain, config_fulldirhash);
            buf_printf(&buf, "%s%c/%s", FNAME_DOMAINDIR, c, domain);
        }
        else {
            buf_printf(&buf, "%s%s", FNAME_DOMAINDIR, domain);
        }
    }

    if (mbname_localpart(mbname)) {
        strarray_unshift(boxes, mbname_localpart(mbname));
        strarray_unshift(boxes, "user");
    }
    if (mbname_isdeleted(mbname)) {
        struct buf dbuf = BUF_INITIALIZER;
        buf_printf(&dbuf, "%X", (unsigned)mbname_isdeleted(mbname));
        strarray_unshift(boxes, config_getstring(IMAPOPT_DELETEDPREFIX));
        strarray_push(boxes, buf_cstring(&dbuf));
        buf_free(&dbuf);
    }

    if (config_hashimapspool && strarray_size(boxes)) {
        const char *idx = strarray_size(boxes) > 1 ? strarray_nth(boxes, 1) : strarray_nth(boxes, 0);
        char c = dir_hash_c(idx, config_fulldirhash);
        buf_printf(&buf, "/%c", c);
    }

    int i;
    for (i = 0; i < strarray_size(boxes); i++) {
        char *item = xstrdup(strarray_nth(boxes, i));
        _rm_dots(item);
        buf_printf(&buf, "/%s", item);
        free(item);
    }

    /* for now, keep API even though we're doing a buffer inside here */
    strncpy(dest, buf_cstring(&buf), destlen);

    buf_free(&buf);
    strarray_free(boxes);
    mbname_free(&mbname);
}

/* note: mboxname must be internal */
EXPORTED char *mboxname_datapath(const char *partition,
                                 const char *mboxname,
                                 const char *uniqueid __attribute__((unused)),
                                 unsigned long uid)
{
    static char pathresult[MAX_MAILBOX_PATH+1];
    const char *root;

    if (!partition) return NULL;

    root = config_partitiondir(partition);
    if (!root) return NULL;

    if (!mboxname) {
        xstrncpy(pathresult, root, MAX_MAILBOX_PATH);
        return pathresult;
    }

    mboxname_hash(pathresult, MAX_MAILBOX_PATH, root, mboxname);

    if (uid) {
        int len = strlen(pathresult);
        snprintf(pathresult + len, MAX_MAILBOX_PATH - len, "/%lu.", uid);
    }
    pathresult[MAX_MAILBOX_PATH] = '\0';

    if (strlen(pathresult) == MAX_MAILBOX_PATH)
        return NULL;

    return pathresult;
}

/* note: mboxname must be internal */
EXPORTED char *mboxname_archivepath(const char *partition,
                                    const char *mboxname,
                                    const char *uniqueid __attribute__((unused)),
                                    unsigned long uid)
{
    static char pathresult[MAX_MAILBOX_PATH+1];
    const char *root;

    if (!partition) return NULL;

    root = config_archivepartitiondir(partition);
    if (!root) root = config_partitiondir(partition);
    if (!root) return NULL;

    /* XXX - dedup with datapath above - but make sure to keep the results
     * in separate buffers and/or audit the callers */
    if (!mboxname) {
        xstrncpy(pathresult, root, MAX_MAILBOX_PATH);
        return pathresult;
    }

    mboxname_hash(pathresult, MAX_MAILBOX_PATH, root, mboxname);

    if (uid) {
        int len = strlen(pathresult);
        snprintf(pathresult + len, MAX_MAILBOX_PATH - len, "/%lu.", uid);
    }
    pathresult[MAX_MAILBOX_PATH] = '\0';

    if (strlen(pathresult) == MAX_MAILBOX_PATH)
        return NULL;

    return pathresult;
}

char *mboxname_lockpath(const char *mboxname)
{
    return mboxname_lockpath_suffix(mboxname, ".lock");
}

char *mboxname_lockpath_suffix(const char *mboxname,
                               const char *suffix)
{
    static char lockresult[MAX_MAILBOX_PATH+1];
    char basepath[MAX_MAILBOX_PATH+1];
    const char *root = config_getstring(IMAPOPT_MBOXNAME_LOCKPATH);
    int len;

    if (!root) {
        snprintf(basepath, MAX_MAILBOX_PATH, "%s/lock", config_dir);
        root = basepath;
    }

    mboxname_hash(lockresult, MAX_MAILBOX_PATH, root, mboxname);

    len = strlen(lockresult);
    snprintf(lockresult + len, MAX_MAILBOX_PATH - len, "%s", suffix);
    lockresult[MAX_MAILBOX_PATH] = '\0';

    if (strlen(lockresult) == MAX_MAILBOX_PATH)
        return NULL;

    return lockresult;
}

EXPORTED char *mboxname_metapath(const char *partition,
                                 const char *mboxname,
                                 const char *uniqueid __attribute__((unused)),
                                 int metafile,
                                 int isnew)
{
    static char metaresult[MAX_MAILBOX_PATH];
    int metaflag = 0;
    int archiveflag = 0;
    const char *root = NULL;
    const char *filename = NULL;
    char confkey[256];

    if (!partition) return NULL;

    *confkey = '\0';

    switch (metafile) {
    case META_HEADER:
        snprintf(confkey, 256, "metadir-header-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_HEADER;
        filename = FNAME_HEADER;
        break;
    case META_INDEX:
        snprintf(confkey, 256, "metadir-index-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_INDEX;
        filename = FNAME_INDEX;
        break;
    case META_CACHE:
        snprintf(confkey, 256, "metadir-cache-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_CACHE;
        filename = FNAME_CACHE;
        break;
    case META_EXPUNGE:
        /* not movable, it's only old */
        metaflag = IMAP_ENUM_METAPARTITION_FILES_EXPUNGE;
        filename = FNAME_EXPUNGE;
        break;
    case META_SQUAT:
        snprintf(confkey, 256, "metadir-squat-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_SQUAT;
        filename = FNAME_SQUAT;
        break;
    case META_ANNOTATIONS:
        snprintf(confkey, 256, "metadir-index-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_ANNOTATIONS;
        filename = FNAME_ANNOTATIONS;
        break;
#ifdef WITH_DAV
    case META_DAV:
        snprintf(confkey, 256, "metadir-dav-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_DAV;
        filename = FNAME_DAV;
        break;
#endif
    case META_ARCHIVECACHE:
        snprintf(confkey, 256, "metadir-archivecache-%s", partition);
        metaflag = IMAP_ENUM_METAPARTITION_FILES_ARCHIVECACHE;
        filename = FNAME_CACHE;
        archiveflag = 1;
        break;
    case 0:
        break;
    default:
        fatal("Unknown meta file requested", EC_SOFTWARE);
    }

    if (*confkey)
        root = config_getoverflowstring(confkey, NULL);

    if (!root && (!metaflag || (config_metapartition_files & metaflag)))
        root = config_metapartitiondir(partition);

    if (!root && archiveflag)
        root = config_archivepartitiondir(partition);

    if (!root)
        root = config_partitiondir(partition);

    if (!root)
        return NULL;

    if (!mboxname) {
        xstrncpy(metaresult, root, MAX_MAILBOX_PATH);
        return metaresult;
    }

    mboxname_hash(metaresult, MAX_MAILBOX_PATH, root, mboxname);

    if (filename) {
        int len = strlen(metaresult);
        if (isnew)
            snprintf(metaresult + len, MAX_MAILBOX_PATH - len, "%s.NEW", filename);
        else
            snprintf(metaresult + len, MAX_MAILBOX_PATH - len, "%s", filename);
    }

    if (strlen(metaresult) >= MAX_MAILBOX_PATH)
        return NULL;

    return metaresult;
}

EXPORTED void mboxname_todeleted(const char *name, char *result, int withtime)
{
    int domainlen = 0;
    char *p;
    const char *deletedprefix = config_getstring(IMAPOPT_DELETEDPREFIX);

    xstrncpy(result, name, MAX_MAILBOX_BUFFER);

    if (config_virtdomains && (p = strchr(name, '!')))
        domainlen = p - name + 1;

    if (withtime) {
        struct timeval tv;
        gettimeofday( &tv, NULL );
        snprintf(result+domainlen, MAX_MAILBOX_BUFFER-domainlen, "%s.%s.%X",
                 deletedprefix, name+domainlen, (unsigned) tv.tv_sec);
    } else {
        snprintf(result+domainlen, MAX_MAILBOX_BUFFER-domainlen, "%s.%s",
                 deletedprefix, name+domainlen);
    }
}

EXPORTED int mboxname_make_parent(char *name)
{
    int domainlen = 0;
    char *p;

    if (config_virtdomains && (p = strchr(name, '!')))
        domainlen = p - name + 1;

    if (!name[0] || !strcmp(name+domainlen, "user"))
        return 0;                               /* stop now */

    p = strrchr(name, '.');

    if (p && (p - name > domainlen))            /* don't split subdomain */
        *p = '\0';
    else if (!name[domainlen])                  /* server entry */
        name[0] = '\0';
    else                                        /* domain entry */
        name[domainlen] = '\0';

    return 1;
}

/* NOTE: caller must free, which is different from almost every
 * other interface in the whole codebase.  Grr */
EXPORTED char *mboxname_conf_getpath(const mbname_t *mbname, const char *suffix)
{
    char *fname = NULL;
    char c[2], d[2];

    if (mbname->domain) {
        if (mbname->localpart) {
            fname = strconcat(config_dir,
                              FNAME_DOMAINDIR,
                              dir_hash_b(mbname->domain, config_fulldirhash, d),
                              "/", mbname->domain,
                              FNAME_USERDIR,
                              dir_hash_b(mbname->localpart, config_fulldirhash, c),
                              "/", mbname->localpart, ".", suffix,
                              (char *)NULL);
        }
        else {
            fname = strconcat(config_dir,
                              FNAME_DOMAINDIR,
                              dir_hash_b(mbname->domain, config_fulldirhash, d),
                              "/", mbname->domain,
                              "/", FNAME_SHAREDPREFIX, ".", suffix,
                              (char *)NULL);
        }
    }
    else {
        if (mbname->localpart) {
            fname = strconcat(config_dir,
                              FNAME_USERDIR,
                              dir_hash_b(mbname->localpart, config_fulldirhash, c),
                              "/", mbname->localpart, ".", suffix,
                              (char *)NULL);
        }
        else {
            fname = strconcat(config_dir,
                              "/", FNAME_SHAREDPREFIX, ".", suffix,
                              (char *)NULL);
        }
    }

    return fname;
}

/* ========================= COUNTERS ============================ */

static bit64 mboxname_readval_old(const char *mboxname, const char *metaname)
{
    bit64 fileval = 0;
    mbname_t *mbname = NULL;
    char *fname = NULL;
    const char *base = NULL;
    size_t len = 0;
    int fd = -1;

    mbname = mbname_from_intname(mboxname);

    fname = mboxname_conf_getpath(mbname, metaname);
    if (!fname) goto done;

    fd = open(fname, O_RDONLY);

    /* read the value - note: we don't care if it's being rewritten,
     * we'll still get a consistent read on either the old or new
     * value */
    if (fd != -1) {
        struct stat sbuf;
        if (fstat(fd, &sbuf)) {
            syslog(LOG_ERR, "IOERROR: failed to stat fd %s: %m", fname);
            goto done;
        }
        if (sbuf.st_size) {
            map_refresh(fd, 1, &base, &len, sbuf.st_size, metaname, mboxname);
            parsenum(base, NULL, sbuf.st_size, &fileval);
            map_free(&base, &len);
        }
    }

 done:
    if (fd != -1) close(fd);
    mbname_free(&mbname);
    free(fname);
    return fileval;
}

#define MV_VERSION 1

#define MV_OFF_GENERATION 0
#define MV_OFF_VERSION 4
#define MV_OFF_HIGHESTMODSEQ 8
#define MV_OFF_MAILMODSEQ 16
#define MV_OFF_CALDAVMODSEQ 24
#define MV_OFF_CARDDAVMODSEQ 32
#define MV_OFF_NOTESMODSEQ 40
#define MV_OFF_UIDVALIDITY 48
#define MV_OFF_CRC 52
#define MV_LENGTH 56

/* NOTE: you need a MV_LENGTH byte base here */
static int mboxname_buf_to_counters(const char *base, size_t len, struct mboxname_counters *vals)
{
    vals->generation = ntohl(*((uint32_t *)(base)));
    vals->version = ntohl(*((uint32_t *)(base+4)));

    /* dodgy broken version storage in v0 code, it could be anything */
    if (len == 48) vals->version = 0;

    switch (vals->version) {
    case 0:
        if (len != 48) return IMAP_MAILBOX_CHECKSUM;
        if (crc32_map(base, 44) != ntohl(*((uint32_t *)(base+44))))
            return IMAP_MAILBOX_CHECKSUM;

        vals->highestmodseq = ntohll(*((uint64_t *)(base+8)));
        vals->mailmodseq = ntohll(*((uint64_t *)(base+16)));
        vals->caldavmodseq = ntohll(*((uint64_t *)(base+24)));
        vals->carddavmodseq = ntohll(*((uint64_t *)(base+32)));
        vals->uidvalidity = ntohl(*((uint32_t *)(base+40)));
        break;

    case 1:
        if (len != 56) return IMAP_MAILBOX_CHECKSUM;
        if (crc32_map(base, 52) != ntohl(*((uint32_t *)(base+52))))
            return IMAP_MAILBOX_CHECKSUM;

        vals->highestmodseq = ntohll(*((uint64_t *)(base+8)));
        vals->mailmodseq = ntohll(*((uint64_t *)(base+16)));
        vals->caldavmodseq = ntohll(*((uint64_t *)(base+24)));
        vals->carddavmodseq = ntohll(*((uint64_t *)(base+32)));
        vals->notesmodseq = ntohll(*((uint64_t *)(base+40)));
        vals->uidvalidity = ntohl(*((uint32_t *)(base+48)));
        break;

    default:
        return IMAP_MAILBOX_BADFORMAT;
    }

    return 0;
}

/* NOTE: you need a MV_LENGTH buffer to write into, aligned on 8 byte boundaries */
static void mboxname_counters_to_buf(const struct mboxname_counters *vals, char *base)
{
    *((uint32_t *)(base+MV_OFF_GENERATION)) = htonl(vals->generation);
    *((uint32_t *)(base+MV_OFF_VERSION)) = htonl(MV_VERSION);
    align_htonll(base+MV_OFF_HIGHESTMODSEQ, vals->highestmodseq);
    align_htonll(base+MV_OFF_MAILMODSEQ, vals->mailmodseq);
    align_htonll(base+MV_OFF_CALDAVMODSEQ, vals->caldavmodseq);
    align_htonll(base+MV_OFF_CARDDAVMODSEQ, vals->carddavmodseq);
    align_htonll(base+MV_OFF_NOTESMODSEQ, vals->notesmodseq);
    *((uint32_t *)(base+MV_OFF_UIDVALIDITY)) = htonl(vals->uidvalidity);
    *((uint32_t *)(base+MV_OFF_CRC)) = htonl(crc32_map(base, MV_OFF_CRC));
}

/* XXX - inform about errors?  Any error causes the value of at least
   last+1 to be returned.  An error only on writing causes
   max(last, fileval) + 1 to still be returned */
static int mboxname_load_counters(const char *mboxname, struct mboxname_counters *vals, int *fdp)
{
    int fd = -1;
    char *fname = NULL;
    struct stat sbuf, fbuf;
    const char *base = NULL;
    size_t len = 0;
    mbname_t *mbname = NULL;
    int r = 0;

    memset(vals, 0, sizeof(struct mboxname_counters));

    mbname = mbname_from_intname(mboxname);

    fname = mboxname_conf_getpath(mbname, "counters");
    if (!fname) {
        r = IMAP_MAILBOX_BADNAME;
        goto done;
    }

    /* get a blocking lock on fd */
    for (;;) {
        fd = open(fname, O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            /* OK to not exist - try creating the directory first */
            if (cyrus_mkdir(fname, 0755)) goto done;
            fd = open(fname, O_RDWR | O_CREAT, 0644);
        }
        if (fd == -1) {
            syslog(LOG_ERR, "IOERROR: failed to create %s: %m", fname);
            goto done;
        }
        if (lock_blocking(fd, fname)) {
            syslog(LOG_ERR, "IOERROR: failed to lock %s: %m", fname);
            goto done;
        }
        if (fstat(fd, &sbuf)) {
            syslog(LOG_ERR, "IOERROR: failed to stat fd %s: %m", fname);
            goto done;
        }
        if (stat(fname, &fbuf)) {
            syslog(LOG_ERR, "IOERROR: failed to stat file %s: %m", fname);
            goto done;
        }
        if (sbuf.st_ino == fbuf.st_ino) break;
        lock_unlock(fd, fname);
        close(fd);
        fd = -1;
    }

    if (fd < 0) {
        r = IMAP_IOERROR;
        goto done;
    }

    if (sbuf.st_size >= 8) {
        /* read the old value */
        map_refresh(fd, 1, &base, &len, sbuf.st_size, "counters", mboxname);
        if (len >= 8) {
            r = mboxname_buf_to_counters(base, len, vals);
        }
        map_free(&base, &len);
    }
    else {
        /* going to have to read the old files */
        vals->mailmodseq = vals->caldavmodseq = vals->carddavmodseq =
            vals->highestmodseq = mboxname_readval_old(mboxname, "modseq");
        vals->uidvalidity = mboxname_readval_old(mboxname, "uidvalidity");
    }

done:
    if (r) {
        if (fd != -1) {
            lock_unlock(fd, fname);
            close(fd);
        }
    }
    else {
        /* maintain the lock until we're done */
        *fdp = fd;
    }
    mbname_free(&mbname);
    free(fname);
    return r;
}

static int mboxname_set_counters(const char *mboxname, struct mboxname_counters *vals, int fd)
{
    char *fname = NULL;
    mbname_t *mbname = NULL;
    char buf[MV_LENGTH];
    char newfname[MAX_MAILBOX_PATH];
    int newfd = -1;
    int n = 0;
    int r = 0;

    mbname = mbname_from_intname(mboxname);

    fname = mboxname_conf_getpath(mbname, "counters");
    if (!fname) {
        r = IMAP_MAILBOX_BADNAME;
        goto done;
    }

    snprintf(newfname, MAX_MAILBOX_PATH, "%s.NEW", fname);
    newfd = open(newfname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (newfd == -1) {
        r = IMAP_IOERROR;
        syslog(LOG_ERR, "IOERROR: failed to open for write %s: %m", newfname);
        goto done;
    }

    /* it's a new generation! */
    vals->generation++;

    mboxname_counters_to_buf(vals, buf);
    n = retry_write(newfd, buf, MV_LENGTH);
    if (n < 0) {
        r = IMAP_IOERROR;
        syslog(LOG_ERR, "IOERROR: failed to write %s: %m", newfname);
        goto done;
    }

    if (fdatasync(newfd)) {
        r = IMAP_IOERROR;
        syslog(LOG_ERR, "IOERROR: failed to fdatasync %s: %m", newfname);
        goto done;
    }

    close(newfd);
    newfd = -1;

    if (rename(newfname, fname)) {
        r = IMAP_IOERROR;
        syslog(LOG_ERR, "IOERROR: failed to rename %s: %m", newfname);
        goto done;
    }

 done:
    if (newfd != -1) close(newfd);
    if (fd != -1) {
        lock_unlock(fd, fname);
        close(fd);
    }
    mbname_free(&mbname);
    free(fname);

    return r;
}

static int mboxname_unload_counters(int fd)
{
    lock_unlock(fd, NULL);
    close(fd);
    return 0;
}

EXPORTED int mboxname_read_counters(const char *mboxname, struct mboxname_counters *vals)
{
    int r = 0;
    mbname_t *mbname = NULL;
    struct stat sbuf;
    char *fname = NULL;
    const char *base = NULL;
    size_t len = 0;
    int fd = -1;

    memset(vals, 0, sizeof(struct mboxname_counters));

    mbname = mbname_from_intname(mboxname);

    fname = mboxname_conf_getpath(mbname, "counters");
    if (!fname) {
        r = IMAP_MAILBOX_BADNAME;
        goto done;
    }

    fd = open(fname, O_RDONLY);

    /* if no file, import from the old files potentially, and write a file regardless */
    if (fd < 0) {
        /* race => multiple rewrites, won't hurt too much */
        r = mboxname_load_counters(mboxname, vals, &fd);
        if (r) goto done;
        r = mboxname_set_counters(mboxname, vals, fd);
        fd = -1;
        if (r) goto done;
        free(fname);
        fname = mboxname_conf_getpath(mbname, "modseq");
        if (fname) unlink(fname);
        free(fname);
        fname = mboxname_conf_getpath(mbname, "uidvalidity");
        if (fname) unlink(fname);
        goto done;
    }

    if (fstat(fd, &sbuf)) {
        syslog(LOG_ERR, "IOERROR: failed to stat fd %s: %m", fname);
        r = IMAP_IOERROR;
        goto done;
    }

    if (sbuf.st_size >= 8) {
        map_refresh(fd, 1, &base, &len, sbuf.st_size, "counters", mboxname);
        if (len >= 8)
            r = mboxname_buf_to_counters(base, len, vals);
        map_free(&base, &len);
    }

 done:
    if (fd != -1) close(fd);
    mbname_free(&mbname);
    free(fname);
    return r;
}

EXPORTED modseq_t mboxname_readmodseq(const char *mboxname)
{
    struct mboxname_counters counters;

    if (!config_getswitch(IMAPOPT_CONVERSATIONS))
        return 0;

    if (mboxname_read_counters(mboxname, &counters))
        return 0;

    return counters.highestmodseq;
}

EXPORTED modseq_t mboxname_nextmodseq(const char *mboxname, modseq_t last, int mbtype)
{
    struct mboxname_counters counters;
    modseq_t *typemodseqp;
    int fd = -1;

    if (!config_getswitch(IMAPOPT_CONVERSATIONS))
        return last + 1;

    /* XXX error handling */
    if (mboxname_load_counters(mboxname, &counters, &fd))
        return last + 1;

    if (mboxname_isaddressbookmailbox(mboxname, mbtype))
        typemodseqp = &counters.carddavmodseq;
    else if (mboxname_iscalendarmailbox(mboxname, mbtype))
        typemodseqp = &counters.caldavmodseq;
    else if (mboxname_isnotesmailbox(mboxname, mbtype))
        typemodseqp = &counters.notesmodseq;
    else
        typemodseqp = &counters.mailmodseq;

    if (counters.highestmodseq < last)
        counters.highestmodseq = last;

    counters.highestmodseq++;

    *typemodseqp = counters.highestmodseq;

    /* always set, because we always increased */
    mboxname_set_counters(mboxname, &counters, fd);

    return counters.highestmodseq;
}

EXPORTED modseq_t mboxname_setmodseq(const char *mboxname, modseq_t val, int mbtype)
{
    struct mboxname_counters counters;
    modseq_t *typemodseqp;
    int fd = -1;
    int dirty = 0;

    if (!config_getswitch(IMAPOPT_CONVERSATIONS))
        return val;

    /* XXX error handling */
    if (mboxname_load_counters(mboxname, &counters, &fd))
        return val;

    if (mboxname_isaddressbookmailbox(mboxname, mbtype))
        typemodseqp = &counters.carddavmodseq;
    else if (mboxname_iscalendarmailbox(mboxname, mbtype))
        typemodseqp = &counters.caldavmodseq;
    else if (mboxname_isnotesmailbox(mboxname, mbtype))
        typemodseqp = &counters.notesmodseq;
    else
        typemodseqp = &counters.mailmodseq;

    if (counters.highestmodseq < val) {
        counters.highestmodseq = val;
        dirty = 1;
    }

    if (*typemodseqp < val) {
        *typemodseqp = val;
        dirty = 1;
    }

    if (dirty)
        mboxname_set_counters(mboxname, &counters, fd);
    else
        mboxname_unload_counters(fd);

    return val;
}

EXPORTED uint32_t mboxname_readuidvalidity(const char *mboxname)
{
    struct mboxname_counters counters;

    if (!config_getswitch(IMAPOPT_CONVERSATIONS))
        return 0;

    if (mboxname_read_counters(mboxname, &counters))
        return 0;

    return counters.uidvalidity;
}

EXPORTED uint32_t mboxname_nextuidvalidity(const char *mboxname, uint32_t last, int mbtype __attribute__((unused)))
{
    struct mboxname_counters counters;
    int fd = -1;

    if (!config_getswitch(IMAPOPT_CONVERSATIONS))
        return last + 1;

    /* XXX error handling */
    if (mboxname_load_counters(mboxname, &counters, &fd))
        return last + 1;

    if (counters.uidvalidity < last)
        counters.uidvalidity = last;

    counters.uidvalidity++;

    /* always set, because we always increased */
    mboxname_set_counters(mboxname, &counters, fd);

    return counters.uidvalidity;
}

EXPORTED uint32_t mboxname_setuidvalidity(const char *mboxname, uint32_t val, int mbtype __attribute__((unused)))
{
    struct mboxname_counters counters;
    int fd = -1;
    int dirty = 0;

    if (!config_getswitch(IMAPOPT_CONVERSATIONS))
        return val;

    /* XXX error handling */
    if (mboxname_load_counters(mboxname, &counters, &fd))
        return val;

    if (counters.uidvalidity < val) {
        counters.uidvalidity = val;
        dirty = 1;
    }

    if (dirty)
        mboxname_set_counters(mboxname, &counters, fd);
    else
        mboxname_unload_counters(fd);

    return val;
}


