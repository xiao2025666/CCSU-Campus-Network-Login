@echo off
chcp 65001 >nul
setlocal
title Dr.COM 校园网一键登录 - 首次配置向导
cd /d "%~dp0"

set "ICON=%~dp0drcom_login.ico"
set "EXE=%~dp0drcom_login.exe"

echo ================================================================
echo    Dr.COM 4.0 校园网一键登录 - 首次配置向导
echo ================================================================
echo.
echo   本脚本将完成两件事:
echo     1. 设置登录程序图标（嵌入 exe + 创建桌面快捷方式）
echo     2. 输入账号密码，一键生成配置文件 drcom.conf
echo.

REM ================================================================
REM  步骤 1/3 : 设置登录程序图标
REM ================================================================
echo [1/3] 设置登录程序图标...
echo.

if not exist "%ICON%" (
    echo   [警告] 未找到图标文件 drcom_login.ico，跳过图标设置。
    goto :step2
)

REM ---- A. 将图标嵌入 drcom_login.exe ----
call :which windres
if errorlevel 1 (
    echo   [跳过] 未检测到 windres ^(MinGW^)，无法把图标嵌入 exe。
    goto :shortcut
)

call :which gcc
if errorlevel 1 (
    echo   [跳过] 未检测到 gcc，无法重新编译 exe。
    goto :shortcut
)

echo   正在编译图标资源...
> "drcom_login.rc" echo IDI_ICON1 ICON "drcom_login.ico"
windres "drcom_login.rc" -O coff -o "drcom_login.res"
if errorlevel 1 (
    echo   [警告] 资源编译失败，跳过图标嵌入。
    goto :shortcut
)

echo   正在重新编译程序（含图标）...
gcc -O2 -Wall -I"include" "src\config_reader.c" "src\login.c" "src\main.c" "src\network.c" "src\utils.c" "drcom_login.res" -o "drcom_login.exe" -lws2_32 -finput-charset=UTF-8 -fexec-charset=GBK
if errorlevel 1 (
    echo   [警告] 编译失败，请检查 MinGW 环境。跳过图标嵌入。
    goto :shortcut
)
echo   [完成] 已生成带图标的 drcom_login.exe
echo.

:shortcut
REM ---- B. 在桌面创建带图标的快捷方式 ----
echo   正在创建桌面快捷方式...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='SilentlyContinue'; $W=New-Object -ComObject WScript.Shell; $L=$W.CreateShortcut([Environment]::GetFolderPath('Desktop') + [char]92 + '校园网一键登录.lnk'); $L.TargetPath='%EXE%'; $L.WorkingDirectory='%~dp0'; if (Test-Path '%ICON%') { $L.IconLocation='%ICON%' }; $L.Description='Dr.COM 校园网一键登录'; $L.Save()"
if errorlevel 1 (
    echo   [警告] 快捷方式创建失败（可忽略，手动运行 exe 即可）。
) else (
    echo   [完成] 已在桌面创建「校园网一键登录」快捷方式。
)
echo.

:step2
REM ================================================================
REM  步骤 2/3 : 输入账号信息
REM ================================================================
echo [2/3] 请输入你的校园网账号信息
echo.

:ask_user
set "DRCOM_USER="
set /p "DRCOM_USER=  学号 / 账号 (必填): "
if not defined DRCOM_USER (
    echo   [错误] 账号不能为空，请重新输入。
    echo.
    goto :ask_user
)

set "DRCOM_PASS="
set /p "DRCOM_PASS=  密码        (必填): "
if not defined DRCOM_PASS (
    echo   [错误] 密码不能为空，请重新输入。
    echo.
    goto :ask_user
)

set "DRCOM_SUFFIX="
set /p "DRCOM_SUFFIX=  运营商后缀 [unicom/dx/yd] (直接回车默认 unicom): "
if not defined DRCOM_SUFFIX set "DRCOM_SUFFIX=unicom"
echo.

REM ================================================================
REM  步骤 3/3 : 生成配置文件 drcom.conf
REM ================================================================
echo [3/3] 正在生成配置文件 drcom.conf ...
echo.

powershell -NoProfile -ExecutionPolicy Bypass -Command "$nl=[char]13+[char]10; $u=$env:DRCOM_USER; $p=$env:DRCOM_PASS; $s=$env:DRCOM_SUFFIX; $c='; ============================================' + $nl + '; Dr.COM 4.0 校园网一键登录 - 配置文件' + $nl + '; 本文件由 setup_first_login.bat 自动生成' + $nl + '; ============================================' + $nl + ';' + $nl + '; ---------- 账号信息（必填） ----------' + $nl + 'username = ' + $u + $nl + 'password = ' + $p + $nl + 'suffix   = ' + $s + $nl + ';' + $nl + '; ---------- 服务器配置（可选，一般无需修改） ----------' + $nl + 'server        = 10.0.100.3' + $nl + 'port_http     = 80' + $nl + 'port_portal   = 801' + $nl + 'ac_name       = ME60-CSDX' + $nl + 'js_version    = 4.2.1' + $nl + 'terminal_type = 1' + $nl; [IO.File]::WriteAllText('drcom.conf', $c, [Text.Encoding]::GetEncoding(936))"

if not exist "drcom.conf" (
    echo   [错误] drcom.conf 生成失败，请检查当前目录写入权限。
    echo.
    pause
    endlocal
    exit /b 1
)

echo   [完成] 已生成配置文件: %~dp0drcom.conf
echo.

echo ================================================================
echo   配置完成！
echo.
echo     - 配置文件 : %~dp0drcom.conf
echo     - 登录程序 : %~dp0drcom_login.exe
echo     - 桌面快捷方式: 校园网一键登录
echo.
echo   双击 drcom_login.exe 或桌面快捷方式即可登录。
echo   如需修改账号密码，重新运行本脚本即可覆盖生成。
echo.
echo   提示: drcom.conf 含明文密码，请勿分享或提交到仓库。
echo ================================================================
echo.
pause
endlocal
exit /b 0

REM ================================================================
REM  子过程: 检测命令是否存在（存在返回 0，不存在返回 1）
REM ================================================================
:which
where %1 >nul 2>nul
exit /b %errorlevel%
