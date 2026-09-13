/**
 * login.c - Dr.COM 4.0 登录协议实现
 * 
 * 根据抓包分析，完整协议流程:
 * 1. GET /a79.htm?userip=IP&wlanacname=AC_NAME&wlancip=WLANCIP
 *    获取认证页面（检测是否已认证）
 * 2. GET /eportal/portal/page/loadConfig?callback=XXX&...
 *    加载门户配置（wlan_user_ip 为 Base64 编码）
 * 3. GET /drcom/chkstatus?callback=XXX&...
 *    检查当前登录状态
 * 4. GET /eportal/portal/login?callback=XXX&login_method=1&...
 *    执行登录
 */

#define _CRT_SECURE_NO_WARNINGS
#include "login.h"
#include "config.h"
#include "utils.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ==================== 账号格式化 ==================== */

void format_account(char *account, size_t size,
                    const char *username, const char *suffix) {
    if (suffix && suffix[0] != '\0') {
        snprintf(account, size, ",0,%s@%s", username, suffix);
    } else {
        snprintf(account, size, ",0,%s", username);
    }
}

/* ==================== 构建登录路径 ==================== */

void build_login_path(char *path, size_t size,
                      const char *username, const char *password,
                      const char *suffix, const char *local_ip,
                      const char *callback) {
    char account[ACCOUNT_BUF];
    char enc_account[ENC_BUF], enc_password[ENC_BUF];

    format_account(account, sizeof(account), username, suffix);
    url_encode(account, enc_account, sizeof(enc_account));
    url_encode(password, enc_password, sizeof(enc_password));

    snprintf(path, size,
        "/eportal/portal/login"
        "?callback=%s"
        "&login_method=1"
        "&user_account=%s"
        "&user_password=%s"
        "&wlan_user_ip=%s"
        "&wlan_user_ipv6="
        "&wlan_user_mac=000000000000"
        "&wlan_ac_ip="
        "&wlan_ac_name=%s"
        "&jsVersion=%s"
        "&terminal_type=%d"
        "&lang=zh-cn"
        "&v=%ld"
        "&lang=zh",
        callback,
        enc_account, enc_password, local_ip,
        AC_NAME, JS_VERSION, TERM_TYPE,
        rand_value());
}

/* ==================== 解析 JSON 整数字段 ==================== */

/* 从 JSON 中提取整数字段值，如 "ret_code":2 */
static int json_get_int(const char *body, const char *field, int *value) {
    if (!body || !field || !value) return -1;

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", field);
    const char *fp = strstr(body, pattern);
    if (!fp) return -1;

    fp = strchr(fp, ':');
    if (!fp) return -1;
    fp++;
    while (*fp == ' ') fp++;

    if (*fp == '"') fp++;  // 跳过引号
    *value = atoi(fp);
    return 0;
}

/* 从 JSON 中提取字符串字段，如 "msg":"xxx" */
static int json_get_str(const char *body, const char *field,
                        char *buf, size_t buf_size) {
    if (!body || !field || !buf || buf_size == 0) return -1;

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", field);
    const char *fp = strstr(body, pattern);
    if (!fp) return -1;

    fp = strchr(fp, ':');
    if (!fp) return -1;
    fp++;
    while (*fp == ' ') fp++;

    if (*fp != '"') return -1;
    fp++;

    const char *end = strchr(fp, '"');
    if (!end) return -1;

    size_t len = (size_t)(end - fp);
    if (len >= buf_size) len = buf_size - 1;
    memmove(buf, fp, len);
    buf[len] = '\0';
    return 0;
}

/* ==================== 解析登录结果 ==================== */

