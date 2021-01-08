/* vcard_support.h -- Helper functions for vcard
 *
 * Copyright (c) 1994-2016 Carnegie Mellon University.  All rights reserved.
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

#include <config.h>

#include "vcard_support.h"
#include "syslog.h"

#include "global.h"

EXPORTED struct vparse_card *vcard_parse_string(const char *str)
{
    struct vparse_state vparser;
    struct vparse_card *vcard = NULL;
    int vr;

    memset(&vparser, 0, sizeof(struct vparse_state));

    vparser.base = str;
    vparse_set_multival(&vparser, "adr", ';');
    vparse_set_multival(&vparser, "org", ';');
    vparse_set_multival(&vparser, "n", ';');
    vparse_set_multival(&vparser, "nickname", ',');
    vparse_set_multival(&vparser, "categories", ',');
    vparse_set_multiparam(&vparser, "type");
    vr = vparse_parse(&vparser, 0);
    if (vr) {
        struct vparse_errorpos pos;
        vparse_fillpos(&vparser, &pos);
        if (pos.startpos < 60) {
            int len = pos.errorpos - pos.startpos;
            syslog(LOG_ERR, "vcard error %s at line %d char %d: %.*s ---> %.*s <---",
                   vparse_errstr(vr), pos.errorline, pos.errorchar,
                   pos.startpos, str, len, str + pos.startpos);
        }
        else if (pos.errorpos - pos.startpos < 40) {
            int len = pos.errorpos - pos.startpos;
            syslog(LOG_ERR, "vcard error %s at line %d char %d: ... %.*s ---> %.*s <---",
                   vparse_errstr(vr), pos.errorline, pos.errorchar,
                   40 - len, str + pos.errorpos - 40,
                   len, str + pos.startpos);
        }
        else {
            syslog(LOG_ERR, "error %s at line %d char %d: %.*s ... %.*s <--- (started at line %d char %d)",
                   vparse_errstr(vr), pos.errorline, pos.errorchar,
                   20, str + pos.startpos,
                   20, str + pos.errorpos - 20,
                   pos.startline, pos.startchar);
        }
    }
    else {
        vcard = vparser.card;
        vparser.card = NULL;
    }
    vparse_free(&vparser);

    return vcard;
}

EXPORTED struct vparse_card *vcard_parse_buf(const struct buf *buf)
{
    return vcard_parse_string(buf_cstring(buf));
}

EXPORTED struct buf *vcard_as_buf(struct vparse_card *vcard)
{
    struct buf *buf = buf_new();

    vparse_tobuf(vcard, buf);

    return buf;
}

EXPORTED struct vparse_card *record_to_vcard(struct mailbox *mailbox,
                                    const struct index_record *record)
{
    struct buf buf = BUF_INITIALIZER;
    struct vparse_card *vcard = NULL;

    /* Load message containing the resource and parse vcard data */
    if (!mailbox_map_record(mailbox, record, &buf)) {
        vcard = vcard_parse_string(buf_cstring(&buf) + record->header_size);
        buf_free(&buf);
    }

    return vcard;
}

/* Decode a base64-encoded binary vCard property and calculate a GUID.

   XXX  This currently assumes vCard v3.
*/
EXPORTED size_t vcard_prop_decode_value(struct vparse_entry *prop,
                                        struct buf *value,
                                        char **content_type,
                                        struct message_guid *guid)
{
    struct vparse_param *param;
    size_t size = 0;

    if (!prop) return 0;

    /* Make sure value=binary (default) and encoding=b (base64) */
    if ((!(param = vparse_get_param(prop, "value")) ||
         !strcasecmp("binary", param->value)) &&
        ((param = vparse_get_param(prop, "encoding")) &&
         !strcasecmp("b", param->value))) {

        char *decbuf = NULL;

        /* Decode property value */
        if (charset_decode_mimebody(prop->v.value, strlen(prop->v.value),
                                    ENCODING_BASE64,
                                    &decbuf, &size) == prop->v.value) return 0;

        if (content_type) {
            struct vparse_param *type = vparse_get_param(prop, "type");

            if (!type) *content_type = NULL;
            else {
                struct buf buf = BUF_INITIALIZER;

                lcase(type->value);
                if (strncmp(type->value, "image/", 6))
                    buf_setcstr(&buf, "image/");
                buf_appendcstr(&buf, type->value);

                *content_type = buf_release(&buf);
            }
        }

        if (guid) {
            /* Generate GUID from decoded property value */
            message_guid_generate(guid, decbuf, size);
        }

        if (value) {
            /* Return the value in the specified buffer */
            buf_setmap(value, decbuf, size);
        }
        free(decbuf);
    }

    return size;
}

