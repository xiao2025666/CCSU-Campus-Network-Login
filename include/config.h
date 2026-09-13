/**
 * config.h - Dr.COM 4.0 登录程序配置常量
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== 网络配置 ==================== */
#define SERVER_IP       "10.0.100.3"   /* 认证服务器地址 */
#define PORT_HTTP       80             /* HTTP 端口（a79.htm、chkstatus） */
#define PORT_PORTAL     801            /* 门户端口（loadConfig、login） */
#define TIMEOUT_SEC     5              /* Socket 超时秒数 */

/* ==================== 设备/账户信息 ==================== */
#define AC_NAME         "ME60-CSDX"    /* AC 名称 */
#define JS_VERSION      "4.2.1"        /* JS 版本号 */
#define TERM_TYPE       1              /* 终端类型 */

/* ==================== 缓冲区大小 ==================== */
#define IP_BUF_SIZE     16
#define ACCOUNT_BUF     256
#define ENC_BUF         512
#define PATH_BUF        2048
#define REQ_BUF         4096
#define RESP_BUF        16384

/* ==================== 返回码 ==================== */
#define LOGIN_OK        0       /* 登录成功 */
#define ERR_ONLINE      2       /* 已在线（result:2） */
#define ERR_NETWORK     -1      /* 网络错误 */
#define ERR_AUTH        -2      /* 认证失败 */
#define ERR_PARAM       -3      /* 参数错误 */

#endif /* CONFIG_H */