int parse_login_result(const char *body, char *msg_buf, size_t msg_size) {
    if (!body) return ERR_NETWORK;

    int result = -1;
    if (json_get_int(body, "result", &result) != 0) {
        /* 兼容 callback 包裹格式: dr1003({...}) */
        const char *json_start = strchr(body, '{');
        const char *json_end   = strrchr(body, '}');
        if (json_start && json_end && json_end > json_start) {
            char wrapped[RESP_BUF];
            size_t len = (size_t)(json_end - json_start + 1);
            if (len >= sizeof(wrapped)) len = sizeof(wrapped) - 1;
            memmove(wrapped, json_start, len);
            wrapped[len] = '\0';
            json_get_int(wrapped, "result", &result);
        }
    }

    /* 提取 msg */
    if (msg_buf && msg_size > 0) {
        msg_buf[0] = '\0';
        json_get_str(body, "msg", msg_buf, msg_size);
        if (msg_buf[0] == '\0') {
            /* 尝试从包裹的 JSON 中提取 */
            const char *json_start = strchr(body, '{');
            const char *json_end   = strrchr(body, '}');
            if (json_start && json_end && json_end > json_start) {
                char wrapped[RESP_BUF];
                size_t len = (size_t)(json_end - json_start + 1);
                if (len >= sizeof(wrapped)) len = sizeof(wrapped) - 1;
                memmove(wrapped, json_start, len);
                wrapped[len] = '\0';
                json_get_str(wrapped, "msg", msg_buf, msg_size);
            }
        }
    }

    /* 判断结果 */
    if (result == 1) return LOGIN_OK;          /* 登录成功 */

    if (result == 0) {
        int ret_code = -1;
        if (json_get_int(body, "ret_code", &ret_code) == 0 && ret_code == 2)
            return ERR_ONLINE;                 /* 已在线 */
        return ERR_AUTH;                       /* 认证失败 */
    }

    if (result == 2) return ERR_ONLINE;        /* 已在线 */
    return ERR_AUTH;
}

/* ==================== 构建 loadConfig 路径 ==================== */

static void build_config_path(char *path, size_t size,
                               const char *local_ip) {
    // wlan_user_ip 需要 Base64 编码
    char ip_b64[64];
    base64_encode((const unsigned char*)local_ip,
                  (int)strlen(local_ip), ip_b64, sizeof(ip_b64));

    // URL encode the base64 result
    char enc_ip[128];
    url_encode(ip_b64, enc_ip, sizeof(enc_ip));

    snprintf(path, size,
        "/eportal/portal/page/loadConfig"
        "?callback=dr1001"
        "&program_index="
        "&wlan_vlan_id=0"
        "&wlan_user_ip=%s"
        "&wlan_user_ipv6="
        "&wlan_user_ssid="
        "&wlan_user_areaid="
        "&wlan_ac_ip="
        "&wlan_ap_mac=000000000000"
        "&gw_id=000000000000"
        "&jsVersion=4.X"
        "&v=%ld"
        "&lang=zh",
        enc_ip, rand_value());
}

/* ==================== 构建 chkstatus 路径 ==================== */

static void build_chkstatus_path(char *path, size_t size) {
    snprintf(path, size,
        "/drcom/chkstatus"
        "?callback=dr1002"
        "&jsVersion=4.X"
        "&v=%ld"
        "&lang=zh",
        rand_value());
}

/* ==================== 构建 a79.htm 路径 ==================== */

static void build_a79_path(char *path, size_t size,
                            const char *local_ip) {
    snprintf(path, size,
        "/a79.htm"
        "?userip=%s"
        "&wlanacname=%s"
        "&wlancip=10.0.100.1",
        local_ip, AC_NAME);
}

/* ==================== 完整登录（4 步流程） ==================== */

