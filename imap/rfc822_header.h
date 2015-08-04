/* Automatically generated by compile_st.pl, do not edit */
#ifndef __STRING_TABLE_rfc822_header_H_
#define __STRING_TABLE_rfc822_header_H_ 1

#include <string.h>

enum rfc822_header {
    RFC822_BAD=-1,
    RFC822_UNKNOWN=-2,
    RFC822_BCC=0,
    RFC822_CC=1,
    RFC822_CONTENT_DESCRIPTION=2,
    RFC822_CONTENT_DISPOSITION=3,
    RFC822_CONTENT_ID=4,
    RFC822_CONTENT_LANGUAGE=5,
    RFC822_CONTENT_LOCATION=6,
    RFC822_CONTENT_MD5=7,
    RFC822_CONTENT_TRANSFER_ENCODING=8,
    RFC822_CONTENT_TYPE=9,
    RFC822_DATE=10,
    RFC822_FROM=11,
    RFC822_IN_REPLY_TO=12,
    RFC822_MESSAGE_ID=13,
    RFC822_REPLY_TO=14,
    RFC822_RECEIVED=15,
    RFC822_REFERENCES=16,
    RFC822_SUBJECT=17,
    RFC822_SENDER=18,
    RFC822_TO=19,
    RFC822_X_DELIVEREDINTERNALDATE=20,
    RFC822_X_ME_MESSAGE_ID=21,
};
extern enum rfc822_header rfc822_header_from_string(const char *s);
extern enum rfc822_header rfc822_header_from_string_len(const char *s, size_t len);
extern const char *rfc822_header_to_string(enum rfc822_header v);

#endif /* __STRING_TABLE_rfc822_header_H_ */
