@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

REM ================================================================
REM   校园网开机自动登录
REM
REM   用法: 为「本文件」创建快捷方式, 放进「启动」文件夹即可。
REM         按 Win+R 输入 shell:startup 可打开该文件夹。
REM
REM   如需调整等待时间, 修改下面 timeout /t 5 的数字(秒)。
REM ================================================================

title 校园网开机自动登录

REM 等待网络就绪
timeout /t 5 /nobreak >nul

if not exist "drcom_login.exe" (
    echo.
    echo [错误] 未找到 drcom_login.exe
    echo         请将本文件与登录程序放在同一目录下。
    echo.
    pause
    exit /b 1
)

REM "< nul" 让程序读到 EOF, 登录结束后不再等待按键
"drcom_login.exe" < nul > "login.log" 2>&1

if errorlevel 1 (
    echo [提示] 本次登录未成功, 详情见 login.log
) else (
    echo [完成] 登录成功
)
timeout /t 3 /nobreak >nul

endlocal
exit /b 0