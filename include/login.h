/**
 * login.h - Dr.COM 4.0 登录协议函数声明
 *
 * 认证流程（基于抓包分析）:
 *   1. GET /a79.htm (port 80)          → 获取门户页面
 *   2. GET /eportal/portal/page/loadConfig (port 801) → 加载配置
 *   3. GET /drcom/chkstatus (port 80)  → 检查登录状态
 *   4. GET /eportal/portal/login (port 801) → 执行登录
 */

#ifndef LOGIN_H
#define LOGIN_H

#include <stddef.h>

/**
 * 格式化账号字符串（格式: ",0,用户名@后缀" 或 ",0,用户名"）
 * @param account  输出缓冲区
 * @param size     缓冲区大小
 * @param username 用户名
 * @param suffix   后缀（可为空）
 */
void format_account(char *account, size_t size,
                    const char *username, const char *suffix);

/**
 * 构建登录请求路径
 * @param path     输出缓冲区
 * @param size     缓冲区大小
 * @param username 用户名
 * @param password 密码
 * @param suffix   后缀
 * @param local_ip 本机 IP
 * @param callback 回调函数名
 */
void build_login_path(char *path, size_t size,
                      const char *username, const char *password,
                      const char *suffix, const char *local_ip,
                      const char *callback);

/**
 * 完整登录流程（含预检查步骤）
 * @param username 用户名
 * @param password 密码
 * @param suffix   后缀
 * @return LOGIN_OK 成功，ERR_* 失败
 */
int drcom_login_full(const char *username, const char *password,
                     const char *suffix);

/**
 * 快速登录（仅执行 login 请求，兼容旧版）
 * @param username 用户名
 * @param password 密码
 * @param suffix   后缀
 * @return LOGIN_OK 成功，ERR_* 失败
 */
int drcom_login_fast(const char *username, const char *password,
                     const char *suffix);

/**
 * 解析登录结果
 * @param body    HTTP 响应体（JSON 格式）
 * @param msg_buf 输出消息缓冲区（可选）
 * @param msg_size 消息缓冲区大小
 * @return LOGIN_OK 成功，ERR_* 失败
 */
int parse_login_result(const char *body, char *msg_buf, size_t msg_size);

#endif /* LOGIN_H */