int drcom_login_full(const char *username, const char *password,
                     const char *suffix) {
    char local_ip[IP_BUF_SIZE];
    char path[PATH_BUF];
    char response[RESP_BUF];
    char msg[256];

    // ---- 步骤 0: 获取本机 IP ----
    if (get_local_ip(local_ip, sizeof(local_ip)) != 0) {
        fprintf(stderr, "[错误] 无法获取本机 IP，请检查网络连接\n");
        return ERR_NETWORK;
    }
    printf("[信息] 本机 IP: %s\n", local_ip);

    // ---- 步骤 1: 获取门户页面（检测网络状态） ----
    build_a79_path(path, sizeof(path), local_ip);
    printf("[步骤 1/4] 获取门户页面...\n");
    if (http_get(SERVER_IP, PORT_HTTP, path, response, sizeof(response)) != 0) {
        fprintf(stderr, "[错误] 门户页面请求失败\n");
        return ERR_NETWORK;
    }
    const char *a79_body = http_body(response);
    // 检查响应中是否已登录
    if (strstr(a79_body, "已登录") || strstr(a79_body, "online")) {
        printf("[信息] 似乎已经登录，继续执行登录流程...\n");
    }

    // ---- 步骤 2: 加载配置 ----
    build_config_path(path, sizeof(path), local_ip);
    printf("[步骤 2/4] 加载门户配置...\n");
    if (http_get(SERVER_IP, PORT_PORTAL, path, response, sizeof(response)) != 0) {
        fprintf(stderr, "[警告] 配置加载失败（继续执行）\n");
    } else {
        const char *cfg_body = http_body(response);
        // 显示配置响应（简短显示）
        if (strlen(cfg_body) > 120) {
            printf("  └─ 配置加载成功 (%u bytes)\n", (unsigned)strlen(cfg_body));
        } else {
            char *gbk = utf8_to_gbk(cfg_body);
            printf("  └─ %s\n", gbk ? gbk : cfg_body);
            free(gbk);
        }
    }

    // ---- 步骤 3: 检查登录状态 ----
    build_chkstatus_path(path, sizeof(path));
    printf("[步骤 3/4] 检查登录状态...\n");
    if (http_get(SERVER_IP, PORT_HTTP, path, response, sizeof(response)) != 0) {
        fprintf(stderr, "[警告] 状态检查失败（继续执行）\n");
    } else {
        const char *st_body = http_body(response);
        char *gbk = utf8_to_gbk(st_body);
        printf("  └─ %s\n", gbk ? gbk : st_body);
        free(gbk);
    }

    // ---- 步骤 4: 执行登录 ----
    build_login_path(path, sizeof(path), username, password,
                     suffix, local_ip, "dr1003");
    printf("[步骤 4/4] 执行登录...\n");
    printf("  └─ 账号: %s", username);
    if (suffix && suffix[0]) printf("@%s", suffix);
    printf("\n");

    if (http_get(SERVER_IP, PORT_PORTAL, path, response, sizeof(response)) != 0) {
        fprintf(stderr, "[错误] 登录请求失败（服务器无响应）\n");
        return ERR_NETWORK;
    }

    const char *body = http_body(response);
    char *body_gbk = utf8_to_gbk(body);
    printf("  └─ 响应: %s\n", body_gbk ? body_gbk : body);

    int ret = parse_login_result(body, msg, sizeof(msg));
    if (msg[0]) {
        char *msg_gbk = utf8_to_gbk(msg);
        printf("  └─ 消息: %s\n", msg_gbk ? msg_gbk : msg);
        free(msg_gbk);
    }

    if (body_gbk) free(body_gbk);

    if (ret == LOGIN_OK) {
        printf("[成功] 登录成功！\n");
        return LOGIN_OK;
    } else if (ret == ERR_ONLINE) {
        printf("[信息] 该账号已在线，无需重复登录。\n");
        return ERR_ONLINE;
    } else {
        printf("[失败] 认证失败，请检查账号/密码/后缀。\n");
        return ERR_AUTH;
    }
}

/* ==================== 快速登录（单步，兼容旧版） ==================== */

int drcom_login_fast(const char *username, const char *password,
                     const char *suffix) {
    char local_ip[IP_BUF_SIZE];
    char path[PATH_BUF];
    char response[RESP_BUF];
    char msg[256];

    if (get_local_ip(local_ip, sizeof(local_ip)) != 0) {
        fprintf(stderr, "[错误] 无法获取本机 IP，请检查网络连接\n");
        return ERR_NETWORK;
    }
    printf("[信息] 本机 IP: %s\n", local_ip);

    build_login_path(path, sizeof(path), username, password,
                     suffix, local_ip, "drcom");
    printf("[信息] 请求路径: %s\n", path);

    if (http_get(SERVER_IP, PORT_PORTAL, path, response, sizeof(response)) != 0) {
        fprintf(stderr, "[错误] HTTP 请求失败（服务器无响应）\n");
        return ERR_NETWORK;
    }

    const char *body = http_body(response);
    char *body_gbk = utf8_to_gbk(body);
    printf("[响应] %s\n", body_gbk ? body_gbk : body);

    int ret = parse_login_result(body, msg, sizeof(msg));
    if (msg[0]) {
        char *msg_gbk = utf8_to_gbk(msg);
        printf("  └─ %s\n", msg_gbk ? msg_gbk : msg);
        free(msg_gbk);
    }
    if (body_gbk) free(body_gbk);

    if (ret == LOGIN_OK) {
        printf("[成功] 登录成功！\n");
        return LOGIN_OK;
    } else if (ret == ERR_ONLINE) {
        printf("[信息] 该账号已在线，无需重复登录。\n");
        return ERR_ONLINE;
    } else {
        printf("[失败] 认证失败，请检查账号/密码/后缀。\n");
        return ERR_AUTH;
    }
}
