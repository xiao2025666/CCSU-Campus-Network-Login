/**
 * utils.c - 工具函数实现
 * URL 编码、字符集转换、Base64 编码等
 */

#define _CRT_SECURE_NO_WARNINGS
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* ==================== 字符集转换 ==================== */

char* utf8_to_gbk(const char* utf8_str) {
    if (!utf8_str) return NULL;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (wlen == 0) return NULL;
    wchar_t *wbuf = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wbuf) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wbuf, wlen);
    int glen = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (glen == 0) { free(wbuf); return NULL; }
    char *gbk_str = (char*)malloc(glen);
    if (gbk_str)
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, gbk_str, glen, NULL, NULL);
    free(wbuf);
    return gbk_str;
}

char* gbk_to_utf8(const char* gbk_str) {
    if (!gbk_str) return NULL;
    int wlen = MultiByteToWideChar(CP_ACP, 0, gbk_str, -1, NULL, 0);
    if (wlen == 0) return NULL;
    wchar_t *wbuf = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wbuf) return NULL;
    MultiByteToWideChar(CP_ACP, 0, gbk_str, -1, wbuf, wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (ulen == 0) { free(wbuf); return NULL; }
    char *utf8_str = (char*)malloc(ulen);
    if (utf8_str)
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8_str, ulen, NULL, NULL);
    free(wbuf);
    return utf8_str;
}

/* ==================== URL 编码 ==================== */

void url_encode(const char *src, char *dst, size_t dst_len) {
    static const char *hex = "0123456789ABCDEF";
    size_t i, j = 0;
    for (i = 0; src[i] && j + 4 < dst_len; i++) {
        unsigned char c = (unsigned char)src[i];
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            dst[j++] = c;
        } else if (c == '@' || c == ',') {
            dst[j++] = c;
        } else {
            dst[j++] = '%';
            dst[j++] = hex[c >> 4];
            dst[j++] = hex[c & 0x0F];
        }
    }
    dst[j] = '\0';
}

/* ==================== Base64 编码 ==================== */

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(const unsigned char *input, int len,
                  char *output, size_t out_len) {
    int i, j = 0;
    for (i = 0; i < len; i += 3) {
        if ((size_t)j + 4 > out_len) break;
        unsigned int val = (unsigned int)input[i] << 16;
        if (i + 1 < len) val |= (unsigned int)input[i + 1] << 8;
        if (i + 2 < len) val |= input[i + 2];
        output[j++] = b64_table[(val >> 18) & 0x3F];
        output[j++] = b64_table[(val >> 12) & 0x3F];
        output[j++] = (i + 1 < len) ? b64_table[(val >> 6) & 0x3F] : '=';
        output[j++] = (i + 2 < len) ? b64_table[val & 0x3F] : '=';
    }
    output[j] = '\0';
    return j;
}

/* ==================== 随机数生成 ==================== */

long rand_value(void) {
    return (long)(rand() ^ GetTickCount());
}
