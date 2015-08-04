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

  IMAPOPT_ADDRESSBOOKPREFIX,
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
  IMAPOPT_ANNOTATION_DB_PATH,
  IMAPOPT_ANYONEUSERACL,
  IMAPOPT_ANNOTATION_ALLOW_UNDEFINED,
  IMAPOPT_ANNOTATION_DEFINITIONS,
  IMAPOPT_ANNOTATION_CALLOUT,
  IMAPOPT_APS_TOPIC,
  IMAPOPT_ARCHIVE_ENABLED,
  IMAPOPT_ARCHIVE_DAYS,
  IMAPOPT_ARCHIVE_MAXSIZE,
  IMAPOPT_ARCHIVE_KEEPFLAGGED,
  IMAPOPT_AUDITLOG,
  IMAPOPT_AUTH_MECH,
  IMAPOPT_AUTOCREATEINBOXFOLDERS,
  IMAPOPT_AUTOCREATEQUOTA,
  IMAPOPT_AUTOCREATEQUOTAMSG,
  IMAPOPT_AUTOSIEVEFOLDERS,
  IMAPOPT_GENERATE_COMPILED_SIEVE_SCRIPT,
  IMAPOPT_AUTOCREATE_SIEVE_COMPILED_SCRIPT,
  IMAPOPT_AUTOSUBSCRIBEINBOXFOLDERS,
  IMAPOPT_AUTOSUBSCRIBESHAREDFOLDERS,
  IMAPOPT_AUTOSUBSCRIBE_ALL_SHAREDFOLDERS,
  IMAPOPT_AUTOCREATE_INBOX_FOLDERS,
  IMAPOPT_AUTOCREATE_POST,
  IMAPOPT_AUTOCREATE_QUOTA,
  IMAPOPT_AUTOCREATE_QUOTA_MESSAGES,
  IMAPOPT_AUTOCREATE_SIEVE_FOLDERS,
  IMAPOPT_AUTOCREATE_SIEVE_SCRIPT,
  IMAPOPT_AUTOCREATE_SIEVE_SCRIPT_COMPILE,
  IMAPOPT_AUTOCREATE_SIEVE_SCRIPT_COMPILED,
  IMAPOPT_AUTOCREATE_SUBSCRIBE_FOLDERS,
  IMAPOPT_AUTOCREATE_SUBSCRIBE_SHAREDFOLDERS,
  IMAPOPT_AUTOCREATE_SUBSCRIBE_SHAREDFOLDERS_ALL,
  IMAPOPT_AUTOCREATE_USERS,
  IMAPOPT_BOUNDARY_LIMIT,
  IMAPOPT_CALDAV_ALLOWATTACH,
  IMAPOPT_CALDAV_ALLOWSCHEDULING,
  IMAPOPT_CALDAV_CREATE_ATTACH,
  IMAPOPT_CALDAV_CREATE_DEFAULT,
  IMAPOPT_CALDAV_CREATE_SCHED,
  IMAPOPT_CALDAV_MAXDATETIME,
  IMAPOPT_CALDAV_MINDATETIME,
  IMAPOPT_CALDAV_REALM,
  IMAPOPT_CALENDARPREFIX,
  IMAPOPT_CALENDAR_USER_ADDRESS_SET,
  IMAPOPT_CARDDAV_REALM,
  IMAPOPT_CHATTY,
  IMAPOPT_CLIENT_TIMEOUT,
  IMAPOPT_COMMANDMINTIMER,
  IMAPOPT_CONFIGDIRECTORY,
  IMAPOPT_CREATEONPOST,
  IMAPOPT_CONVERSATIONS,
  IMAPOPT_CONVERSATIONS_COUNTED_FLAGS,
  IMAPOPT_CONVERSATIONS_DB,
  IMAPOPT_CONVERSATIONS_EXPIRE_DAYS,
  IMAPOPT_DAV_REALM,
  IMAPOPT_DEBUG_COMMAND,
  IMAPOPT_DEFAULTACL,
  IMAPOPT_DEFAULTDOMAIN,
  IMAPOPT_DEFAULTPARTITION,
  IMAPOPT_DEFAULTSERVER,
  IMAPOPT_DELETEDPREFIX,
  IMAPOPT_DELETE_MODE,
  IMAPOPT_DELETERIGHT,
  IMAPOPT_DISABLE_USER_NAMESPACE,
  IMAPOPT_DISABLE_SHARED_NAMESPACE,
  IMAPOPT_DISCONNECT_ON_VANISHED_MAILBOX,
  IMAPOPT_ISCHEDULE_DKIM_DOMAIN,
  IMAPOPT_ISCHEDULE_DKIM_KEY_FILE,
  IMAPOPT_ISCHEDULE_DKIM_SELECTOR,
  IMAPOPT_DUPLICATE_DB,
  IMAPOPT_DUPLICATE_DB_PATH,
  IMAPOPT_DUPLICATESUPPRESSION,
  IMAPOPT_EVENT_CONTENT_INCLUSION_MODE,
  IMAPOPT_EVENT_CONTENT_SIZE,
  IMAPOPT_EVENT_EXCLUDE_FLAGS,
  IMAPOPT_EVENT_EXCLUDE_SPECIALUSE,
  IMAPOPT_EVENT_EXTRA_PARAMS,
  IMAPOPT_EVENT_GROUPS,
  IMAPOPT_EVENT_NOTIFIER,
  IMAPOPT_EXPUNGE_MODE,
  IMAPOPT_FAILEDLOGINPAUSE,
  IMAPOPT_FLUSHSEENSTATE,
  IMAPOPT_FOOLSTUPIDCLIENTS,
  IMAPOPT_FORCE_SASL_CLIENT_MECH,
  IMAPOPT_FULLDIRHASH,
  IMAPOPT_HASHIMAPSPOOL,
  IMAPOPT_DEBUG,
  IMAPOPT_HTTPALLOWCOMPRESS,
  IMAPOPT_HTTPALLOWCORS,
  IMAPOPT_HTTPALLOWTRACE,
  IMAPOPT_HTTPALLOWEDURLS,
  IMAPOPT_HTTPCONTENTMD5,
  IMAPOPT_HTTPDOCROOT,
  IMAPOPT_HTTPKEEPALIVE,
  IMAPOPT_HTTPMODULES,
  IMAPOPT_HTTPPRETTYTELEMETRY,
  IMAPOPT_HTTPTIMEOUT,
  IMAPOPT_IDLESOCKET,
  IMAPOPT_IGNOREREFERENCE,
  IMAPOPT_IMAPIDLEPOLL,
  IMAPOPT_IMAPIDRESPONSE,
  IMAPOPT_IMAPMAGICPLUS,
  IMAPOPT_IMPLICIT_OWNER_RIGHTS,
  IMAPOPT_IMPROVED_MBOXLIST_SORT,
  IMAPOPT_INTERNALDATE_HEURISTIC,
  IMAPOPT_IOLOG,
  IMAPOPT_LDAP_AUTHZ,
  IMAPOPT_LDAP_BASE,
  IMAPOPT_LDAP_BIND_DN,
  IMAPOPT_LDAP_DEREF,
  IMAPOPT_LDAP_DOMAIN_BASE_DN,
  IMAPOPT_LDAP_DOMAIN_FILTER,
  IMAPOPT_LDAP_DOMAIN_NAME_ATTRIBUTE,
  IMAPOPT_LDAP_DOMAIN_SCOPE,
  IMAPOPT_LDAP_DOMAIN_RESULT_ATTRIBUTE,
  IMAPOPT_LDAP_FILTER,
  IMAPOPT_LDAP_GROUP_BASE,
  IMAPOPT_LDAP_GROUP_FILTER,
  IMAPOPT_LDAP_GROUP_SCOPE,
  IMAPOPT_LDAP_ID,
  IMAPOPT_LDAP_MECH,
  IMAPOPT_LDAP_USER_ATTRIBUTE,
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
  IMAPOPT_LDAP_CA_DIR,
  IMAPOPT_LDAP_CA_FILE,
  IMAPOPT_LDAP_CIPHERS,
  IMAPOPT_LDAP_CLIENT_CERT,
  IMAPOPT_LDAP_CLIENT_KEY,
  IMAPOPT_LDAP_VERIFY_PEER,
  IMAPOPT_LDAP_TLS_CACERT_DIR,
  IMAPOPT_LDAP_TLS_CACERT_FILE,
  IMAPOPT_LDAP_TLS_CERT,
  IMAPOPT_LDAP_TLS_KEY,
  IMAPOPT_LDAP_TLS_CHECK_PEER,
  IMAPOPT_LDAP_TLS_CIPHERS,
  IMAPOPT_LDAP_URI,
  IMAPOPT_LDAP_VERSION,
  IMAPOPT_LMTP_DOWNCASE_RCPT,
  IMAPOPT_LMTP_FUZZY_MAILBOX_MATCH,
  IMAPOPT_LMTP_OVER_QUOTA_PERM_FAILURE,
  IMAPOPT_LMTP_STRICT_QUOTA,
  IMAPOPT_LMTP_STRICT_RFC2821,
  IMAPOPT_LMTPSOCKET,
  IMAPOPT_LMTPTXN_TIMEOUT,
  IMAPOPT_LOGINREALMS,
  IMAPOPT_LOGINUSEACL,
  IMAPOPT_LOGTIMESTAMPS,
  IMAPOPT_MAILBOX_DEFAULT_OPTIONS,
  IMAPOPT_MAILBOX_INITIAL_FLAGS,
  IMAPOPT_MAILNOTIFIER,
  IMAPOPT_MAXHEADERLINES,
  IMAPOPT_MAXLOGINS_PER_HOST,
  IMAPOPT_MAXLOGINS_PER_USER,
  IMAPOPT_MAXMESSAGESIZE,
  IMAPOPT_MAXQUOTED,
  IMAPOPT_MAXWORD,
  IMAPOPT_MBOXKEY_DB,
  IMAPOPT_MBOXLIST_DB,
  IMAPOPT_MBOXLIST_DB_PATH,
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
  IMAPOPT_NEWSADDHEADERS,
  IMAPOPT_NEWSGROUPS,
  IMAPOPT_NEWSMASTER,
  IMAPOPT_NEWSPEER,
  IMAPOPT_NEWSPOSTUSER,
  IMAPOPT_NEWSPREFIX,
  IMAPOPT_NEWSRC_DB_PATH,
  IMAPOPT_NNTPTIMEOUT,
  IMAPOPT_NOTESMAILBOX,
  IMAPOPT_NOTIFYSOCKET,
  IMAPOPT_NOTIFY_EXTERNAL,
  IMAPOPT_PARTITION_SELECT_MODE,
  IMAPOPT_PARTITION_SELECT_EXCLUDE,
  IMAPOPT_PARTITION_SELECT_USAGE_REINIT,
  IMAPOPT_PARTITION_SELECT_SOFT_USAGE_LIMIT,
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
  IMAPOPT_PTSCACHE_DB_PATH,
  IMAPOPT_PTSCACHE_TIMEOUT,
  IMAPOPT_PTSKRB5_CONVERT524,
  IMAPOPT_PTSKRB5_STRIP_DEFAULT_REALM,
  IMAPOPT_QOSMARKING,
  IMAPOPT_QUOTA_DB,
  IMAPOPT_QUOTA_DB_PATH,
  IMAPOPT_QUOTAWARN,
  IMAPOPT_QUOTAWARNKB,
  IMAPOPT_QUOTAWARNMSG,
  IMAPOPT_REJECT8BIT,
  IMAPOPT_RFC2046_STRICT,
  IMAPOPT_RFC3028_STRICT,
  IMAPOPT_RSS_FEEDLIST_TEMPLATE,
  IMAPOPT_RSS_FEEDS,
  IMAPOPT_RSS_MAXAGE,
  IMAPOPT_RSS_MAXITEMS,
  IMAPOPT_RSS_MAXSYNOPSIS,
  IMAPOPT_RSS_REALM,
  IMAPOPT_SASL_MAXIMUM_LAYER,
  IMAPOPT_SASL_MINIMUM_LAYER,
  IMAPOPT_SEARCH_BATCHSIZE,
  IMAPOPT_SEARCH_NORMALISATION_MAX,
  IMAPOPT_SEARCH_ENGINE,
  IMAPOPT_SEARCH_INDEX_HEADERS,
  IMAPOPT_SEARCH_INDEXED_DB,
  IMAPOPT_SEARCH_SKIPDIACRIT,
  IMAPOPT_SEARCH_SKIPHTML,
  IMAPOPT_SEARCH_WHITESPACE,
  IMAPOPT_SEENSTATE_DB,
  IMAPOPT_SENDMAIL,
  IMAPOPT_SERVERLIST,
  IMAPOPT_SERVERLIST_SELECT_MODE,
  IMAPOPT_SERVERLIST_SELECT_USAGE_REINIT,
  IMAPOPT_SERVERLIST_SELECT_SOFT_USAGE_LIMIT,
  IMAPOPT_SERVERNAME,
  IMAPOPT_SERVERINFO,
  IMAPOPT_SHAREDPREFIX,
  IMAPOPT_SIEVE_ALLOWREFERRALS,
  IMAPOPT_SIEVE_EXTENSIONS,
  IMAPOPT_SIEVE_MAXSCRIPTSIZE,
  IMAPOPT_SIEVE_MAXSCRIPTS,
  IMAPOPT_SIEVE_UTF8FILEINTO,
  IMAPOPT_SIEVE_SASL_SEND_UNSOLICITED_CAPABILITY,
  IMAPOPT_SIEVE_VACATION_MIN_RESPONSE,
  IMAPOPT_SIEVE_VACATION_MAX_RESPONSE,
  IMAPOPT_SIEVEDIR,
  IMAPOPT_SIEVENOTIFIER,
  IMAPOPT_SIEVEUSEHOMEDIR,
  IMAPOPT_ANYSIEVEFOLDER,
  IMAPOPT_SINGLEINSTANCESTORE,
  IMAPOPT_SKIPLIST_ALWAYS_CHECKPOINT,
  IMAPOPT_SKIPLIST_UNSAFE,
  IMAPOPT_SOFT_NOAUTH,
  IMAPOPT_SORTCACHE_DB,
  IMAPOPT_SPECIALUSE_EXTRA,
  IMAPOPT_SPECIALUSEALWAYS,
  IMAPOPT_SPHINX_TEXT_EXCLUDES_ODD_HEADERS,
  IMAPOPT_SPHINX_SOCKET,
  IMAPOPT_SPHINX_PIDFILE,
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
  IMAPOPT_STATUSCACHE_DB_PATH,
  IMAPOPT_SYNC_AUTHNAME,
  IMAPOPT_SYNC_HOST,
  IMAPOPT_SYNC_LOG,
  IMAPOPT_SYNC_LOG_CHAIN,
  IMAPOPT_SYNC_LOG_CHANNELS,
  IMAPOPT_SYNC_LOG_UNSUPPRESSABLE_CHANNELS,
  IMAPOPT_SYNC_PASSWORD,
  IMAPOPT_SYNC_PORT,
  IMAPOPT_SYNC_REALM,
  IMAPOPT_SYNC_REPEAT_INTERVAL,
  IMAPOPT_SYNC_SHUTDOWN_FILE,
  IMAPOPT_SYNC_TIMEOUT,
  IMAPOPT_SYSLOG_PREFIX,
  IMAPOPT_SYSLOG_FACILITY,
  IMAPOPT_TCP_KEEPALIVE,
  IMAPOPT_TCP_KEEPALIVE_CNT,
  IMAPOPT_TCP_KEEPALIVE_IDLE,
  IMAPOPT_TCP_KEEPALIVE_INTVL,
  IMAPOPT_TEMP_PATH,
  IMAPOPT_TIMEOUT,
  IMAPOPT_TLS_CA_FILE,
  IMAPOPT_TLS_CA_PATH,
  IMAPOPT_TLSCACHE_DB,
  IMAPOPT_TLSCACHE_DB_PATH,
  IMAPOPT_TLS_CERT_FILE,
  IMAPOPT_TLS_CIPHER_LIST,
  IMAPOPT_TLS_CIPHERS,
  IMAPOPT_TLS_CLIENT_CA_DIR,
  IMAPOPT_TLS_CLIENT_CA_FILE,
  IMAPOPT_TLS_CLIENT_CERT,
  IMAPOPT_TLS_CLIENT_CERTS,
  IMAPOPT_TLS_CLIENT_KEY,
  IMAPOPT_TLS_COMPRESSION,
  IMAPOPT_TLS_ECCURVE,
  IMAPOPT_TLS_KEY_FILE,
  IMAPOPT_TLS_PREFER_SERVER_CIPHERS,
  IMAPOPT_TLS_SERVER_CA_DIR,
  IMAPOPT_TLS_SERVER_CA_FILE,
  IMAPOPT_TLS_SERVER_CERT,
  IMAPOPT_TLS_SERVER_KEY,
  IMAPOPT_TLS_SESSIONS_DB,
  IMAPOPT_TLS_SESSIONS_DB_PATH,
  IMAPOPT_TLS_SESSION_TIMEOUT,
  IMAPOPT_TLS_VERSIONS,
  IMAPOPT_UMASK,
  IMAPOPT_USERDENY_DB,
  IMAPOPT_USERDENY_DB_PATH,
  IMAPOPT_USERNAME_TOLOWER,
  IMAPOPT_USERPREFIX,
  IMAPOPT_UNIX_GROUP_ENABLE,
  IMAPOPT_UNIXHIERARCHYSEP,
  IMAPOPT_VIRTDOMAINS,
  IMAPOPT_LMTP_CATCHALL_MAILBOX,
  IMAPOPT_DEFAULTSEARCHTIER,
  IMAPOPT_ZONEINFO_DB,
  IMAPOPT_ZONEINFO_DB_PATH,

  IMAPOPT_LAST

};



