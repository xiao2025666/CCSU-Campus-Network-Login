# ****************************************************************************
# Makefile - Dr.COM 4.0 校园网一键登录程序
#
# 支持的编译器:
#   MinGW (gcc)  - 默认
#   MSVC (cl)    - mingw32-make MSVC=1
#   MinGW64      - 自动检测
#
# 用法:
#   make             编译（MinGW）
#   make MSVC=1      编译（MSVC）
#   make clean       清理
#   make run         编译并运行
#   make help        显示帮助
# ****************************************************************************

# ==================== 项目配置 ====================
TARGET      := drcom_login.exe
SRC_DIR     := src
INC_DIR     := include
BUILD_DIR   := build

# 源文件
SRCS        := $(wildcard $(SRC_DIR)/*.c)
OBJS        := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# ==================== 编译器检测 ====================
# 如果指定了 MSVC=1 或环境中有 cl.exe，使用 MSVC
ifdef MSVC
    CC      := cl
else
    # 检测是否有 x86_64-w64-mingw32-gcc（MinGW64）
    ifneq (,$(wildcard $(shell which x86_64-w64-mingw32-gcc 2>/dev/null)))
        CC  := x86_64-w64-mingw32-gcc
    else
        CC  := gcc
    endif
endif

# ==================== MinGW 编译 ====================
ifneq ($(CC),cl)
    CFLAGS      := -O2 -Wall -Wextra
    CFLAGS      += -I$(INC_DIR)
    CFLAGS      += -finput-charset=UTF-8 -fexec-charset=GBK
    LDFLAGS     := -lws2_32
    RM          := del /Q 2>nul || rm -f 2>/dev/null
    MKDIR       := mkdir

    # 调试版本
    ifdef DEBUG
        CFLAGS  := -O0 -g -Wall -Wextra -DDEBUG -I$(INC_DIR)
        CFLAGS  += -finput-charset=UTF-8 -fexec-charset=GBK
    endif

    # 链接
    $(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "========================================"
	@echo "  编译完成: $(TARGET)"
	@echo "  用法: ./$(TARGET) --help"
	@echo "========================================"

    # 编译
    $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# ==================== MSVC 编译 ====================
else
    CFLAGS      := /O2 /W3 /I$(INC_DIR)
    LDFLAGS     := ws2_32.lib
    RM          := del /Q
    MKDIR       := if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

    $(TARGET): $(OBJS)
	$(CC) $(CFLAGS) /Fe:$@ $** $(LDFLAGS)
	@echo.
	@echo ========================================
	@echo   编译完成: $(TARGET)
	@echo ========================================

    $(BUILD_DIR)/%.obj: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) /c /Fo$@ $<

    OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.obj)
endif

# ==================== 目录创建 ====================
$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

# ==================== 伪目标 ====================
.PHONY: all clean run help debug

all: $(TARGET)

# 运行
run: $(TARGET)
	./$(TARGET)

# 调试编译
debug:
	$(MAKE) DEBUG=1

# 清理
clean:
	-if exist $(TARGET) del /Q $(TARGET)
	-if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
	@echo "已清理"

# 帮助
help:
	@echo "Dr.COM 4.0 校园网一键登录程序 - Makefile"
	@echo ""
	@echo "目标:"
	@echo "  make             编译（默认，MinGW gcc）"
	@echo "  make MSVC=1      使用 MSVC 编译"
	@echo "  make debug       调试模式编译（含符号）"
	@echo "  make run         编译并运行"
	@echo "  make clean       清理构建文件"
	@echo "  make help        显示此帮助"
	@echo ""
	@echo "默认账号配置在 include/config.h 中"
	@echo ""
	@echo "示例:"
	@echo "  make && make run              # 编译并运行"
	@echo "  make MSVC=1                   # MSVC 编译"
	@echo "  make DEBUG=1 run              # 调试模式运行"

# 显示编译器版本
version:
	$(CC) --version
