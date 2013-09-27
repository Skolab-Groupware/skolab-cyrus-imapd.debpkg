/* auto-generated by config2header 1.17 */

/* DO NOT EDIT */

/* THIS FILE AUTOMATICALLY GENERATED BY config2header 1.17 */

#include <config.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H

#include <unistd.h>

#endif

#include <syslog.h>

#include <stdlib.h>

#include <string.h>

#include "imapopts.h"



struct imapopt_s imapopts[] =

{

  { IMAPOPT_ZERO, "", 0, OPT_NOTOPT, { NULL }, { { NULL, IMAP_ENUM_ZERO } } },

  { IMAPOPT_ADDRESSBOOKPREFIX, "addressbookprefix", 0, OPT_STRING,
    {(void *)("#addressbooks")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ADMINS, "admins", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_AFSPTS_LOCALREALMS, "afspts_localrealms", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_AFSPTS_MYCELL, "afspts_mycell", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALLOWALLSUBSCRIBE, "allowallsubscribe", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALLOWANONYMOUSLOGIN, "allowanonymouslogin", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALLOWAPOP, "allowapop", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALLOWNEWNEWS, "allownewnews", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALLOWPLAINTEXT, "allowplaintext", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALLOWUSERMOVES, "allowusermoves", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ALTNAMESPACE, "altnamespace", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ANNOTATION_DB, "annotation_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ANNOTATION_DB_PATH, "annotation_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ANYONEUSERACL, "anyoneuseracl", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_ANNOTATION_DEFINITIONS, "annotation_definitions", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_AUDITLOG, "auditlog", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_AUTH_MECH, "auth_mech", 0, OPT_STRINGLIST,
    {(void*)("unix")},
    { { "unix" , IMAP_ENUM_ZERO },
      { "pts" , IMAP_ENUM_ZERO },
      { "krb" , IMAP_ENUM_ZERO },
      { "krb5" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_AUTOCREATEQUOTA, "autocreatequota", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_BERKELEY_CACHESIZE, "berkeley_cachesize", 0, OPT_INT,
    {(void*)512},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_BERKELEY_LOCKS_MAX, "berkeley_locks_max", 0, OPT_INT,
    {(void*)50000},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_BERKELEY_TXNS_MAX, "berkeley_txns_max", 0, OPT_INT,
    {(void*)100},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_BOUNDARY_LIMIT, "boundary_limit", 0, OPT_INT,
    {(void*)1000},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_CALDAV_ALLOWSCHEDULING, "caldav_allowscheduling", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_CALDAV_REALM, "caldav_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_CALENDARPREFIX, "calendarprefix", 0, OPT_STRING,
    {(void *)("#calendars")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_CARDDAV_REALM, "carddav_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_CLIENT_TIMEOUT, "client_timeout", 0, OPT_INT,
    {(void*)10},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_COMMANDMINTIMER, "commandmintimer", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_CONFIGDIRECTORY, "configdirectory", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DAV_REALM, "dav_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DEBUG_COMMAND, "debug_command", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DEFAULTACL, "defaultacl", 0, OPT_STRING,
    {(void *)("anyone lrs")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DEFAULTDOMAIN, "defaultdomain", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DEFAULTPARTITION, "defaultpartition", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DEFAULTSERVER, "defaultserver", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DELETEDPREFIX, "deletedprefix", 0, OPT_STRING,
    {(void *)("DELETED")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DELETE_MODE, "delete_mode", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_DELETE_MODE_IMMEDIATE)},
    { { "immediate" , IMAP_ENUM_DELETE_MODE_IMMEDIATE },
      { "delayed" , IMAP_ENUM_DELETE_MODE_DELAYED },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DELETERIGHT, "deleteright", 0, OPT_STRING,
    {(void *)("c")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DISABLE_USER_NAMESPACE, "disable_user_namespace", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DISABLE_SHARED_NAMESPACE, "disable_shared_namespace", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DISCONNECT_ON_VANISHED_MAILBOX, "disconnect_on_vanished_mailbox", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DKIM_DOMAIN, "dkim_domain", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DKIM_SELECTOR, "dkim_selector", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DUPLICATE_DB, "duplicate_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-nosync" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "berkeley-hash-nosync" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { "sql" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DUPLICATE_DB_PATH, "duplicate_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_DUPLICATESUPPRESSION, "duplicatesuppression", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_EXPUNGE_MODE, "expunge_mode", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_EXPUNGE_MODE_DEFAULT)},
    { { "default" , IMAP_ENUM_EXPUNGE_MODE_DEFAULT },
      { "immediate" , IMAP_ENUM_EXPUNGE_MODE_IMMEDIATE },
      { "delayed" , IMAP_ENUM_EXPUNGE_MODE_DELAYED },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_EXPUNGE_DAYS, "expunge_days", 0, OPT_INT,
    {(void*)7},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_FAILEDLOGINPAUSE, "failedloginpause", 0, OPT_INT,
    {(void*)3},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_FLUSHSEENSTATE, "flushseenstate", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_FOOLSTUPIDCLIENTS, "foolstupidclients", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_FORCE_SASL_CLIENT_MECH, "force_sasl_client_mech", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_FULLDIRHASH, "fulldirhash", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HASHIMAPSPOOL, "hashimapspool", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPALLOWCOMPRESS, "httpallowcompress", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPALLOWCORS, "httpallowcors", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPALLOWTRACE, "httpallowtrace", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPALLOWEDURLS, "httpallowedurls", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPCONTENTMD5, "httpcontentmd5", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPDOCROOT, "httpdocroot", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPKEEPALIVE, "httpkeepalive", 0, OPT_INT,
    {(void*)20},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPMODULES, "httpmodules", 0, OPT_BITFIELD,
    {(void *)(0)},
    { { "caldav" , IMAP_ENUM_HTTPMODULES_CALDAV },
      { "carddav" , IMAP_ENUM_HTTPMODULES_CARDDAV },
      { "domainkey" , IMAP_ENUM_HTTPMODULES_DOMAINKEY },
      { "ischedule" , IMAP_ENUM_HTTPMODULES_ISCHEDULE },
      { "rss" , IMAP_ENUM_HTTPMODULES_RSS },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPPRETTYTELEMETRY, "httpprettytelemetry", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_HTTPTIMEOUT, "httptimeout", 0, OPT_INT,
    {(void*)5},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IDLESOCKET, "idlesocket", 0, OPT_STRING,
    {(void *)("{configdirectory}/socket/idle")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IGNOREREFERENCE, "ignorereference", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IMAPIDLEPOLL, "imapidlepoll", 0, OPT_INT,
    {(void*)60},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IMAPIDRESPONSE, "imapidresponse", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IMAPMAGICPLUS, "imapmagicplus", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IMPLICIT_OWNER_RIGHTS, "implicit_owner_rights", 0, OPT_STRING,
    {(void *)("lkxa")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_IMPROVED_MBOXLIST_SORT, "improved_mboxlist_sort", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_INTERNALDATE_HEURISTIC, "internaldate_heuristic", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_INTERNALDATE_HEURISTIC_STANDARD)},
    { { "standard" , IMAP_ENUM_INTERNALDATE_HEURISTIC_STANDARD },
      { "receivedheader" , IMAP_ENUM_INTERNALDATE_HEURISTIC_RECEIVEDHEADER },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_AUTHZ, "ldap_authz", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_BASE, "ldap_base", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_BIND_DN, "ldap_bind_dn", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_DEREF, "ldap_deref", 0, OPT_STRINGLIST,
    {(void*)("never")},
    { { "search" , IMAP_ENUM_ZERO },
      { "find" , IMAP_ENUM_ZERO },
      { "always" , IMAP_ENUM_ZERO },
      { "never" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_FILTER, "ldap_filter", 0, OPT_STRING,
    {(void *)("(uid=%u)")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_GROUP_BASE, "ldap_group_base", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_GROUP_FILTER, "ldap_group_filter", 0, OPT_STRING,
    {(void *)("(cn=%u)")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_GROUP_SCOPE, "ldap_group_scope", 0, OPT_STRINGLIST,
    {(void*)("sub")},
    { { "sub" , IMAP_ENUM_ZERO },
      { "one" , IMAP_ENUM_ZERO },
      { "base" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_ID, "ldap_id", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_MECH, "ldap_mech", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_MEMBER_ATTRIBUTE, "ldap_member_attribute", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_MEMBER_BASE, "ldap_member_base", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_MEMBER_FILTER, "ldap_member_filter", 0, OPT_STRING,
    {(void *)("(member=%D)")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_MEMBER_METHOD, "ldap_member_method", 0, OPT_STRINGLIST,
    {(void*)("attribute")},
    { { "attribute" , IMAP_ENUM_ZERO },
      { "filter" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_MEMBER_SCOPE, "ldap_member_scope", 0, OPT_STRINGLIST,
    {(void*)("sub")},
    { { "sub" , IMAP_ENUM_ZERO },
      { "one" , IMAP_ENUM_ZERO },
      { "base" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_PASSWORD, "ldap_password", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_REALM, "ldap_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_REFERRALS, "ldap_referrals", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_RESTART, "ldap_restart", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SASL, "ldap_sasl", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SASL_AUTHC, "ldap_sasl_authc", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SASL_AUTHZ, "ldap_sasl_authz", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SASL_MECH, "ldap_sasl_mech", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SASL_PASSWORD, "ldap_sasl_password", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SASL_REALM, "ldap_sasl_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SCOPE, "ldap_scope", 0, OPT_STRINGLIST,
    {(void*)("sub")},
    { { "sub" , IMAP_ENUM_ZERO },
      { "one" , IMAP_ENUM_ZERO },
      { "base" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SERVERS, "ldap_servers", 0, OPT_STRING,
    {(void *)("ldap://localhost/")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_SIZE_LIMIT, "ldap_size_limit", 0, OPT_INT,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_START_TLS, "ldap_start_tls", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TIME_LIMIT, "ldap_time_limit", 0, OPT_INT,
    {(void*)5},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TIMEOUT, "ldap_timeout", 0, OPT_INT,
    {(void*)5},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TLS_CACERT_DIR, "ldap_tls_cacert_dir", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TLS_CACERT_FILE, "ldap_tls_cacert_file", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TLS_CERT, "ldap_tls_cert", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TLS_CHECK_PEER, "ldap_tls_check_peer", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TLS_CIPHERS, "ldap_tls_ciphers", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_TLS_KEY, "ldap_tls_key", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_URI, "ldap_uri", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LDAP_VERSION, "ldap_version", 0, OPT_INT,
    {(void*)3},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTP_DOWNCASE_RCPT, "lmtp_downcase_rcpt", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTP_FUZZY_MAILBOX_MATCH, "lmtp_fuzzy_mailbox_match", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTP_OVER_QUOTA_PERM_FAILURE, "lmtp_over_quota_perm_failure", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTP_STRICT_QUOTA, "lmtp_strict_quota", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTP_STRICT_RFC2821, "lmtp_strict_rfc2821", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTPSOCKET, "lmtpsocket", 0, OPT_STRING,
    {(void *)("{configdirectory}/socket/lmtp")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LMTPTXN_TIMEOUT, "lmtptxn_timeout", 0, OPT_INT,
    {(void*)300},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LOGINREALMS, "loginrealms", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LOGINUSEACL, "loginuseacl", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_LOGTIMESTAMPS, "logtimestamps", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MAILBOX_DEFAULT_OPTIONS, "mailbox_default_options", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MAILNOTIFIER, "mailnotifier", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MAXHEADERLINES, "maxheaderlines", 0, OPT_INT,
    {(void*)1000},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MAXMESSAGESIZE, "maxmessagesize", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MAXQUOTED, "maxquoted", 0, OPT_INT,
    {(void*)131072},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MAXWORD, "maxword", 0, OPT_INT,
    {(void*)131072},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MBOXKEY_DB, "mboxkey_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "berkeley" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MBOXLIST_DB, "mboxlist_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "flat" , IMAP_ENUM_ZERO },
      { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { "sql" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MBOXLIST_DB_PATH, "mboxlist_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MBOXNAME_LOCKPATH, "mboxname_lockpath", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_METAPARTITION_FILES, "metapartition_files", 0, OPT_BITFIELD,
    {(void *)(0)},
    { { "header" , IMAP_ENUM_METAPARTITION_FILES_HEADER },
      { "index" , IMAP_ENUM_METAPARTITION_FILES_INDEX },
      { "cache" , IMAP_ENUM_METAPARTITION_FILES_CACHE },
      { "expunge" , IMAP_ENUM_METAPARTITION_FILES_EXPUNGE },
      { "squat" , IMAP_ENUM_METAPARTITION_FILES_SQUAT },
      { "lock" , IMAP_ENUM_METAPARTITION_FILES_LOCK },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_AUTHNAME, "mupdate_authname", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_CONFIG, "mupdate_config", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_MUPDATE_CONFIG_STANDARD)},
    { { "standard" , IMAP_ENUM_MUPDATE_CONFIG_STANDARD },
      { "unified" , IMAP_ENUM_MUPDATE_CONFIG_UNIFIED },
      { "replicated" , IMAP_ENUM_MUPDATE_CONFIG_REPLICATED },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUNGE8BIT, "munge8bit", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_CONNECTIONS_MAX, "mupdate_connections_max", 0, OPT_INT,
    {(void*)128},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_PASSWORD, "mupdate_password", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_PORT, "mupdate_port", 0, OPT_INT,
    {(void*)3905},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_REALM, "mupdate_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_RETRY_DELAY, "mupdate_retry_delay", 0, OPT_INT,
    {(void*)20},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_SERVER, "mupdate_server", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_USERNAME, "mupdate_username", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_WORKERS_MAX, "mupdate_workers_max", 0, OPT_INT,
    {(void*)50},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_WORKERS_MAXSPARE, "mupdate_workers_maxspare", 0, OPT_INT,
    {(void*)10},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_WORKERS_MINSPARE, "mupdate_workers_minspare", 0, OPT_INT,
    {(void*)2},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_MUPDATE_WORKERS_START, "mupdate_workers_start", 0, OPT_INT,
    {(void*)5},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NETSCAPEURL, "netscapeurl", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSADDHEADERS, "newsaddheaders", 0, OPT_BITFIELD,
    {(void *)(IMAP_ENUM_NEWSADDHEADERS_TO
			 | 0)},
    { { "to" , IMAP_ENUM_NEWSADDHEADERS_TO },
      { "replyto" , IMAP_ENUM_NEWSADDHEADERS_REPLYTO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSGROUPS, "newsgroups", 0, OPT_STRING,
    {(void *)("*")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSMASTER, "newsmaster", 0, OPT_STRING,
    {(void *)("news")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSPEER, "newspeer", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSPOSTUSER, "newspostuser", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSPREFIX, "newsprefix", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NEWSRC_DB_PATH, "newsrc_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NNTPTIMEOUT, "nntptimeout", 0, OPT_INT,
    {(void*)3},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NOTIFYSOCKET, "notifysocket", 0, OPT_STRING,
    {(void *)("{configdirectory}/socket/notify")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_NOTIFY_EXTERNAL, "notify_external", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PLAINTEXTLOGINPAUSE, "plaintextloginpause", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PLAINTEXTLOGINALERT, "plaintextloginalert", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPEXPIRETIME, "popexpiretime", 0, OPT_INT,
    {(void*)-1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPMINPOLL, "popminpoll", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPSUBFOLDERS, "popsubfolders", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPPOLLPADDING, "poppollpadding", 0, OPT_INT,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPTIMEOUT, "poptimeout", 0, OPT_INT,
    {(void*)10},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPUSEACL, "popuseacl", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POPUSEIMAPFLAGS, "popuseimapflags", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POSTMASTER, "postmaster", 0, OPT_STRING,
    {(void *)("postmaster")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POSTSPEC, "postspec", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_POSTUSER, "postuser", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROC_PATH, "proc_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXY_AUTHNAME, "proxy_authname", 0, OPT_STRING,
    {(void *)("proxy")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXY_COMPRESS, "proxy_compress", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXY_PASSWORD, "proxy_password", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXY_REALM, "proxy_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXYD_ALLOW_STATUS_REFERRAL, "proxyd_allow_status_referral", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXYD_DISABLE_MAILBOX_REFERRALS, "proxyd_disable_mailbox_referrals", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PROXYSERVERS, "proxyservers", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTS_MODULE, "pts_module", 0, OPT_STRINGLIST,
    {(void*)("afskrb")},
    { { "afskrb" , IMAP_ENUM_ZERO },
      { "ldap" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTLOADER_SOCK, "ptloader_sock", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTSCACHE_DB, "ptscache_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTSCACHE_DB_PATH, "ptscache_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTSCACHE_TIMEOUT, "ptscache_timeout", 0, OPT_INT,
    {(void*)10800},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTSKRB5_CONVERT524, "ptskrb5_convert524", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_PTSKRB5_STRIP_DEFAULT_REALM, "ptskrb5_strip_default_realm", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_QOSMARKING, "qosmarking", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_QOSMARKING_CS0)},
    { { "cs0" , IMAP_ENUM_QOSMARKING_CS0 },
      { "cs1" , IMAP_ENUM_QOSMARKING_CS1 },
      { "cs2" , IMAP_ENUM_QOSMARKING_CS2 },
      { "cs3" , IMAP_ENUM_QOSMARKING_CS3 },
      { "cs4" , IMAP_ENUM_QOSMARKING_CS4 },
      { "cs5" , IMAP_ENUM_QOSMARKING_CS5 },
      { "cs6" , IMAP_ENUM_QOSMARKING_CS6 },
      { "cs7" , IMAP_ENUM_QOSMARKING_CS7 },
      { "af11" , IMAP_ENUM_QOSMARKING_AF11 },
      { "af12" , IMAP_ENUM_QOSMARKING_AF12 },
      { "af13" , IMAP_ENUM_QOSMARKING_AF13 },
      { "af21" , IMAP_ENUM_QOSMARKING_AF21 },
      { "af22" , IMAP_ENUM_QOSMARKING_AF22 },
      { "af23" , IMAP_ENUM_QOSMARKING_AF23 },
      { "af31" , IMAP_ENUM_QOSMARKING_AF31 },
      { "af32" , IMAP_ENUM_QOSMARKING_AF32 },
      { "af33" , IMAP_ENUM_QOSMARKING_AF33 },
      { "af41" , IMAP_ENUM_QOSMARKING_AF41 },
      { "af42" , IMAP_ENUM_QOSMARKING_AF42 },
      { "af43" , IMAP_ENUM_QOSMARKING_AF43 },
      { "ef" , IMAP_ENUM_QOSMARKING_EF },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_QUOTA_DB, "quota_db", 0, OPT_STRINGLIST,
    {(void*)("quotalegacy")},
    { { "flat" , IMAP_ENUM_ZERO },
      { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { "sql" , IMAP_ENUM_ZERO },
      { "quotalegacy" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_QUOTA_DB_PATH, "quota_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_QUOTAWARN, "quotawarn", 0, OPT_INT,
    {(void*)90},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_QUOTAWARNKB, "quotawarnkb", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_REJECT8BIT, "reject8bit", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RFC2046_STRICT, "rfc2046_strict", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RFC3028_STRICT, "rfc3028_strict", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RSS_FEEDLIST_TEMPLATE, "rss_feedlist_template", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RSS_FEEDS, "rss_feeds", 0, OPT_STRING,
    {(void *)("*")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RSS_MAXAGE, "rss_maxage", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RSS_MAXITEMS, "rss_maxitems", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RSS_MAXSYNOPSIS, "rss_maxsynopsis", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_RSS_REALM, "rss_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SASL_MAXIMUM_LAYER, "sasl_maximum_layer", 0, OPT_INT,
    {(void*)256},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SASL_MINIMUM_LAYER, "sasl_minimum_layer", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SEENSTATE_DB, "seenstate_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "flat" , IMAP_ENUM_ZERO },
      { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SENDMAIL, "sendmail", 0, OPT_STRING,
    {(void *)("/usr/lib/sendmail")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SERVERLIST, "serverlist", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SERVERNAME, "servername", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SERVERINFO, "serverinfo", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_SERVERINFO_ON)},
    { { "off" , IMAP_ENUM_SERVERINFO_OFF },
      { "min" , IMAP_ENUM_SERVERINFO_MIN },
      { "on" , IMAP_ENUM_SERVERINFO_ON },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SHAREDPREFIX, "sharedprefix", 0, OPT_STRING,
    {(void *)("Shared Folders")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVE_ALLOWREFERRALS, "sieve_allowreferrals", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVE_EXTENSIONS, "sieve_extensions", 0, OPT_BITFIELD,
    {(void *)(IMAP_ENUM_SIEVE_EXTENSIONS_FILEINTO
			 | IMAP_ENUM_SIEVE_EXTENSIONS_REJECT
			 | IMAP_ENUM_SIEVE_EXTENSIONS_VACATION
			 | IMAP_ENUM_SIEVE_EXTENSIONS_IMAPFLAGS
			 | IMAP_ENUM_SIEVE_EXTENSIONS_NOTIFY
			 | IMAP_ENUM_SIEVE_EXTENSIONS_ENVELOPE
			 | IMAP_ENUM_SIEVE_EXTENSIONS_RELATIONAL
			 | IMAP_ENUM_SIEVE_EXTENSIONS_REGEX
			 | IMAP_ENUM_SIEVE_EXTENSIONS_SUBADDRESS
			 | IMAP_ENUM_SIEVE_EXTENSIONS_COPY
			 | 0)},
    { { "fileinto" , IMAP_ENUM_SIEVE_EXTENSIONS_FILEINTO },
      { "reject" , IMAP_ENUM_SIEVE_EXTENSIONS_REJECT },
      { "vacation" , IMAP_ENUM_SIEVE_EXTENSIONS_VACATION },
      { "imapflags" , IMAP_ENUM_SIEVE_EXTENSIONS_IMAPFLAGS },
      { "notify" , IMAP_ENUM_SIEVE_EXTENSIONS_NOTIFY },
      { "include" , IMAP_ENUM_SIEVE_EXTENSIONS_INCLUDE },
      { "envelope" , IMAP_ENUM_SIEVE_EXTENSIONS_ENVELOPE },
      { "body" , IMAP_ENUM_SIEVE_EXTENSIONS_BODY },
      { "relational" , IMAP_ENUM_SIEVE_EXTENSIONS_RELATIONAL },
      { "regex" , IMAP_ENUM_SIEVE_EXTENSIONS_REGEX },
      { "subaddress" , IMAP_ENUM_SIEVE_EXTENSIONS_SUBADDRESS },
      { "copy" , IMAP_ENUM_SIEVE_EXTENSIONS_COPY },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVE_MAXSCRIPTSIZE, "sieve_maxscriptsize", 0, OPT_INT,
    {(void*)32},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVE_MAXSCRIPTS, "sieve_maxscripts", 0, OPT_INT,
    {(void*)5},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVE_UTF8FILEINTO, "sieve_utf8fileinto", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVE_SASL_SEND_UNSOLICITED_CAPABILITY, "sieve_sasl_send_unsolicited_capability", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVEDIR, "sievedir", 0, OPT_STRING,
    {(void *)("/usr/sieve")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVENOTIFIER, "sievenotifier", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SIEVEUSEHOMEDIR, "sieveusehomedir", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SINGLEINSTANCESTORE, "singleinstancestore", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SKIPLIST_ALWAYS_CHECKPOINT, "skiplist_always_checkpoint", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SKIPLIST_UNSAFE, "skiplist_unsafe", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SOFT_NOAUTH, "soft_noauth", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SPECIALUSEALWAYS, "specialusealways", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SQL_DATABASE, "sql_database", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SQL_ENGINE, "sql_engine", 0, OPT_STRINGLIST,
    {(void*)(NULL)},
    { { "mysql" , IMAP_ENUM_ZERO },
      { "pgsql" , IMAP_ENUM_ZERO },
      { "sqlite" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SQL_HOSTNAMES, "sql_hostnames", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SQL_PASSWD, "sql_passwd", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SQL_USER, "sql_user", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SQL_USESSL, "sql_usessl", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SRVTAB, "srvtab", 0, OPT_STRING,
    {(void *)("")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SUBMITSERVERS, "submitservers", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SUBSCRIPTION_DB, "subscription_db", 0, OPT_STRINGLIST,
    {(void*)("flat")},
    { { "flat" , IMAP_ENUM_ZERO },
      { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SUPPRESS_CAPABILITIES, "suppress_capabilities", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_STATUSCACHE, "statuscache", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_STATUSCACHE_DB, "statuscache_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-nosync" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "berkeley-hash-nosync" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { "sql" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_STATUSCACHE_DB_PATH, "statuscache_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_AUTHNAME, "sync_authname", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_COMPRESS, "sync_compress", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_HOST, "sync_host", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_LOG, "sync_log", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_LOG_CHAIN, "sync_log_chain", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_LOG_CHANNELS, "sync_log_channels", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_PASSWORD, "sync_password", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_PORT, "sync_port", 0, OPT_STRING,
    {(void *)("csync")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_REALM, "sync_realm", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_REPEAT_INTERVAL, "sync_repeat_interval", 0, OPT_INT,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYNC_SHUTDOWN_FILE, "sync_shutdown_file", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_SYSLOG_PREFIX, "syslog_prefix", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TCP_KEEPALIVE, "tcp_keepalive", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TCP_KEEPALIVE_CNT, "tcp_keepalive_cnt", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TCP_KEEPALIVE_IDLE, "tcp_keepalive_idle", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TCP_KEEPALIVE_INTVL, "tcp_keepalive_intvl", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TEMP_PATH, "temp_path", 0, OPT_STRING,
    {(void *)("/tmp")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TIMEOUT, "timeout", 0, OPT_INT,
    {(void*)30},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_CA_FILE, "tls_ca_file", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_CA_PATH, "tls_ca_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLSCACHE_DB, "tlscache_db", 0, OPT_STRINGLIST,
    {(void*)("skiplist")},
    { { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-nosync" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "berkeley-hash-nosync" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { "sql" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLSCACHE_DB_PATH, "tlscache_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_CERT_FILE, "tls_cert_file", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_CIPHER_LIST, "tls_cipher_list", 0, OPT_STRING,
    {(void *)("DEFAULT")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_KEY_FILE, "tls_key_file", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_REQUIRE_CERT, "tls_require_cert", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_TLS_SESSION_TIMEOUT, "tls_session_timeout", 0, OPT_INT,
    {(void*)1440},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_UMASK, "umask", 0, OPT_STRING,
    {(void *)("077")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_USERDENY_DB, "userdeny_db", 0, OPT_STRINGLIST,
    {(void*)("flat")},
    { { "flat" , IMAP_ENUM_ZERO },
      { "berkeley" , IMAP_ENUM_ZERO },
      { "berkeley-hash" , IMAP_ENUM_ZERO },
      { "skiplist" , IMAP_ENUM_ZERO },
      { "sql" , IMAP_ENUM_ZERO },
      { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_USERDENY_DB_PATH, "userdeny_db_path", 0, OPT_STRING,
    {(void *)(NULL)},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_USER_FOLDER_LIMIT, "user_folder_limit", 0, OPT_INT,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_USERNAME_TOLOWER, "username_tolower", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_USERPREFIX, "userprefix", 0, OPT_STRING,
    {(void *)("Other Users")},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_UNIX_GROUP_ENABLE, "unix_group_enable", 0, OPT_SWITCH,
    {(void*)1},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_UNIXHIERARCHYSEP, "unixhierarchysep", 0, OPT_SWITCH,
    {(void*)0},
    { { NULL, IMAP_ENUM_ZERO } } },
  { IMAPOPT_VIRTDOMAINS, "virtdomains", 0, OPT_ENUM,
    {(void *)(IMAP_ENUM_VIRTDOMAINS_OFF)},
    { { "off" , IMAP_ENUM_VIRTDOMAINS_OFF },
      { "userid" , IMAP_ENUM_VIRTDOMAINS_USERID },
      { "on" , IMAP_ENUM_VIRTDOMAINS_ON },
      { NULL, IMAP_ENUM_ZERO } } },

  { IMAPOPT_LAST, NULL, 0, OPT_NOTOPT, { NULL }, { { NULL, IMAP_ENUM_ZERO } } }

};



/* c code goes here */



