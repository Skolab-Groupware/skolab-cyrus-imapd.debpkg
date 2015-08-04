/*
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

/* cyrus includes */
#include "assert.h"
#include "exitcodes.h"
#include "global.h"
#include "index.h"
#include "imapurl.h"
#include "mailbox.h"
#include "mboxlist.h"
#include "util.h"
#include "xmalloc.h"
#include "xstrlcpy.h"

/* generated headers are not necessarily in current directory */
#include "imap/imap_err.h"

static int verbose = 0;

static int dump_me(const char *name, int matchlen, int maycreate, void *rock);
static void print_seq(const char *tag, const char *attrib,
                      unsigned *seq, int n);
static int usage(const char *name);

struct incremental_record {
    unsigned incruid;
};

int main(int argc, char *argv[])
{
    int option;
    int i;
    char *alt_config = NULL;
    struct incremental_record irec;

    if ((geteuid()) == 0 && (become_cyrus(/*is_master*/0) != 0)) {
        fatal("must run as the Cyrus user", EC_USAGE);
    }

    while ((option = getopt(argc, argv, "vC:")) != EOF) {
        switch (option) {
        case 'v':
            verbose++;
            break;

        case 'C': /* alt config file */
            alt_config = optarg;
            break;

        default:
            usage(argv[0]);
            break;
        }
    }

    if (optind == argc) {
        usage(argv[0]);
    }

    cyrus_init(alt_config, "dump", 0, CONFIG_NEED_PARTITION_DATA);
    mboxlist_init(0);
    mboxlist_open(NULL);

    irec.incruid = 0;
    strarray_t *array = strarray_new();
    for (i = optind; i < argc; i++) {
        strarray_append(array, argv[i]);
    }
    if (array->count)
        mboxlist_findallmulti(NULL, array, 1, 0, 0, dump_me, &irec);

    strarray_free(array);

    mboxlist_close();
    mboxlist_done();

    cyrus_done();

    return 0;
}

static int usage(const char *name)
{
    fprintf(stderr, "usage: %s [-v] [mboxpattern ...]\n", name);

    exit(EC_USAGE);
}

static void generate_boundary(char *boundary, size_t size)
{
    assert(size >= 100);

    snprintf(boundary, size, "dump-%ld-%ld-%ld",
             (long) getpid(), (long) time(NULL), (long) rand());
}

static search_expr_t *systemflag_match(int flag)
{
    search_expr_t *e = search_expr_new(NULL, SEOP_MATCH);
    e->attr = search_attr_find("systemflags");
    e->value.u = flag;
    return e;
}

static int dump_me(const char *name, int matchlen __attribute__((unused)),
                   int maycreate __attribute__((unused)), void *rock)
{
    int r;
    char boundary[128];
    struct imapurl url;
    char imapurl[MAX_MAILBOX_PATH+1];
    struct incremental_record *irec = (struct incremental_record *) rock;
    struct searchargs searchargs;
    struct index_state *state;
    unsigned *uids;
    unsigned *uidseq;
    int i, n, numuids;
    unsigned msgno;

    r = index_open(name, NULL, &state);
    if (r) {
        if (verbose) {
            printf("error opening %s: %s\n", name, error_message(r));
        }
        return 0;
    }

    generate_boundary(boundary, sizeof(boundary));

    printf("Content-Type: multipart/related; boundary=\"%s\"\n\n", boundary);

    printf("--%s\n", boundary);
    printf("Content-Type: text/xml\n");
    printf("IMAP-Dump-Version: 0\n");
    printf("\n");

    printf("<imapdump uniqueid=\"%s\">\n", state->mailbox->uniqueid);
    memset(&url, 0, sizeof(struct imapurl));
    url.server = config_servername;
    url.mailbox = name;
    imapurl_toURL(imapurl, &url);
    printf("  <mailbox-url>%s</mailbox-url>\n", imapurl);
    printf("  <incremental-uid>%d</incremental-uid>\n", irec->incruid);
    printf("  <nextuid>%u</nextuid>\n", state->mailbox->i.last_uid + 1);
    printf("\n");

    memset(&searchargs, 0, sizeof(struct searchargs));
    searchargs.root = search_expr_new(NULL, SEOP_TRUE);
    numuids = index_getuidsequence(state, &searchargs, &uids);
    print_seq("uidlist", NULL, uids, numuids);
    printf("\n");

    printf("  <flags>\n");

    searchargs.root = systemflag_match(FLAG_ANSWERED);
    n = index_getuidsequence(state, &searchargs, &uidseq);
    print_seq("flag", "name=\"\\Answered\" user=\"*\"", uidseq, n);
    if (uidseq) free(uidseq);

    searchargs.root = systemflag_match(FLAG_DELETED);
    n = index_getuidsequence(state, &searchargs, &uidseq);
    print_seq("flag", "name=\"\\Deleted\" user=\"*\"", uidseq, n);
    if (uidseq) free(uidseq);

    searchargs.root = systemflag_match(FLAG_DRAFT);
    n = index_getuidsequence(state, &searchargs, &uidseq);
    print_seq("flag", "name=\"\\Draft\" user=\"*\"", uidseq, n);
    if (uidseq) free(uidseq);

    searchargs.root = systemflag_match(FLAG_FLAGGED);
    n = index_getuidsequence(state, &searchargs, &uidseq);
    print_seq("flag", "name=\"\\Flagged\" user=\"*\"", uidseq, n);
    if (uidseq) free(uidseq);

    printf("  </flags>\n");

    printf("</imapdump>\n");

    i = 0;
    while (uids[i] < irec->incruid && i < numuids) {
        /* already dumped this message */
        /* xxx could do binary search to get to the first
           undumped uid */
        i++;
    }

    for (msgno = 1; msgno <= state->exists; msgno++) {
        struct buf buf = BUF_INITIALIZER;
        struct index_map *im = &state->map[msgno-1];
        struct index_record record;

        while (im->uid > uids[i] && i < numuids)
            i++;
        if (i >= numuids)
            break;

        if (im->uid < uids[i])
            continue;

        /* got a match */
        i++;
        memset(&record, 0, sizeof(struct index_record));
        record.recno = im->recno;
        record.uid = im->uid;
        if (mailbox_reload_index_record(state->mailbox, &record))
            continue;

        printf("\n--%s\n", boundary);
        printf("Content-Type: message/rfc822\n");
        printf("Content-ID: %d\n", uids[i]);
        printf("\n");
        r = mailbox_map_record(state->mailbox, &record, &buf);
        if (r) {
            if (verbose) {
                printf("error mapping message %u: %s\n", record.uid,
                       error_message(r));
            }
            break;
        }
        fwrite(buf.s, 1, buf.len, stdout);
        buf_free(&buf);
    }

    printf("\n--%s--\n", boundary);

    index_close(&state);

    return 0;
}

static void print_seq(const char *tag, const char *attrib,
                      unsigned *seq, int n)
{
    int i;

    printf("  <%s%s%s>", tag, attrib ? " " : "", attrib ? attrib : "");
    for (i = 0; i < n; i++) {
        printf("%u ", seq[i]);
    }
    printf("</%s>\n", tag);
}
