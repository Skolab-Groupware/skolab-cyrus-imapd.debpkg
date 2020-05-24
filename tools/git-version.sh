#!/bin/bash
# Generate a version string for cyrus-imapd
#
# Copyright (c) 1994-2016 Carnegie Mellon University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The name "Carnegie Mellon University" must not be used to
#    endorse or promote products derived from this software without
#    prior written permission. For permission or any legal
#    details, please contact
#      Carnegie Mellon University
#      Center for Technology Transfer and Enterprise Creation
#      4615 Forbes Avenue
#      Suite 302
#      Pittsburgh, PA  15213
#      (412) 268-7393, fax: (412) 268-7395
#      innovation@andrew.cmu.edu
#
# 4. Redistributions of any form whatsoever must retain the following
#    acknowledgment:
#    "This product includes software developed by Computing Services
#     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
#
# CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
# AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# if we've come from a release package, ignore git entirely
test -s VERSION &&
    exec head -1 VERSION

# first try: based on annotated git tags (real releases)
version=$(git describe --dirty=-dirty --match 'cyrus-imapd-*' 2>/dev/null)
test -n "$version" &&
    version=$(echo "$version" | sed -e 's/^cyrus-imapd-//')

# second try: lightweight git tags
test -z "$version" &&
    version=$(git describe --dirty=-dirty --tags 2>/dev/null)

# third try: probably not a git repository
test -z "$version" &&
    version=unknown

echo $version
