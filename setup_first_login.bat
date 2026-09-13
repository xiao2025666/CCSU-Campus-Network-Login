@echo off
chcp 65001 >nul
setlocal
title Dr.COM 校园网一键登录 - 首次配置向导
cd /d "%~dp0"

set "ICON=%~dp0drcom_login.ico"
set "EXE=%~dp0drcom_login.exe"
set "TOOLCHAIN=%~dp0mingw64-mini-非完整编译器"
set "ZIP=%~dp0mingw64-mini.zip"
set "RELEASE_URL=https://github.com/xiao2025666/CCSU-Campus-Network-Login/releases/latest/download/mingw64-mini.zip"

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
REM     编译器查找顺序: 内置精简工具链 -> 系统 PATH 中的 gcc -> 从 Releases 下载
REM ================================================================
echo [1/3] 设置登录程序图标...
echo.

if not exist "%ICON%" (
    echo   [警告] 未找到图标文件 drcom_login.ico，跳过图标设置。
    goto :step2
)

REM 已提供编译好的 exe（免编译发行版）时直接跳过编译，无需任何编译环境。
REM 需要强制重新编译时运行: setup_first_login.bat --rebuild
if /i not "%~1"=="--rebuild" if exist "%EXE%" (
    echo   [跳过] 已存在编译好的 drcom_login.exe，无需编译。
    goto :shortcut
)

if not exist "%TOOLCHAIN%\bin\gcc.exe" goto :detect_system
set "GCC_EXE=%TOOLCHAIN%\bin\gcc.exe"
set "WINDRES_EXE=%TOOLCHAIN%\bin\windres.exe"
echo   [信息] 使用内置精简工具链: mingw64-mini-非完整编译器
goto :compile_icon

:detect_system
call :which gcc
if errorlevel 1 goto :ask_toolchain_download
call :which windres
if errorlevel 1 (
    echo   [跳过] 系统 gcc 可用但缺少 windres，无法把图标嵌入 exe。
    goto :shortcut
)
set "GCC_EXE=gcc"
set "WINDRES_EXE=windres"
echo   [信息] 使用系统中已安装的 gcc
goto :compile_icon

:ask_toolchain_download
echo   [提示] 未检测到编译器 gcc / windres。
echo          嵌入图标需要编译器；也可跳过，仅设置桌面快捷方式图标。
echo.
echo   是否从 GitHub Releases 下载精简工具链 (约 26 MB)? [Y/n，直接回车 = 下载]
powershell -NoProfile -ExecutionPolicy Bypass -Command "$d=Read-Host '  请输入'; if ($d -match '^\s*[Nn]') { exit 1 } else { exit 0 }"
if errorlevel 1 (
    echo   [跳过] 不下载，仅设置快捷方式图标。
    goto :shortcut
)
echo   正在下载精简工具链（仅需一次）...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue'; try { Invoke-WebRequest -UseBasicParsing -Uri '%RELEASE_URL%' -OutFile '%ZIP%'; Write-Host '   [完成] 下载完成' } catch { Write-Host ('   [错误] 下载失败: ' + $_.Exception.Message) }"
if not exist "%ZIP%" (
    echo   [跳过] 下载失败，仅设置快捷方式图标。
    goto :shortcut
)
echo   正在解压...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; try { Expand-Archive -Path '%ZIP%' -DestinationPath '%~dp0' -Force; Write-Host '   [完成] 解压完成' } catch { Write-Host ('   [错误] 解压失败: ' + $_.Exception.Message) }"
if not exist "%TOOLCHAIN%\bin\gcc.exe" (
    echo   [警告] 未找到编译器，请手动把工具链解压到:
    echo          %TOOLCHAIN%
    goto :shortcut
)
set "GCC_EXE=%TOOLCHAIN%\bin\gcc.exe"
set "WINDRES_EXE=%TOOLCHAIN%\bin\windres.exe"

:compile_icon
REM gcc 通过 PATH 查找 as.exe / ld.exe，必须把工具链 bin 加进去（内置工具链尤其需要）
if exist "%TOOLCHAIN%\bin" set "PATH=%TOOLCHAIN%\bin;%PATH%"

