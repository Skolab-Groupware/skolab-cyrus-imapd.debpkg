/* ctl_cyrusdb.c -- Program to perform operations common to all cyrus DBs
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include "annotate.h"
#include "cyrusdb.h"
#include "duplicate.h"
#include "global.h"
#include "exitcodes.h"
#include "libcyr_cfg.h"
#include "mboxlist.h"
#include "seen.h"
#include "statuscache.h"
#include "tls.h"
#include "util.h"
#include "xmalloc.h"
#include "xstrlcpy.h"

#define N(a) (sizeof(a) / sizeof(a[0]))

static struct cyrusdb {
    const char *name;
    const char **configptr;
    cyrusdb_archiver *archiver;
    int doarchive;
} dblist[] = {
    { FNAME_MBOXLIST,           &config_mboxlist_db,    NULL,   1 },
    { FNAME_QUOTADB,            &config_quota_db,       NULL,   1 },
    { FNAME_GLOBALANNOTATIONS,  &config_annotation_db,  NULL,   1 },
    { FNAME_DELIVERDB,          &config_duplicate_db,   NULL,   0 },
    { FNAME_TLSSESSIONS,        &config_tls_sessions_db,NULL,   0 },
    { FNAME_PTSDB,              &config_ptscache_db,    NULL,   0 },
    { FNAME_STATUSCACHEDB,      &config_statuscache_db, NULL,   0 },
    { NULL,                     NULL,                   NULL,   0 }
};

static int compdb(const void *v1, const void *v2)
{
    struct cyrusdb *db1 = (struct cyrusdb *) v1;
    struct cyrusdb *db2 = (struct cyrusdb *) v2;

    /* compare archive pointers for sort */
    return ((char *)db1->archiver - (char *)db2->archiver);
}

static void usage(void)
{
    fprintf(stderr, "ctl_cyrusdb [-C <altconfig>] -c\n");
    fprintf(stderr, "ctl_cyrusdb [-C <altconfig>] -r [-x]\n");
    exit(-1);
}

/* Callback for use by recover_reserved */
static int fixmbox(const mbentry_t *mbentry,
                   void *rock __attribute__((unused)))
{
    int r;

    if (mbentry->legacy_specialuse) {
        char *userid = mboxname_to_userid(mbentry->name);
        if (userid) {
            struct buf buf = BUF_INITIALIZER;
            buf_setcstr(&buf, mbentry->legacy_specialuse);
            annotatemore_rawwrite(mbentry->name, "/specialuse", userid, &buf);
            buf_free(&buf);
            free(userid);
        }
        mbentry_t *copy = mboxlist_entry_copy(mbentry);
        /* XXX - const correctness */
        free(copy->legacy_specialuse);
        copy->legacy_specialuse = NULL;
        mboxlist_update(copy, /*localonly*/1);
        mboxlist_entry_free(&copy);
    }

    /* if MBTYPE_RESERVED, unset it & call mboxlist_delete */
    if (mbentry->mbtype & MBTYPE_RESERVE) {
        r = mboxlist_deletemailbox(mbentry->name, 1, NULL, NULL, NULL, 0, 0, 1);
        if (r) {
            /* log the error */
            syslog(LOG_ERR,
                   "could not remove reserved mailbox '%s': %s",
                   mbentry->name, error_message(r));
        } else {
            syslog(LOG_NOTICE,
                   "removed reserved mailbox '%s'",
                   mbentry->name);
        }
    }

    return 0;
}

static void recover_reserved(void)
{
    mboxlist_init(0);
    mboxlist_open(NULL);

    /* Need annotations.db for mboxlist_deletemailbox() and also
     * for fixing legacy specialuse support */
    annotate_init(NULL, NULL);
    annotatemore_open();

    /* Need quotadb for deleting mailboxes with quotas */
    quotadb_init(0);
    quotadb_open(NULL);

    /* build a list of mailboxes - we're using internal names here */
    mboxlist_allmbox(NULL, fixmbox, NULL, 0);

    quotadb_close();
    quotadb_done();

    annotatemore_close();
    annotate_done();

    mboxlist_close();
    mboxlist_done();
}

static const char *dbfname(struct cyrusdb *db)
{
    static char buf[MAX_MAILBOX_PATH];
    snprintf(buf, MAX_MAILBOX_PATH, "%s%s", config_dir, db->name);
    return buf;
}

static void check_convert(struct cyrusdb *db, const char *fname)
{
    const char *detectname = cyrusdb_detect(fname);
    char backendbuf[100];
    char *p;
    int r;

    /* unable to detect current type, assume all is good */
    if (!detectname) return;

    /* strip the -nosync from the name if present */
    xstrncpy(backendbuf, *db->configptr, 100);
    p = strstr(backendbuf, "-nosync");
    if (p) *p = '\0';

    /* ignore files that are already the right type */
    if (!strcmp(backendbuf, detectname)) return;

    /* otherwise we need to upgrade! */
    syslog(LOG_NOTICE, "converting %s from %s to %s",
           fname, detectname, *db->configptr);

    r = cyrusdb_convert(fname, fname, detectname, *db->configptr);
    if (r)
        syslog(LOG_NOTICE, "conversion failed %s", fname);
}

