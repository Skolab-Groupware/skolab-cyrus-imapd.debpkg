/* bsearch.h -- binary search
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

#ifndef INCLUDED_BSEARCH_H
#define INCLUDED_BSEARCH_H

extern int bsearch_mem_mbox(const char *word,
                            const char *base, unsigned long len,
                            unsigned long hint,
                            unsigned long *linelenp);

extern int bsearch_compare_mbox(const char *s1, const char *s2);

extern int bsearch_ncompare_mbox(const char *s1, int l1, const char *s2, int l2);
extern int bsearch_uncompare_mbox(const unsigned char *s1, size_t l1,
                                  const unsigned char *s2, size_t l2);
extern int bsearch_memtree_mbox(const unsigned char *s1, size_t l1,
                                const unsigned char *s2, size_t l2);

extern int bsearch_ncompare_raw(const char *s1, int l1, const char *s2, int l2);

extern int cmpstringp_raw(const void *p1, const void *p2);
extern int cmpstringp_mbox(const void *p1, const void *p2);

#endif /* INCLUDED_BSEARCH_H */
