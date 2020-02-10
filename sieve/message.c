/* message.c -- message parsing functions
 * Larry Greenfield
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "md5.h"
#include "sieve_interface.h"
#include "interp.h"
#include "message.h"
#include "xmalloc.h"
#include "util.h"

/* [e]reject message m with message msg
 *
 * incompatible with: fileinto, redirect
 */
int do_reject(action_list_t *a, int action, const char *msg)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        b = a;
        if (a->a == ACTION_FILEINTO ||
            a->a == ACTION_SNOOZE ||
            a->a == ACTION_KEEP ||
            a->a == ACTION_REDIRECT ||
            a->a == ACTION_REJECT ||
            a->a == ACTION_EREJECT ||
            a->a == ACTION_VACATION ||
            a->a == ACTION_SETFLAG ||
            a->a == ACTION_ADDFLAG ||
            a->a == ACTION_REMOVEFLAG ||
            a->a == ACTION_MARK ||
            a->a == ACTION_UNMARK
            )
            return SIEVE_RUN_ERROR;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = action;
    a->cancel_keep = 1;
    a->u.rej.msg = msg;
    a->u.rej.is_extended = (action == ACTION_EREJECT);

    b->next = a;

    return 0;
}

/* snooze message m
 *
 * incompatible with: [e]reject
 */
int do_snooze(action_list_t *a, const char *awaken_mbox, int is_mboxid,
              strarray_t *addflags, strarray_t *removeflags,
              unsigned char days, arrayu64_t *times,
              strarray_t *imapflags)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT) {
            strarray_free(addflags);
            strarray_free(removeflags);
            strarray_free(imapflags);
            arrayu64_free(times);
            return SIEVE_RUN_ERROR;
        }

        if (a->a == ACTION_SNOOZE) {
            /* don't bother doing it twice */
            /* check that we have a valid action */
            if (b == NULL) {
                strarray_free(addflags);
                strarray_free(removeflags);
                strarray_free(imapflags);
                arrayu64_free(times);
                return SIEVE_INTERNAL_ERROR;
            }

            /* cut this action out of the list */
            b->next = a->next;
            a->next = NULL;
            free_action_list(a);
            a = b;
        }

        b = a;
        a = a->next;
    }

    a = new_action_list();
    a->a = ACTION_SNOOZE;
    a->cancel_keep = 1;
    a->u.snz.awaken_mbox = awaken_mbox;
    a->u.snz.imapflags = imapflags;
    a->u.snz.addflags = addflags;
    a->u.snz.removeflags = removeflags;
    a->u.snz.days = days;
    a->u.snz.times = times;
    a->u.snz.is_mboxid = is_mboxid;

    b->next = a;

    return 0;
}

/* fileinto message m into mailbox
 *
 * incompatible with: [e]reject
 */
int do_fileinto(action_list_t *a, const char *mbox, const char *specialuse,
                int cancel_keep, int do_create, const char *mailboxid,
                strarray_t *imapflags)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT) {
            strarray_free(imapflags);
            return SIEVE_RUN_ERROR;
        }

        if (a->a == ACTION_FILEINTO && !strcmp(a->u.fil.mailbox, mbox)) {
            /* don't bother doing it twice */
            /* check that we have a valid action */
            if (b == NULL) {
                strarray_free(imapflags);
                return SIEVE_INTERNAL_ERROR;
            }

            /* cut this action out of the list */
            b->next = a->next;
            a->next = NULL;
            free_action_list(a);
            a = b;
        }

        b = a;
        a = a->next;
    }

    a = new_action_list();
    a->a = ACTION_FILEINTO;
    a->cancel_keep |= cancel_keep;
    a->u.fil.mailbox = mbox;
    a->u.fil.specialuse = specialuse;
    a->u.fil.imapflags = imapflags;
    a->u.fil.do_create = do_create;
    a->u.fil.mailboxid = mailboxid;

    b->next = a;

    return 0;
}

/* redirect message m to to addr
 *
 * incompatible with: [e]reject
 */
