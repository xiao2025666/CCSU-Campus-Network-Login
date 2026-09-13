/**
 * config_reader.h - 外部配置文件读取模块
 *
 * 配置文件格式（drcom.conf）:
 *   ; 或 # 开头的行为注释
 *   key = value   键值对（等号两边空格可选）
 *
 * 示例:
 *   # Dr.COM 登录配置
 *   username = 2025xxxxxx
 *   password = your_password
 *   suffix   = unicom
 *
 * 搜索顺序:
 *   1. 程序所在目录的 drcom.conf
 *   2. 当前工作目录的 drcom.conf
 *   3. 环境变量 DRCOM_CONF 指定路径
 */

#ifndef CONFIG_READER_H
#define CONFIG_READER_H

/* 配置项键名 */
#define CFG_KEY_USERNAME   "username"
#define CFG_KEY_PASSWORD   "password"
#define CFG_KEY_SUFFIX     "suffix"
#define CFG_KEY_SERVER     "server"
#define CFG_KEY_PORT_HTTP  "port_http"
#define CFG_KEY_PORT_PORTAL "port_portal"
#define CFG_KEY_AC_NAME    "ac_name"
#define CFG_KEY_JS_VERSION "js_version"
#define CFG_KEY_TERM_TYPE  "terminal_type"

/* 配置值缓冲区大小 */
#define CFG_VAL_BUF        128

/**
 * Dr.COM 登录配置结构体
 */
typedef struct {
    char username[CFG_VAL_BUF];
    char password[CFG_VAL_BUF];
    char suffix[CFG_VAL_BUF];
    /* 以下为可选覆盖项，空字符串表示使用编译期默认值 */
    char server[CFG_VAL_BUF];
    char ac_name[CFG_VAL_BUF];
    char js_version[CFG_VAL_BUF];
    int  port_http;
    int  port_portal;
    int  terminal_type;
} DrcomConfig;

/**
 * 初始化配置结构体为默认值
 */
void config_init(DrcomConfig *cfg);

/**
 * 从文件读取配置
 * @param cfg  输出配置结构体
 * @param path 配置文件路径（NULL 则自动搜索）
 * @return 0 成功，-1 失败
 */
int config_load(DrcomConfig *cfg, const char *path);

/**
 * 自动搜索并加载配置文件
 * @param cfg 输出配置结构体
 * @return 0 成功（找到并加载），1 未找到配置文件，-1 读取错误
 */
int config_auto_load(DrcomConfig *cfg);

/**
 * 打印当前配置（隐藏密码）
 * @param cfg 配置结构体
 */
void config_print(const DrcomConfig *cfg);

#endif /* CONFIG_READER_H */