int main(int argc, char *argv[])
{
    extern char *optarg;
    int opt, r, r2;
    char *alt_config = NULL;
    int reserve_flag = 1;
    enum { RECOVER, CHECKPOINT, NONE } op = NONE;
    char *dirname = NULL, *backup1 = NULL, *backup2 = NULL;
    strarray_t files = STRARRAY_INITIALIZER;
    char *msg = "";
    int i, rotated = 0;

    if ((geteuid()) == 0 && (become_cyrus(/*is_master*/0) != 0)) {
        fatal("must run as the Cyrus user", EC_USAGE);
    }
    r = r2 = 0;

    while ((opt = getopt(argc, argv, "C:rxc")) != EOF) {
        switch (opt) {
        case 'C': /* alt config file */
            alt_config = optarg;
            break;

        case 'r':
            libcyrus_config_setint(CYRUSOPT_DB_INIT_FLAGS, CYRUSDB_RECOVER);
            msg = "recovering cyrus databases";
            if (op == NONE) op = RECOVER;
            else usage();
            break;

        case 'c':
            msg = "checkpointing cyrus databases";
            if (op == NONE) op = CHECKPOINT;
            else usage();
            break;

        case 'x':
            reserve_flag = 0;
            break;

        default:
            usage();
            break;
        }
    }

    if (op == NONE || (op != RECOVER && !reserve_flag)) {
        usage();
        /* NOTREACHED */
    }

    cyrus_init(alt_config, "ctl_cyrusdb", 0, 0);

    /* create the name of the db directory */
    /* (used by backup directory names) */
    dirname = strconcat(config_dir, FNAME_DBDIR, (char *)NULL);

    /* create the names of the backup directories */
    backup1 = strconcat(dirname, ".backup1", (char *)NULL);
    backup2 = strconcat(dirname, ".backup2", (char *)NULL);

    syslog(LOG_NOTICE, "%s", msg);

    /* detect backends */
    for (i = 0; dblist[i].name != NULL; i++)
        dblist[i].archiver = cyrusdb_getarchiver(*dblist[i].configptr);

    /* sort dbenvs */
    qsort(dblist, N(dblist)-1, sizeof(struct cyrusdb), &compdb);

    for (i = 0; dblist[i].name; i++) {
        const char *fname = dbfname(&dblist[i]);

        if (op == RECOVER)
            check_convert(&dblist[i], fname);

        /* if we need to archive this db, add it to the list */
        if (dblist[i].doarchive)
            strarray_add(&files, fname);

        /* deal with each dbenv once */
        if (dblist[i+1].archiver == dblist[i].archiver)
            continue;

        r = r2 = 0;
        switch (op) {
        case RECOVER:
            break;

        case CHECKPOINT:
            r2 = cyrusdb_sync(*dblist[i].configptr);
            if (r2) {
                syslog(LOG_ERR, "DBERROR: sync %s: %s", dirname,
                       cyrusdb_strerror(r2));
                fprintf(stderr,
                        "ctl_cyrusdb: unable to sync environment\n");
            }

            /* ARCHIVE */
            r2 = 0;

            if (!rotated) {
                /* rotate the backup directories -- ONE time only */
                char *file;
                DIR *dirp;
                struct dirent *dirent;

                /* remove db.backup2 */
                dirp = opendir(backup2);

                if (dirp) {
                    while ((dirent = readdir(dirp)) != NULL) {
                        if (dirent->d_name[0] == '.') continue;
                        file = strconcat(backup2, "/", dirent->d_name, (char *)NULL);
                        unlink(file);
                        free(file);
                    }

                    closedir(dirp);
                }
                r2 = rmdir(backup2);

                /* move db.backup1 to db.backup2 */
                if (r2 == 0 || errno == ENOENT)
                    r2 = rename(backup1, backup2);

                /* make a new db.backup1 */
                if (r2 == 0 || errno == ENOENT)
                    r2 = mkdir(backup1, 0755);

                rotated = 1;
            }

            /* do the archive */
            if (r2 == 0)
                r2 = dblist[i].archiver(&files, backup1);

            if (r2) {
                syslog(LOG_ERR, "DBERROR: archive %s: %s", dirname,
                       cyrusdb_strerror(r2));
                fprintf(stderr,
                        "ctl_cyrusdb: unable to archive environment\n");
            }


            break;

        default:
            break;
        }

        strarray_truncate(&files, 0);
    }

    strarray_fini(&files);

    if(op == RECOVER && reserve_flag)
        recover_reserved();

    free(dirname);
    free(backup1);
    free(backup2);
    cyrus_done();

    syslog(LOG_NOTICE, "done %s", msg);
    exit(r || r2);
}
