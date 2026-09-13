/**
 * network.h - 网络通信函数声明
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

/**
 * 获取本机 IP 地址（通过 UDP 连接到目标服务器来确定出口网卡）
 * @param out_ip  输出缓冲区
 * @param ip_len  缓冲区大小
 * @return 0 成功，-1 失败
 */
int get_local_ip(char *out_ip, size_t ip_len);

/**
 * 执行 HTTP GET 请求
 * @param host     服务器 IP
 * @param port     端口号
 * @param path     请求路径（含查询参数）
 * @param response 响应内容输出缓冲区（含 HTTP 头部）
 * @param resp_size 缓冲区大小
 * @return 0 成功，-1 失败
 */
int http_get(const char *host, int port, const char *path,
             char *response, size_t resp_size);

/**
 * 从 HTTP 响应中提取消息体（跳过 HTTP 头）
 * @param response 完整的 HTTP 响应字符串
 * @return 指向消息体起始位置的指针
 */
const char* http_body(const char *response);

/**
 * 初始化 Winsock
 * @return 0 成功，-1 失败
 */
int net_init(void);

/**
 * 清理 Winsock
 */
void net_cleanup(void);

#endif /* NETWORK_H */
