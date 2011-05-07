/* auto-generated by config2header 1.17 */
#ifndef INCLUDED_IMAPOPTS_H
#define INCLUDED_IMAPOPTS_H

enum opttype {
  OPT_BITFIELD,
  OPT_STRINGLIST,
  OPT_ENUM,
  OPT_SWITCH,
  OPT_INT,
  OPT_STRING,
  OPT_NOTOPT
};


enum imapopt {

  IMAPOPT_ZERO = 0,

  IMAPOPT_ADMINS,
  IMAPOPT_AFSPTS_LOCALREALMS,
  IMAPOPT_AFSPTS_MYCELL,
  IMAPOPT_ALLOWALLSUBSCRIBE,
  IMAPOPT_ALLOWANONYMOUSLOGIN,
  IMAPOPT_ALLOWAPOP,
  IMAPOPT_ALLOWNEWNEWS,
  IMAPOPT_ALLOWPLAINTEXT,
  IMAPOPT_ALLOWUSERMOVES,
  IMAPOPT_ALTNAMESPACE,
  IMAPOPT_ANNOTATION_DB,
  IMAPOPT_ANYONEUSERACL,
  IMAPOPT_ANNOTATION_DEFINITIONS,
  IMAPOPT_AUDITLOG,
  IMAPOPT_AUTH_MECH,
  IMAPOPT_AUTOCREATEQUOTA,
  IMAPOPT_BERKELEY_CACHESIZE,
  IMAPOPT_BERKELEY_LOCKS_MAX,
  IMAPOPT_BERKELEY_TXNS_MAX,
  IMAPOPT_CLIENT_TIMEOUT,
  IMAPOPT_COMMANDMINTIMER,
  IMAPOPT_CONFIGDIRECTORY,
  IMAPOPT_DEBUG_COMMAND,
  IMAPOPT_DEFAULTACL,
  IMAPOPT_DEFAULTDOMAIN,
  IMAPOPT_DEFAULTPARTITION,
  IMAPOPT_DEFAULTSERVER,
  IMAPOPT_DELETEDPREFIX,
  IMAPOPT_DELETE_MODE,
  IMAPOPT_DELETERIGHT,
  IMAPOPT_DISCONNECT_ON_VANISHED_MAILBOX,
  IMAPOPT_DUPLICATE_DB,
  IMAPOPT_DUPLICATESUPPRESSION,
  IMAPOPT_EXPUNGE_MODE,
  IMAPOPT_EXPUNGE_DAYS,
  IMAPOPT_FAILEDLOGINPAUSE,
  IMAPOPT_FLUSHSEENSTATE,
  IMAPOPT_FOOLSTUPIDCLIENTS,
  IMAPOPT_FORCE_SASL_CLIENT_MECH,
  IMAPOPT_FULLDIRHASH,
  IMAPOPT_HASHIMAPSPOOL,
  IMAPOPT_IDLESOCKET,
  IMAPOPT_IGNOREREFERENCE,
  IMAPOPT_IMAPIDLEPOLL,
  IMAPOPT_IMAPIDRESPONSE,
  IMAPOPT_IMAPMAGICPLUS,
  IMAPOPT_IMPLICIT_OWNER_RIGHTS,
  IMAPOPT_IMPROVED_MBOXLIST_SORT,
  IMAPOPT_INTERNALDATE_HEURISTIC,
  IMAPOPT_LDAP_AUTHZ,
  IMAPOPT_LDAP_BASE,
  IMAPOPT_LDAP_BIND_DN,
  IMAPOPT_LDAP_DEREF,
  IMAPOPT_LDAP_FILTER,
  IMAPOPT_LDAP_GROUP_BASE,
  IMAPOPT_LDAP_GROUP_FILTER,
  IMAPOPT_LDAP_GROUP_SCOPE,
  IMAPOPT_LDAP_ID,
  IMAPOPT_LDAP_MECH,
  IMAPOPT_LDAP_MEMBER_ATTRIBUTE,
  IMAPOPT_LDAP_MEMBER_BASE,
  IMAPOPT_LDAP_MEMBER_FILTER,
  IMAPOPT_LDAP_MEMBER_METHOD,
  IMAPOPT_LDAP_MEMBER_SCOPE,
  IMAPOPT_LDAP_PASSWORD,
  IMAPOPT_LDAP_REALM,
  IMAPOPT_LDAP_REFERRALS,
  IMAPOPT_LDAP_RESTART,
  IMAPOPT_LDAP_SASL,
  IMAPOPT_LDAP_SASL_AUTHC,
  IMAPOPT_LDAP_SASL_AUTHZ,
  IMAPOPT_LDAP_SASL_MECH,
  IMAPOPT_LDAP_SASL_PASSWORD,
  IMAPOPT_LDAP_SASL_REALM,
  IMAPOPT_LDAP_SCOPE,
  IMAPOPT_LDAP_SERVERS,
  IMAPOPT_LDAP_SIZE_LIMIT,
  IMAPOPT_LDAP_START_TLS,
  IMAPOPT_LDAP_TIME_LIMIT,
  IMAPOPT_LDAP_TIMEOUT,
  IMAPOPT_LDAP_TLS_CACERT_DIR,
  IMAPOPT_LDAP_TLS_CACERT_FILE,
  IMAPOPT_LDAP_TLS_CERT,
  IMAPOPT_LDAP_TLS_CHECK_PEER,
  IMAPOPT_LDAP_TLS_CIPHERS,
  IMAPOPT_LDAP_TLS_KEY,
  IMAPOPT_LDAP_URI,
  IMAPOPT_LDAP_VERSION,
  IMAPOPT_LMTP_DOWNCASE_RCPT,
  IMAPOPT_LMTP_FUZZY_MAILBOX_MATCH,
  IMAPOPT_LMTP_OVER_QUOTA_PERM_FAILURE,
  IMAPOPT_LMTP_STRICT_QUOTA,
  IMAPOPT_LMTPSOCKET,
  IMAPOPT_LOGINREALMS,
  IMAPOPT_LOGINUSEACL,
  IMAPOPT_LOGTIMESTAMPS,
  IMAPOPT_MAILBOX_DEFAULT_OPTIONS,
  IMAPOPT_MAILNOTIFIER,
  IMAPOPT_MAXHEADERLINES,
  IMAPOPT_MAXMESSAGESIZE,
  IMAPOPT_MAXQUOTED,
  IMAPOPT_MAXWORD,
  IMAPOPT_MBOXKEY_DB,
  IMAPOPT_MBOXLIST_DB,
  IMAPOPT_MBOXNAME_LOCKPATH,
  IMAPOPT_METAPARTITION_FILES,
  IMAPOPT_MUPDATE_AUTHNAME,
  IMAPOPT_MUPDATE_CONFIG,
  IMAPOPT_MUNGE8BIT,
  IMAPOPT_MUPDATE_CONNECTIONS_MAX,
  IMAPOPT_MUPDATE_PASSWORD,
  IMAPOPT_MUPDATE_PORT,
  IMAPOPT_MUPDATE_REALM,
  IMAPOPT_MUPDATE_RETRY_DELAY,
  IMAPOPT_MUPDATE_SERVER,
  IMAPOPT_MUPDATE_USERNAME,
  IMAPOPT_MUPDATE_WORKERS_MAX,
  IMAPOPT_MUPDATE_WORKERS_MAXSPARE,
  IMAPOPT_MUPDATE_WORKERS_MINSPARE,
  IMAPOPT_MUPDATE_WORKERS_START,
  IMAPOPT_NETSCAPEURL,
  IMAPOPT_NEWSGROUPS,
  IMAPOPT_NEWSMASTER,
  IMAPOPT_NEWSPEER,
  IMAPOPT_NEWSPOSTUSER,
  IMAPOPT_NEWSPREFIX,
  IMAPOPT_NNTPTIMEOUT,
  IMAPOPT_NOTIFYSOCKET,
  IMAPOPT_NOTIFY_EXTERNAL,
  IMAPOPT_PLAINTEXTLOGINPAUSE,
  IMAPOPT_PLAINTEXTLOGINALERT,
  IMAPOPT_POPEXPIRETIME,
  IMAPOPT_POPMINPOLL,
  IMAPOPT_POPSUBFOLDERS,
  IMAPOPT_POPPOLLPADDING,
  IMAPOPT_POPTIMEOUT,
  IMAPOPT_POPUSEACL,
  IMAPOPT_POPUSEIMAPFLAGS,
  IMAPOPT_POSTMASTER,
  IMAPOPT_POSTSPEC,
  IMAPOPT_POSTUSER,
  IMAPOPT_PROC_PATH,
  IMAPOPT_PROXY_AUTHNAME,
  IMAPOPT_PROXY_COMPRESS,
  IMAPOPT_PROXY_PASSWORD,
  IMAPOPT_PROXY_REALM,
  IMAPOPT_PROXYD_ALLOW_STATUS_REFERRAL,
  IMAPOPT_PROXYD_DISABLE_MAILBOX_REFERRALS,
  IMAPOPT_PROXYSERVERS,
  IMAPOPT_PTS_MODULE,
  IMAPOPT_PTLOADER_SOCK,
  IMAPOPT_PTSCACHE_DB,
  IMAPOPT_PTSCACHE_TIMEOUT,
  IMAPOPT_PTSKRB5_CONVERT524,
  IMAPOPT_PTSKRB5_STRIP_DEFAULT_REALM,
  IMAPOPT_QOSMARKING,
  IMAPOPT_QUOTA_DB,
  IMAPOPT_QUOTAWARN,
  IMAPOPT_QUOTAWARNKB,
  IMAPOPT_REJECT8BIT,
  IMAPOPT_RFC2046_STRICT,
  IMAPOPT_RFC3028_STRICT,
  IMAPOPT_SASL_MAXIMUM_LAYER,
  IMAPOPT_SASL_MINIMUM_LAYER,
  IMAPOPT_SEENSTATE_DB,
  IMAPOPT_SENDMAIL,
  IMAPOPT_SERVERLIST,
  IMAPOPT_SERVERNAME,
  IMAPOPT_SERVERINFO,
  IMAPOPT_SHAREDPREFIX,
  IMAPOPT_SIEVE_ALLOWREFERRALS,
  IMAPOPT_SIEVE_EXTENSIONS,
  IMAPOPT_SIEVE_MAXSCRIPTSIZE,
  IMAPOPT_SIEVE_MAXSCRIPTS,
  IMAPOPT_SIEVE_UTF8FILEINTO,
  IMAPOPT_SIEVE_SASL_SEND_UNSOLICITED_CAPABILITY,
  IMAPOPT_SIEVEDIR,
  IMAPOPT_SIEVENOTIFIER,
  IMAPOPT_SIEVEUSEHOMEDIR,
  IMAPOPT_SINGLEINSTANCESTORE,
  IMAPOPT_SKIPLIST_ALWAYS_CHECKPOINT,
  IMAPOPT_SKIPLIST_UNSAFE,
  IMAPOPT_SOFT_NOAUTH,
  IMAPOPT_SQL_DATABASE,
  IMAPOPT_SQL_ENGINE,
  IMAPOPT_SQL_HOSTNAMES,
  IMAPOPT_SQL_PASSWD,
  IMAPOPT_SQL_USER,
  IMAPOPT_SQL_USESSL,
  IMAPOPT_SRVTAB,
  IMAPOPT_SUBMITSERVERS,
  IMAPOPT_SUBSCRIPTION_DB,
  IMAPOPT_SUPPRESS_CAPABILITIES,
  IMAPOPT_STATUSCACHE,
  IMAPOPT_STATUSCACHE_DB,
  IMAPOPT_SYNC_AUTHNAME,
  IMAPOPT_SYNC_COMPRESS,
  IMAPOPT_SYNC_HOST,
  IMAPOPT_SYNC_LOG,
  IMAPOPT_SYNC_LOG_CHAIN,
  IMAPOPT_SYNC_LOG_CHANNELS,
  IMAPOPT_SYNC_PASSWORD,
  IMAPOPT_SYNC_PORT,
  IMAPOPT_SYNC_REALM,
  IMAPOPT_SYNC_REPEAT_INTERVAL,
  IMAPOPT_SYNC_SHUTDOWN_FILE,
  IMAPOPT_SYSLOG_PREFIX,
  IMAPOPT_TCP_KEEPALIVE,
  IMAPOPT_TCP_KEEPALIVE_CNT,
  IMAPOPT_TCP_KEEPALIVE_IDLE,
  IMAPOPT_TCP_KEEPALIVE_INTVL,
  IMAPOPT_TEMP_PATH,
  IMAPOPT_TIMEOUT,
  IMAPOPT_TLS_CA_FILE,
  IMAPOPT_TLS_CA_PATH,
  IMAPOPT_TLSCACHE_DB,
  IMAPOPT_TLS_CERT_FILE,
  IMAPOPT_TLS_CIPHER_LIST,
  IMAPOPT_TLS_KEY_FILE,
  IMAPOPT_TLS_REQUIRE_CERT,
  IMAPOPT_TLS_SESSION_TIMEOUT,
  IMAPOPT_UMASK,
  IMAPOPT_USERDENY_DB,
  IMAPOPT_USER_FOLDER_LIMIT,
  IMAPOPT_USERNAME_TOLOWER,
  IMAPOPT_USERPREFIX,
  IMAPOPT_UNIX_GROUP_ENABLE,
  IMAPOPT_UNIXHIERARCHYSEP,
  IMAPOPT_VIRTDOMAINS,

