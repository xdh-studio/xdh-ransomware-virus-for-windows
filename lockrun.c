// syslock_controller.c
// 编译：gcc syslock_controller.c -o syslock_controller.exe
// 运行后自动执行上述逻辑，无需参数

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// 执行 exe，并返回其退出码（若失败返回 -1）
int RunExeAndGetExitCode(const char* exePath, const char* workingDir) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 创建进程
    if (!CreateProcessA(exePath, NULL, NULL, NULL, FALSE, 0, NULL,
                        workingDir ? workingDir : NULL, &si, &pi)) {
        printf("运行失败: %s (错误码: %d)\n", exePath, GetLastError());
        return -1;
    }

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 获取退出码
    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        printf("获取退出码失败: %s\n", exePath);
        exitCode = -1;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}

// 获取当前可执行文件所在目录（末尾带反斜杠）
void GetExeDirectory(char* buffer, size_t size) {
    GetModuleFileNameA(NULL, buffer, size);
    char* lastSlash = strrchr(buffer, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';  // 截断到最后一个反斜杠
    } else {
        buffer[0] = '\0';
    }
}

int main() {
    const char* sysenv = "C:/syslock_app/sysenvirmenmon.exe";
    const char* syscoun = "C:/syslock_app/syslockcoun.exe";
    const char* syspop  = "C:/syslock_app/syspop_up.exe";

    // 1. 运行 sysenvirmenmon.exe（不关心结果）
    printf("执行: %s\n", sysenv);
    RunExeAndGetExitCode(sysenv, NULL);

    // 2. 运行 syslockcoun.exe 并获取返回码
    printf("执行: %s\n", syscoun);
    int result = RunExeAndGetExitCode(syscoun, NULL);
    if (result == -1) {
        printf("syslockcoun.exe 运行失败，终止。\n");
        return 1;
    }

    // 3. 根据返回值决定后续
    char exeDir[MAX_PATH];
    GetExeDirectory(exeDir, sizeof(exeDir));

    if (result == 0) {   // 假设返回 0 代表 "lock"，可根据实际调整
        printf("返回码 %d 为 lock，执行 syspop_up.exe\n", result);
        RunExeAndGetExitCode(syspop, NULL);
    } else if (result == 1) {  // 假设返回 1 代表 "unlock"
        char lockAppPath[MAX_PATH];
        snprintf(lockAppPath, sizeof(lockAppPath), "%slock_app.exe", exeDir);
        printf("返回码 %d 为 unlock，执行 %s\n", result, lockAppPath);
        RunExeAndGetExitCode(lockAppPath, exeDir);
    } else {
        printf("未知返回码 %d，不做任何操作。\n", result);
    }

    printf("程序结束。\n");
    return 0;
}