int do_redirect(action_list_t *a, const char *addr, const char *deliverby,
                const char *dsn_notify, const char *dsn_ret,
                int is_ext_list, int cancel_keep)
{
    action_list_t *b = NULL;

    /* xxx we should validate addr */

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT)
            return SIEVE_RUN_ERROR;

        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_REDIRECT;
    a->cancel_keep = cancel_keep;
    a->u.red.addr = addr;
    a->u.red.is_ext_list = is_ext_list;
    a->u.red.deliverby = deliverby;
    a->u.red.dsn_notify = dsn_notify;
    a->u.red.dsn_ret = dsn_ret;

    b->next = a;

    return 0;
}

/* keep message
 *
 * incompatible with: [e]reject
 */
int do_keep(action_list_t *a, strarray_t *imapflags)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT) {
            return SIEVE_RUN_ERROR;
            strarray_free(imapflags);
        }

        if (a->a == ACTION_KEEP) {
            /* don't bother doing it twice */
            /* check that we have a valid action */
            if (b == NULL) {
                strarray_free(imapflags);
                return SIEVE_INTERNAL_ERROR;
            }
            /* cut this action out of the list */
            b->next = a->next;
            a->next = NULL;
            free_action_list(a);
            a = b;
        }

        b = a;
        a = a->next;
    }

    a = new_action_list();
    a->a = ACTION_KEEP;
    a->cancel_keep = 1;
    a->u.keep.imapflags = imapflags;

    b->next = a;

    return 0;
}

/* discard message m
 *
 * incompatible with: nothing---it doesn't cancel any actions
 */
int do_discard(action_list_t *a)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_DISCARD) /* don't bother doing twice */
            return 0;

        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_DISCARD;
    a->cancel_keep = 1;

    b->next = a;

    return 0;
}

static int makehash(unsigned char hash[],
                    const char *s1, const char *s2, const char *s3)
{
    MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx, s1, strlen(s1));
    MD5Update(&ctx, s2, strlen(s2));
    if (s3) MD5Update(&ctx, s3, strlen(s3));
    MD5Final(hash, &ctx);

    return SIEVE_OK;
}

int do_vacation(action_list_t *a, char *addr, char *fromaddr,
                char *subj, const char *msg, int seconds,
                int mime, const char *handle,
                const sieve_fileinto_context_t *fcc)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT ||
            a->a == ACTION_VACATION) /* vacation can't be used twice */
            return SIEVE_RUN_ERROR;

        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_VACATION;
    a->u.vac.send.addr = addr;
    a->u.vac.send.fromaddr = fromaddr;
    a->u.vac.send.subj = subj;  /* user specified subject */
    a->u.vac.send.msg = msg;
    a->u.vac.send.mime = mime;
    a->u.vac.send.fcc.mailbox = fcc->mailbox;
    a->u.vac.send.fcc.specialuse = fcc->specialuse;
    a->u.vac.send.fcc.do_create = fcc->do_create;
    a->u.vac.send.fcc.imapflags = fcc->imapflags;
    if (handle)
        makehash(a->u.vac.autoresp.hash, addr, handle, NULL);
    else
        makehash(a->u.vac.autoresp.hash, addr, fromaddr, msg);
    a->u.vac.autoresp.seconds = seconds;

    b->next = a;

    return 0;
}

/* mark message m
 *
 * incompatible with: [e]reject
 */
int do_mark(action_list_t *a)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT)
            return SIEVE_RUN_ERROR;

        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_MARK;

    b->next = a;

    return 0;
}


/* unmark message m
 *
 * incompatible with: [e]reject
 */
int do_unmark(action_list_t *a)
{

    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        if (a->a == ACTION_REJECT || a->a == ACTION_EREJECT)
            return SIEVE_RUN_ERROR;

        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_UNMARK;

    b->next = a;

    return 0;
}

/* (e)notify
 *
 * incompatible with: none
 */
int do_notify(notify_list_t *n, const char *id, const char *from,
              const char *method, const char **options,
              const char *priority, const char *message)
{
    notify_list_t *b = NULL;

    /* find the end of the notify list */
    while (n != NULL) {
        b = n;
        n = n->next;
    }

    /* add to the notify list */
    n = new_notify_list();
    n->isactive = 1;
    n->id = id;
    n->from = from;
    n->method = method;
    n->options = options;
    n->priority = priority;
    n->message = message;

    b->next = n;

    return 0;
}