  IMAPOPT_LAST

};



enum enum_value {

  IMAP_ENUM_ZERO = 0,

  IMAP_ENUM_DELETE_MODE_IMMEDIATE = 0,
  IMAP_ENUM_DELETE_MODE_DELAYED,
  IMAP_ENUM_EXPUNGE_MODE_DEFAULT = 0,
  IMAP_ENUM_EXPUNGE_MODE_IMMEDIATE,
  IMAP_ENUM_EXPUNGE_MODE_DELAYED,
  IMAP_ENUM_INTERNALDATE_HEURISTIC_STANDARD = 0,
  IMAP_ENUM_INTERNALDATE_HEURISTIC_RECEIVEDHEADER,
  IMAP_ENUM_METAPARTITION_FILES_HEADER = (1<<0),
  IMAP_ENUM_METAPARTITION_FILES_INDEX = (1<<1),
  IMAP_ENUM_METAPARTITION_FILES_CACHE = (1<<2),
  IMAP_ENUM_METAPARTITION_FILES_EXPUNGE = (1<<3),
  IMAP_ENUM_METAPARTITION_FILES_SQUAT = (1<<4),
  IMAP_ENUM_METAPARTITION_FILES_LOCK = (1<<5),
  IMAP_ENUM_MUPDATE_CONFIG_STANDARD = 0,
  IMAP_ENUM_MUPDATE_CONFIG_UNIFIED,
  IMAP_ENUM_MUPDATE_CONFIG_REPLICATED,
  IMAP_ENUM_QOSMARKING_CS0 = 0,
  IMAP_ENUM_QOSMARKING_CS1,
  IMAP_ENUM_QOSMARKING_CS2,
  IMAP_ENUM_QOSMARKING_CS3,
  IMAP_ENUM_QOSMARKING_CS4,
  IMAP_ENUM_QOSMARKING_CS5,
  IMAP_ENUM_QOSMARKING_CS6,
  IMAP_ENUM_QOSMARKING_CS7,
  IMAP_ENUM_QOSMARKING_AF11,
  IMAP_ENUM_QOSMARKING_AF12,
  IMAP_ENUM_QOSMARKING_AF13,
  IMAP_ENUM_QOSMARKING_AF21,
  IMAP_ENUM_QOSMARKING_AF22,
  IMAP_ENUM_QOSMARKING_AF23,
  IMAP_ENUM_QOSMARKING_AF31,
  IMAP_ENUM_QOSMARKING_AF32,
  IMAP_ENUM_QOSMARKING_AF33,
  IMAP_ENUM_QOSMARKING_AF41,
  IMAP_ENUM_QOSMARKING_AF42,
  IMAP_ENUM_QOSMARKING_AF43,
  IMAP_ENUM_QOSMARKING_EF,
  IMAP_ENUM_SERVERINFO_OFF = 0,
  IMAP_ENUM_SERVERINFO_MIN,
  IMAP_ENUM_SERVERINFO_ON,
  IMAP_ENUM_SIEVE_EXTENSIONS_FILEINTO = (1<<0),
  IMAP_ENUM_SIEVE_EXTENSIONS_REJECT = (1<<1),
  IMAP_ENUM_SIEVE_EXTENSIONS_VACATION = (1<<2),
  IMAP_ENUM_SIEVE_EXTENSIONS_IMAPFLAGS = (1<<3),
  IMAP_ENUM_SIEVE_EXTENSIONS_NOTIFY = (1<<4),
  IMAP_ENUM_SIEVE_EXTENSIONS_INCLUDE = (1<<5),
  IMAP_ENUM_SIEVE_EXTENSIONS_ENVELOPE = (1<<6),
  IMAP_ENUM_SIEVE_EXTENSIONS_BODY = (1<<7),
  IMAP_ENUM_SIEVE_EXTENSIONS_RELATIONAL = (1<<8),
  IMAP_ENUM_SIEVE_EXTENSIONS_REGEX = (1<<9),
  IMAP_ENUM_SIEVE_EXTENSIONS_SUBADDRESS = (1<<10),
  IMAP_ENUM_SIEVE_EXTENSIONS_COPY = (1<<11),
  IMAP_ENUM_VIRTDOMAINS_OFF = 0,
  IMAP_ENUM_VIRTDOMAINS_USERID,
  IMAP_ENUM_VIRTDOMAINS_ON

};



union config_value {

    void *dummy;

    const char *s;	/* OPT_STRING, OPT_STRINGLIST */

    long i;		/* OPT_INT */

    long b;		/* OPT_SWITCH */

    enum enum_value e;	/* OPT_ENUM */

    unsigned long x;	/* OPT_BITFIELD */

};



struct enum_option_s {

    const char *name;

    const enum enum_value val;

};



#define MAX_ENUM_OPTS 21

struct imapopt_s {

    const enum imapopt opt;

    const char *optname;

    int seen;

    const enum opttype t;

    union config_value val;

    const struct enum_option_s enum_options[MAX_ENUM_OPTS+1];

};



extern struct imapopt_s imapopts[];



#endif /* INCLUDED_IMAPOPTIONS_H */