enum enum_value {

  IMAP_ENUM_ZERO = 0,

  IMAP_ENUM_CALDAV_ALLOWSCHEDULING_OFF = 0,
  IMAP_ENUM_CALDAV_ALLOWSCHEDULING_ON,
  IMAP_ENUM_CALDAV_ALLOWSCHEDULING_APPLE,
  IMAP_ENUM_DELETE_MODE_IMMEDIATE = 0,
  IMAP_ENUM_DELETE_MODE_DELAYED,
  IMAP_ENUM_EVENT_CONTENT_INCLUSION_MODE_STANDARD = 0,
  IMAP_ENUM_EVENT_CONTENT_INCLUSION_MODE_MESSAGE,
  IMAP_ENUM_EVENT_CONTENT_INCLUSION_MODE_HEADER,
  IMAP_ENUM_EVENT_CONTENT_INCLUSION_MODE_BODY,
  IMAP_ENUM_EVENT_CONTENT_INCLUSION_MODE_HEADERBODY,
  IMAP_ENUM_EVENT_EXTRA_PARAMS_BODYSTRUCTURE = (1<<0),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_CLIENTADDRESS = (1<<1),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_DISKUSED = (1<<2),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_FLAGNAMES = (1<<3),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_MESSAGECONTENT = (1<<4),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_MESSAGESIZE = (1<<5),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_MESSAGES = (1<<6),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_MODSEQ = (1<<7),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_SERVICE = (1<<8),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_TIMESTAMP = (1<<9),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_UIDNEXT = (1<<10),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_MIDSET = (1<<11),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_UNSEENMESSAGES = (1<<12),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_ENVELOPE = (1<<13),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_SESSIONID = (1<<14),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_MAILBOXACL = (1<<15),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_MBTYPE = (1<<16),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_DAVFILENAME = (1<<17),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_CMU_DAVUID = (1<<18),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_FASTMAIL_CLIENTID = (1<<19),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_FASTMAIL_SESSIONID = (1<<20),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_FASTMAIL_CONVEXISTS = (1<<21),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_FASTMAIL_CONVUNSEEN = (1<<22),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_FASTMAIL_CID = (1<<23),
  IMAP_ENUM_EVENT_EXTRA_PARAMS_VND_FASTMAIL_COUNTERS = (1<<24),
  IMAP_ENUM_EVENT_GROUPS_MESSAGE = (1<<0),
  IMAP_ENUM_EVENT_GROUPS_QUOTA = (1<<1),
  IMAP_ENUM_EVENT_GROUPS_FLAGS = (1<<2),
  IMAP_ENUM_EVENT_GROUPS_ACCESS = (1<<3),
  IMAP_ENUM_EVENT_GROUPS_MAILBOX = (1<<4),
  IMAP_ENUM_EVENT_GROUPS_SUBSCRIPTION = (1<<5),
  IMAP_ENUM_EVENT_GROUPS_CALENDAR = (1<<6),
  IMAP_ENUM_EVENT_GROUPS_APPLEPUSHSERVICE = (1<<7),
  IMAP_ENUM_EXPUNGE_MODE_DEFAULT = 0,
  IMAP_ENUM_EXPUNGE_MODE_IMMEDIATE,
  IMAP_ENUM_EXPUNGE_MODE_DELAYED,
  IMAP_ENUM_HTTPMODULES_CALDAV = (1<<0),
  IMAP_ENUM_HTTPMODULES_CARDDAV = (1<<1),
  IMAP_ENUM_HTTPMODULES_DOMAINKEY = (1<<2),
  IMAP_ENUM_HTTPMODULES_FREEBUSY = (1<<3),
  IMAP_ENUM_HTTPMODULES_ISCHEDULE = (1<<4),
  IMAP_ENUM_HTTPMODULES_JMAP = (1<<5),
  IMAP_ENUM_HTTPMODULES_RSS = (1<<6),
  IMAP_ENUM_HTTPMODULES_TZDIST = (1<<7),
  IMAP_ENUM_INTERNALDATE_HEURISTIC_STANDARD = 0,
  IMAP_ENUM_INTERNALDATE_HEURISTIC_RECEIVEDHEADER,
  IMAP_ENUM_METAPARTITION_FILES_HEADER = (1<<0),
  IMAP_ENUM_METAPARTITION_FILES_INDEX = (1<<1),
  IMAP_ENUM_METAPARTITION_FILES_CACHE = (1<<2),
  IMAP_ENUM_METAPARTITION_FILES_EXPUNGE = (1<<3),
  IMAP_ENUM_METAPARTITION_FILES_SQUAT = (1<<4),
  IMAP_ENUM_METAPARTITION_FILES_ANNOTATIONS = (1<<5),
  IMAP_ENUM_METAPARTITION_FILES_LOCK = (1<<6),
  IMAP_ENUM_METAPARTITION_FILES_DAV = (1<<7),
  IMAP_ENUM_METAPARTITION_FILES_ARCHIVECACHE = (1<<8),
  IMAP_ENUM_MUPDATE_CONFIG_STANDARD = 0,
  IMAP_ENUM_MUPDATE_CONFIG_UNIFIED,
  IMAP_ENUM_MUPDATE_CONFIG_REPLICATED,
  IMAP_ENUM_NEWSADDHEADERS_TO = (1<<0),
  IMAP_ENUM_NEWSADDHEADERS_REPLYTO = (1<<1),
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
  IMAP_ENUM_SEARCH_ENGINE_NONE = 0,
  IMAP_ENUM_SEARCH_ENGINE_SQUAT,
  IMAP_ENUM_SEARCH_ENGINE_SPHINX,
  IMAP_ENUM_SEARCH_ENGINE_XAPIAN,
  IMAP_ENUM_SEARCH_WHITESPACE_SKIP = 0,
  IMAP_ENUM_SEARCH_WHITESPACE_MERGE,
  IMAP_ENUM_SEARCH_WHITESPACE_KEEP,
  IMAP_ENUM_SERVERINFO_OFF = 0,
  IMAP_ENUM_SERVERINFO_MIN,
  IMAP_ENUM_SERVERINFO_ON,
  IMAP_ENUM_SIEVE_EXTENSIONS_FILEINTO = (1<<0),
  IMAP_ENUM_SIEVE_EXTENSIONS_REJECT = (1<<1),
  IMAP_ENUM_SIEVE_EXTENSIONS_VACATION = (1<<2),
  IMAP_ENUM_SIEVE_EXTENSIONS_VACATION_SECONDS = (1<<3),
  IMAP_ENUM_SIEVE_EXTENSIONS_IMAPFLAGS = (1<<4),
  IMAP_ENUM_SIEVE_EXTENSIONS_NOTIFY = (1<<5),
  IMAP_ENUM_SIEVE_EXTENSIONS_INCLUDE = (1<<6),
  IMAP_ENUM_SIEVE_EXTENSIONS_ENVELOPE = (1<<7),
  IMAP_ENUM_SIEVE_EXTENSIONS_BODY = (1<<8),
  IMAP_ENUM_SIEVE_EXTENSIONS_RELATIONAL = (1<<9),
  IMAP_ENUM_SIEVE_EXTENSIONS_REGEX = (1<<10),
  IMAP_ENUM_SIEVE_EXTENSIONS_SUBADDRESS = (1<<11),
  IMAP_ENUM_SIEVE_EXTENSIONS_COPY = (1<<12),
  IMAP_ENUM_SIEVE_EXTENSIONS_DATE = (1<<13),
  IMAP_ENUM_SIEVE_EXTENSIONS_INDEX = (1<<14),
  IMAP_ENUM_SIEVE_EXTENSIONS_IMAP4FLAGS = (1<<15),
  IMAP_ENUM_TLS_CLIENT_CERTS_OFF = 0,
  IMAP_ENUM_TLS_CLIENT_CERTS_OPTIONAL,
  IMAP_ENUM_TLS_CLIENT_CERTS_REQUIRE,
  IMAP_ENUM_VIRTDOMAINS_OFF = 0,
  IMAP_ENUM_VIRTDOMAINS_USERID,
  IMAP_ENUM_VIRTDOMAINS_ON

};



union config_value {

    

    const char *s;      /* OPT_STRING, OPT_STRINGLIST */

    long i;             /* OPT_INT */

    long b;             /* OPT_SWITCH */

    enum enum_value e;  /* OPT_ENUM */

    unsigned long x;    /* OPT_BITFIELD */

};



struct enum_option_s {

    const char *name;

    const enum enum_value val;

};



#define MAX_ENUM_OPTS 25

struct imapopt_s {

    const enum imapopt opt;

    const char *optname;

    int seen;

    const enum opttype t;

    const char *deprecated_since;

    const enum imapopt preferred_opt;

    union config_value val;

    const union config_value def;

    const struct enum_option_s enum_options[MAX_ENUM_OPTS+1];

};



extern struct imapopt_s imapopts[];



#endif /* INCLUDED_IMAPOPTIONS_H */