/* denotify
 *
 * incompatible with: none
 */
int do_denotify(notify_list_t *n, comparator_t *comp, const void *pat,
                strarray_t *match_vars, void *comprock, const char *priority)
{
    while (n != NULL) {
        if (n->isactive &&
            (!priority || !strcasecmp(n->priority, priority)) &&
            (!comp ||
             (n->id && comp(n->id, strlen(n->id), pat, match_vars, comprock)))) {
            n->isactive = 0;
        }
        n = n->next;
    }

    return 0;
}

/* add a header to the message
 *
 * incompatible with: none
 */
int do_addheader(action_list_t *a, const char *name, const char *value,
                 int index)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_ADDHEADER;
    a->u.addh.name = name;
    a->u.addh.value = value;
    a->u.addh.index = index;

    b->next = a;

    return 0;
}

/* delete a header from the message
 *
 * incompatible with: none
 */
int do_deleteheader(action_list_t *a, const char *name, void *patterns,
                    int index, int comptype,
                    comparator_t *compfunc, void *comprock)
{
    action_list_t *b = NULL;

    /* see if this conflicts with any previous actions taken on this message */
    while (a != NULL) {
        b = a;
        a = a->next;
    }

    /* add to the action list */
    a = new_action_list();
    a->a = ACTION_DELETEHEADER;
    a->u.delh.name = name;
    a->u.delh.patterns = patterns;
    a->u.delh.index = index;
    a->u.delh.comptype = comptype;
    a->u.delh.compfunc = compfunc;
    a->u.delh.comprock = comprock;

    b->next = a;

    return 0;
}

int do_duptrack(duptrack_list_t *d, sieve_duplicate_context_t *dc)
{
    duptrack_list_t *b = NULL;

    /* find the end of the duptrack list */
    while (d != NULL) {
        b = d;
        d = d->next;
    }

    /* add to the duptrack list */
    d = new_duptrack_list();
    d->id = dc->id;
    d->seconds = dc->seconds;

    b->next = d;

    return 0;
}

notify_list_t *new_notify_list(void)
{
    return (notify_list_t *) xzmalloc(sizeof(notify_list_t));
}

void free_notify_list(notify_list_t *n)
{
    while (n) {
        notify_list_t *b = n->next;
        free(n->options); /* strings live in bytecode, only free the array */
        free(n);
        n = b;
    }
}

action_list_t *new_action_list(void)
{
    return (action_list_t *) xzmalloc(sizeof(action_list_t));
}

void free_action_list(action_list_t *a)
{
    while (a) {
        action_list_t *b = a->next;

        switch (a->a) {
        case ACTION_FILEINTO:
            strarray_free(a->u.fil.imapflags);
            break;

        case ACTION_SNOOZE:
            strarray_free(a->u.snz.imapflags);
            strarray_free(a->u.snz.addflags);
            strarray_free(a->u.snz.removeflags);
            arrayu64_free(a->u.snz.times);
            break;

        case ACTION_KEEP:
            strarray_free(a->u.keep.imapflags);
            break;

        case ACTION_VACATION:
            if(a->u.vac.send.subj) free(a->u.vac.send.subj);
            if(a->u.vac.send.addr) free(a->u.vac.send.addr);
            if(a->u.vac.send.fromaddr) free(a->u.vac.send.fromaddr);
            strarray_free(a->u.vac.send.fcc.imapflags);
            break;

        case ACTION_DELETEHEADER:
            ptrarray_free(a->u.delh.patterns);
            break;

        default:
            break;
        }

        free(a);
        a = b;
    }
}

duptrack_list_t *new_duptrack_list(void)
{
    return (duptrack_list_t *) xzmalloc(sizeof(duptrack_list_t));
}

void free_duptrack_list(duptrack_list_t *d)
{
    while (d) {
        duptrack_list_t *b = d->next;
        free(d->id);
        free(d);
        d = b;
    }
}
