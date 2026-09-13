/**
 * utils.h - 工具函数声明
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/**
 * UTF-8 字符串转 GBK
 * @param utf8_str UTF-8 输入字符串
 * @return GBK 编码字符串（需调用 free() 释放），失败返回 NULL
 */
char* utf8_to_gbk(const char* utf8_str);

/**
 * GBK 字符串转 UTF-8
 * @param gbk_str GBK 输入字符串
 * @return UTF-8 编码字符串（需调用 free() 释放），失败返回 NULL
 */
char* gbk_to_utf8(const char* gbk_str);

/**
 * URL 编码（保留字母数字、- _ . ~ @ ,）
 * @param src     源字符串
 * @param dst     输出缓冲区
 * @param dst_len 输出缓冲区大小
 */
void url_encode(const char *src, char *dst, size_t dst_len);

/**
 * Base64 编码
 * @param input  输入数据
 * @param len    输入长度
 * @param output 输出缓冲区
 * @param out_len 输出缓冲区大小
 * @return 编码后的字符串长度
 */
int base64_encode(const unsigned char *input, int len, char *output, size_t out_len);

/**
 * 生成随机数（用于 v 参数）
 * @return 随机整数值
 */
long rand_value(void);

#endif /* UTILS_H */