echo   正在编译图标资源...
if not exist "drcom_login.rc" > "drcom_login.rc" echo IDI_ICON1 ICON "drcom_login.ico"
"%WINDRES_EXE%" "drcom_login.rc" -O coff -o "drcom_login.res"
if errorlevel 1 (
    echo   [警告] 资源编译失败，跳过图标嵌入。
    goto :shortcut
)

echo   正在重新编译程序（含图标）...
"%GCC_EXE%" -O2 -Wall -I"include" "src\config_reader.c" "src\login.c" "src\main.c" "src\network.c" "src\utils.c" "drcom_login.res" -o "drcom_login.exe" -lws2_32 -finput-charset=UTF-8 -fexec-charset=GBK
if errorlevel 1 (
    echo   [警告] 编译失败，跳过图标嵌入。
    goto :shortcut
)
echo   [完成] 已生成带图标的 drcom_login.exe
echo.

:shortcut
echo   正在创建桌面快捷方式...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='SilentlyContinue'; $W=New-Object -ComObject WScript.Shell; $L=$W.CreateShortcut([Environment]::GetFolderPath('Desktop') + [char]92 + '校园网一键登录.lnk'); $L.TargetPath='%EXE%'; $L.WorkingDirectory='%~dp0'; if (Test-Path '%ICON%') { $L.IconLocation='%ICON%' }; $L.Description='Dr.COM 校园网一键登录'; $L.Save()"
if errorlevel 1 (
    echo   [警告] 快捷方式创建失败（可忽略）。
) else (
    echo   [完成] 已在桌面创建「校园网一键登录」快捷方式。
)
echo.

:step2
REM ================================================================
REM  步骤 2/3 : 输入账号信息并生成 drcom.conf
REM     用 PowerShell Read-Host 读取，避免 cmd 的 set /p 在切换代码页后失效
REM ================================================================
echo [2/3] 请输入你的校园网账号信息
echo.
echo   （密码输入时不显示，输入完直接回车即可）
echo.

powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $u=Read-Host '  学号 / 账号'; if([string]::IsNullOrWhiteSpace($u)){Write-Host '  [错误] 账号不能为空'; exit 1}; $sec=Read-Host '  密码' -AsSecureString; $p=[Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec)); if([string]::IsNullOrWhiteSpace($p)){Write-Host '  [错误] 密码不能为空'; exit 1}; $s=Read-Host '  运营商后缀 [unicom/dx/yd] (直接回车 = unicom)'; if([string]::IsNullOrWhiteSpace($s)){$s='unicom'}; $nl=[char]13+[char]10; $c='; ============================================' + $nl + '; Dr.COM 4.0 校园网一键登录 - 配置文件' + $nl + '; 本文件由 setup_first_login.bat 自动生成' + $nl + '; 警告: 含明文密码, 请勿分享或提交到仓库' + $nl + '; ============================================' + $nl + ';' + $nl + '; ---------- 账号信息（必填） ----------' + $nl + 'username = ' + $u + $nl + 'password = ' + $p + $nl + 'suffix   = ' + $s + $nl + ';' + $nl + '; ---------- 服务器配置（可选，一般无需修改） ----------' + $nl + 'server        = 10.0.100.3' + $nl + 'port_http     = 80' + $nl + 'port_portal   = 801' + $nl + 'ac_name       = ME60-CSDX' + $nl + 'js_version    = 4.2.1' + $nl + 'terminal_type = 1' + $nl; [IO.File]::WriteAllText((Join-Path (Get-Location) 'drcom.conf'), $c, [Text.UTF8Encoding]::new($false))"
if errorlevel 1 goto :conf_failed

echo [3/3] 配置完成
echo.
echo ================================================================
echo   全部完成！
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

:conf_failed
echo.
echo   [错误] drcom.conf 生成失败（可能取消了输入或输入为空）。
echo.
pause
endlocal
exit /b 1

:which
where %1 >nul 2>nul
exit /b %errorlevel%
