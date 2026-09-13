/**
 * network.c - 网络通信函数实现
 * HTTP 客户端、本地 IP 探测
 */

#define _CRT_SECURE_NO_WARNINGS
#include "network.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

/* ==================== Winsock 管理 ==================== */

int net_init(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "[错误] WSAStartup 失败\n");
        return -1;
    }
    return 0;
}

void net_cleanup(void) {
    WSACleanup();
}

/* ==================== 本地 IP 获取 ==================== */

int get_local_ip(char *out_ip, size_t ip_len) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) return -1;

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons((u_short)PORT_PORTAL);
    dest.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) == SOCKET_ERROR) {
        closesocket(sock);
        return -1;
    }

    struct sockaddr_in local;
    int len = sizeof(local);
    if (getsockname(sock, (struct sockaddr*)&local, &len) == SOCKET_ERROR) {
        closesocket(sock);
        return -1;
    }

    strncpy(out_ip, inet_ntoa(local.sin_addr), ip_len - 1);
    out_ip[ip_len - 1] = '\0';
    closesocket(sock);
    return 0;
}

/* ==================== HTTP GET 请求 ==================== */

int http_get(const char *host, int port, const char *path,
             char *response, size_t resp_size) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return -1;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((u_short)port);
    server.sin_addr.s_addr = inet_addr(host);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        closesocket(sock);
        return -1;
    }

    int timeout = TIMEOUT_SEC * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    char request[REQ_BUF];
    int req_len = snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
        "\r\n", path, host, port);

    if (send(sock, request, req_len, 0) == SOCKET_ERROR) {
        closesocket(sock);
        return -1;
    }

    size_t total = 0;
    int n;
    while (total < resp_size - 1) {
        n = recv(sock, response + total, (int)(resp_size - total - 1), 0);
        if (n <= 0) break;
        total += n;
    }
    response[total] = '\0';
    closesocket(sock);
    return 0;
}

/* ==================== HTTP 响应体提取 ==================== */

const char* http_body(const char *response) {
    const char *body = strstr(response, "\r\n\r\n");
    if (body) body += 4;
    else body = response;

    // 处理 Chunked Transfer Encoding: 去除 chunk 大小标记
    // 格式: <hex_size>\r\n<chunk_data>\r\n... 0\r\n\r\n
    if (body[0] && strchr("0123456789ABCDEFabcdef", (unsigned char)body[0])) {
        // 尝试检测是否为 chunked: 以 hex 数字开头，后跟 \r\n
        const char *crlf = strstr(body, "\r\n");
        if (crlf) {
            // 检查 hex size 是否合法（全十六进制字符）
            const char *p = body;
            int is_hex = 1;
            while (p < crlf) {
                if (!strchr("0123456789ABCDEFabcdef", (unsigned char)*p)) {
                    is_hex = 0;
                    break;
                }
                p++;
            }
            if (is_hex && crlf > body) {
                // 跳过第一个 chunk size 行，从实际数据开始
                body = crlf + 2;
                // 找到数据结束位置（最后一个 0\r\n\r\n）
                const char *end = strstr(body, "\r\n0\r\n\r\n");
                if (!end) end = strstr(body, "0\r\n\r\n");
                if (end) {
                    // 截断至有效数据
                    static char chunk_buf[RESP_BUF];
                    size_t data_len = (size_t)(end - body);
                    if (data_len >= sizeof(chunk_buf))
                        data_len = sizeof(chunk_buf) - 1;
                    memmove(chunk_buf, body, data_len);
                    chunk_buf[data_len] = '\0';
                    return chunk_buf;
                }
            }
        }
    }
    return body;
}
