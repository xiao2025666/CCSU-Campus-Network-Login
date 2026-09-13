/**
 * main.c - Dr.COM 4.0 一键登录程序入口
 *
 * 编译命令:
 *   make                     # 自动编译
 *   make MSVC=1              # MSVC 编译
 *
 * 用法:
 *   drcom_login.exe                      从 drcom.conf 读取账号登录
 *   drcom_login.exe --fast               快速模式
 *   drcom_login.exe <用户名> <密码> [后缀]  使用指定账号
 *   drcom_login.exe --conf <路径>         指定配置文件路径
 *   drcom_login.exe --help               显示帮助
 *
 * 配置文件搜索顺序:
 *   1. 环境变量 DRCOM_CONF 指定路径
 *   2. 程序所在目录的 drcom.conf
 *   3. 当前工作目录的 drcom.conf
 */

#define _CRT_SECURE_NO_WARNINGS
#include "config.h"
#include "config_reader.h"
#include "login.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_banner(void) {
    printf("========================================\n");
    printf("  Dr.COM 4.0 校园网一键登录程序\n");
    printf("  版本: 2.0 | 配置: drcom.conf\n");
    printf("========================================\n");
}

static void print_help(const char *prog) {
    printf("用法:\n");
    printf("  %s                        从 drcom.conf 读取账号登录\n", prog);
    printf("  %s --fast                 快速模式（单步直达）\n", prog);
    printf("  %s <用户名> <密码> [后缀]   使用指定账号登录\n", prog);
    printf("  %s --conf <路径>           指定配置文件\n", prog);
    printf("  %s --help                 显示此帮助\n", prog);
    printf("\n");
    printf("示例:\n");
    printf("  %s                                              # 读取 drcom.conf\n", prog);
    printf("  %s --fast                                       # 快速登录\n", prog);
    printf("  %s 2025xxxxxx your_password unicom                 # 命令行指定\n", prog);
    printf("  %s --conf ../my.conf                             # 自定义配置\n", prog);
    printf("\n");
    printf("配置文件格式（drcom.conf）:\n");
    printf("  username = 学号\n");
    printf("  password = 密码\n");
    printf("  suffix   = 运营商后缀（如 unicom、dx、yd）\n");
}

int main(int argc, char *argv[]) {
    int fast_mode = 0;
    DrcomConfig cfg;
    const char *username = NULL;
    const char *password = NULL;
    const char *suffix   = NULL;
    const char *conf_path = NULL;
    int has_cli_account = 0;  /* 是否通过命令行指定了账号 */

    /* ========== 解析命令行参数 ========== */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "/?") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--fast") == 0 || strcmp(argv[i], "-f") == 0) {
            fast_mode = 1;
            continue;
        }
        if (strcmp(argv[i], "--conf") == 0 || strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) conf_path = argv[++i];
            continue;
        }
        /* 位置参数: 用户名 密码 [后缀] */
        if (!has_cli_account && argv[i][0] != '-') {
            username = argv[i];
            password = (i + 1 < argc) ? argv[++i] : NULL;
            suffix   = (i + 1 < argc && argv[i + 1][0] != '-') ? argv[++i] : "";
            if (suffix && suffix[0] == '@') suffix++;  /* 去掉 @ 前缀 */
            has_cli_account = 1;
        }
    }

    /* ========== 读取配置文件 ========== */
    config_init(&cfg);
    int conf_ret;
    if (conf_path) {
        conf_ret = config_load(&cfg, conf_path);
    } else {
        conf_ret = config_auto_load(&cfg);
    }

    /* ========== 确定最终账号 ========== */
    /* 优先级: 命令行 > 配置文件 > 提示用户 */
    if (!has_cli_account) {
        if (conf_ret == 0 && cfg.username[0]) {
            username = cfg.username;
            password = cfg.password;
            suffix   = cfg.suffix;
        } else {
            /* 既没有命令行参数，也没有配置文件 */
            print_banner();
            printf("[错误] 未找到配置文件 drcom.conf\n");
            printf("      请创建 drcom.conf 或在命令行直接指定账号。\n\n");
            print_help(argv[0]);
            return 1;
        }
    }

    /* ========== 初始化 ========== */
    srand((unsigned int)time(NULL));
    print_banner();

    if (conf_ret == 0) {
        printf("[配置] 已加载配置文件");
        if (conf_path) printf(": %s", conf_path);
        printf("\n");
    }

    if (net_init() != 0) {
        fprintf(stderr, "[错误] 网络初始化失败\n");
        return 1;
    }

    printf("[信息] 账号: %s", username);
    if (suffix && suffix[0]) printf("@%s", suffix);
    printf("\n");

    if (fast_mode) {
        printf("[信息] 模式: 快速登录（单步）\n");
    } else {
        printf("[信息] 模式: 完整流程（4步）\n");
    }
    printf("\n");

    /* ========== 执行登录 ========== */
    int ret;
    if (fast_mode) {
        ret = drcom_login_fast(username, password, suffix);
    } else {
        ret = drcom_login_full(username, password, suffix);
    }

    net_cleanup();

    printf("\n");
    if (ret == LOGIN_OK) {
        printf("登录成功，按任意键退出...\n");
        getchar();
        return 0;
    } else if (ret == ERR_ONLINE) {
        printf("账号已在线，按任意键退出...\n");
        getchar();
        return 0;
    } else {
        printf("登录失败，按任意键退出...\n");
        getchar();
        return 1;
    }
}
