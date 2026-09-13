/**
 * config_reader.c - 外部配置文件读取实现
 *
 * 支持 INI 风格 key = value 格式
 * 支持 # 和 ; 注释
 * 支持空行
 */

#define _CRT_SECURE_NO_WARNINGS
#include "config_reader.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* ==================== 默认值 ==================== */

void config_init(DrcomConfig *cfg) {
    if (!cfg) return;
    cfg->username[0]   = '\0';
    cfg->password[0]   = '\0';
    cfg->suffix[0]     = '\0';
    cfg->server[0]     = '\0';
    cfg->ac_name[0]    = '\0';
    cfg->js_version[0] = '\0';
    cfg->port_http     = 0;
    cfg->port_portal   = 0;
    cfg->terminal_type = 0;
}

/* ==================== 字符串修剪 ==================== */

static const char* trim_start(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static void trim_end(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                       s[len - 1] == '\r' || s[len - 1] == '\n'))
        s[--len] = '\0';
}

/* ==================== 单行解析 ==================== */

static int parse_line(const char *line, char *key, size_t key_size,
                      char *val, size_t val_size) {
    const char *p = trim_start(line);

    /* 跳过空行和注释 */
    if (*p == '\0' || *p == '#' || *p == ';' || *p == '\n')
        return 0;

    /* 查找 = 分隔符 */
    const char *eq = strchr(p, '=');
    if (!eq) return 0;

    /* 提取 key */
    size_t klen = (size_t)(eq - p);
    if (klen >= key_size) klen = key_size - 1;
    strncpy(key, p, klen);
    key[klen] = '\0';
    trim_end(key);

    if (key[0] == '\0') return 0;

    /* 提取 value */
    const char *vstart = trim_start(eq + 1);
    strncpy(val, vstart, val_size - 1);
    val[val_size - 1] = '\0';
    trim_end(val);

    return 1;
}

/* ==================== 设置配置值 ==================== */

/* 安全字符串复制，确保以 null 结尾 */
static void safe_strcpy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0 || !src) return;
    size_t len = strlen(src);
    if (len >= dst_size) len = dst_size - 1;
    memmove(dst, src, len);
    dst[len] = '\0';
}

static void config_set(DrcomConfig *cfg, const char *key, const char *val) {
    if (!cfg || !key || !val) return;

    if (strcmp(key, CFG_KEY_USERNAME) == 0)
        safe_strcpy(cfg->username, sizeof(cfg->username), val);
    else if (strcmp(key, CFG_KEY_PASSWORD) == 0)
        safe_strcpy(cfg->password, sizeof(cfg->password), val);
    else if (strcmp(key, CFG_KEY_SUFFIX) == 0)
        safe_strcpy(cfg->suffix, sizeof(cfg->suffix), val);
    else if (strcmp(key, CFG_KEY_SERVER) == 0)
        safe_strcpy(cfg->server, sizeof(cfg->server), val);
    else if (strcmp(key, CFG_KEY_AC_NAME) == 0)
        safe_strcpy(cfg->ac_name, sizeof(cfg->ac_name), val);
    else if (strcmp(key, CFG_KEY_JS_VERSION) == 0)
        safe_strcpy(cfg->js_version, sizeof(cfg->js_version), val);
    else if (strcmp(key, CFG_KEY_PORT_HTTP) == 0)
        cfg->port_http = atoi(val);
    else if (strcmp(key, CFG_KEY_PORT_PORTAL) == 0)
        cfg->port_portal = atoi(val);
    else if (strcmp(key, CFG_KEY_TERM_TYPE) == 0)
        cfg->terminal_type = atoi(val);
}

/* ==================== 加载配置文件 ==================== */

int config_load(DrcomConfig *cfg, const char *path) {
    if (!cfg || !path) return -1;

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    config_init(cfg);

    char line[512];
    char key[64], val[256];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (parse_line(line, key, sizeof(key), val, sizeof(val))) {
            config_set(cfg, key, val);
        }
    }

    fclose(fp);
    return 0;
}

/* ==================== 自动搜索配置文件 ==================== */

int config_auto_load(DrcomConfig *cfg) {
    if (!cfg) return -1;

    /* 1. 优先检查环境变量 DRCOM_CONF */
    char *env_path = getenv("DRCOM_CONF");
    if (env_path && env_path[0]) {
        if (config_load(cfg, env_path) == 0)
            return 0;
    }

    /* 2. 检查程序所在目录的 drcom.conf */
    char exe_path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if (len > 0 && len < sizeof(exe_path)) {
        /* 找到最后一个 \ 或 /，截断为目录 */
        char *sep = strrchr(exe_path, '\\');
        if (!sep) sep = strrchr(exe_path, '/');
        if (sep) {
            size_t dir_len = (size_t)(sep - exe_path + 1);
            char conf_path[MAX_PATH];
            snprintf(conf_path, sizeof(conf_path),
                     "%.*sdrcom.conf", (int)dir_len, exe_path);
            if (config_load(cfg, conf_path) == 0)
                return 0;
        }
    }

    /* 3. 检查当前工作目录的 drcom.conf */
    if (config_load(cfg, "drcom.conf") == 0)
        return 0;

    /* 未找到配置文件 */
    return 1;
}

/* ==================== 打印配置 ==================== */

void config_print(const DrcomConfig *cfg) {
    if (!cfg) return;
    printf("[配置]\n");
    if (cfg->username[0])
        printf("  用户名: %s\n", cfg->username);
    if (cfg->password[0]) {
        /* 只显示前4位 + ****，保护隐私 */
        size_t plen = strlen(cfg->password);
        if (plen > 4)
            printf("  密码: %.4s****\n", cfg->password);
        else
            printf("  密码: ****\n");
    }
    if (cfg->suffix[0])
        printf("  后缀: %s\n", cfg->suffix);
    if (cfg->server[0])
        printf("  服务器: %s\n", cfg->server);
    if (cfg->ac_name[0])
        printf("  AC名称: %s\n", cfg->ac_name);
}
