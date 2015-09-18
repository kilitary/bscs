
#define ERR -2
#define SUCCESS 0
#define ENABLE 1
#define DISABLE 0
#define FORMATERROR fmterr()
#define SOCKS_BUF_SIZE 32768

#define CONTROLPORT 83
#define CRM_BUF_SIZE 1024
#define BACK_SOCKS_VERSION 1
#define SIZEOF_HEADERS 1
#define SOCKS_PORT 65530

#ifdef FRELEASE
#define deb //
#else
#define deb fdeb
#endif