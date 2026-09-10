#define _UNICODE
#define UNICODE
#define _CRT_SECURE_NO_WARNINGS   // 避免 wcscat 等警告

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <objbase.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#define SYS_LOCK_DIR L"C:\\syslock_app"
#define LOCKRUN_EXE  L"C:\\syslock_app\\lockrun_m.exe"
#define HIDE_EXE     L"C:\\syslock_app\\HideDirectory.exe"
#define RCEDIT_EXE   L"C:\\syslock_app\\rcedit.exe"
#define MAX_PATH_LEN 32768

// 存储已处理目录的链表
typedef struct DirNode {
    WCHAR path[MAX_PATH];
    struct DirNode *next;
} DirNode;

DirNode *processedDirs = NULL;

BOOL IsDirProcessed(const WCHAR *dir) {
    DirNode *p = processedDirs;
    while (p) {
        if (lstrcmpiW(p->path, dir) == 0)
            return TRUE;
        p = p->next;
    }
    return FALSE;
}

void AddProcessedDir(const WCHAR *dir) {
    DirNode *node = (DirNode*)malloc(sizeof(DirNode));
    if (!node) return;
    lstrcpyW(node->path, dir);
    node->next = processedDirs;
    processedDirs = node;
}

// 从注册表枚举安装目录
void EnumInstallDirs(WCHAR *dirs[], int *count, int maxCount) {
    HKEY hKey;
    DWORD index = 0;
    WCHAR subKeyName[1024];
    DWORD size = sizeof(subKeyName);
    WCHAR installPath[MAX_PATH];
    DWORD pathSize;

    // HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        while (RegEnumKeyExW(hKey, index, subKeyName, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY hSub;
            if (RegOpenKeyExW(hKey, subKeyName, 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
                pathSize = sizeof(installPath);
                if (RegQueryValueExW(hSub, L"InstallLocation", NULL, NULL, (LPBYTE)installPath, &pathSize) == ERROR_SUCCESS) {
                    if (lstrlenW(installPath) > 0 && PathFileExistsW(installPath)) {
                        PathRemoveBackslashW(installPath);
                        if (!IsDirProcessed(installPath) && *count < maxCount) {
                            dirs[*count] = wcsdup(installPath);   // 使用 wcsdup 替代 _wcsdup
                            if (dirs[*count]) {
                                AddProcessedDir(installPath);
                                (*count)++;
                            }
                        }
                    }
                }
                RegCloseKey(hSub);
            }
            index++;
            size = sizeof(subKeyName);
        }
        RegCloseKey(hKey);
    }

    // HKLM\Software\WOW6432Node\... (32-bit apps)
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        index = 0;
        while (RegEnumKeyExW(hKey, index, subKeyName, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY hSub;
            if (RegOpenKeyExW(hKey, subKeyName, 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
                pathSize = sizeof(installPath);
                if (RegQueryValueExW(hSub, L"InstallLocation", NULL, NULL, (LPBYTE)installPath, &pathSize) == ERROR_SUCCESS) {
                    if (lstrlenW(installPath) > 0 && PathFileExistsW(installPath)) {
                        PathRemoveBackslashW(installPath);
                        if (!IsDirProcessed(installPath) && *count < maxCount) {
                            dirs[*count] = wcsdup(installPath);
                            if (dirs[*count]) {
                                AddProcessedDir(installPath);
                                (*count)++;
                            }
                        }
                    }
                }
                RegCloseKey(hSub);
            }
            index++;
            size = sizeof(subKeyName);
        }
        RegCloseKey(hKey);
    }

    // HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        index = 0;
        while (RegEnumKeyExW(hKey, index, subKeyName, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY hSub;
            if (RegOpenKeyExW(hKey, subKeyName, 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
                pathSize = sizeof(installPath);
                if (RegQueryValueExW(hSub, L"InstallLocation", NULL, NULL, (LPBYTE)installPath, &pathSize) == ERROR_SUCCESS) {
                    if (lstrlenW(installPath) > 0 && PathFileExistsW(installPath)) {
                        PathRemoveBackslashW(installPath);
                        if (!IsDirProcessed(installPath) && *count < maxCount) {
                            dirs[*count] = wcsdup(installPath);
                            if (dirs[*count]) {
                                AddProcessedDir(installPath);
                                (*count)++;
                            }
                        }
                    }
                }
                RegCloseKey(hSub);
            }
            index++;
            size = sizeof(subKeyName);
        }
        RegCloseKey(hKey);
    }
}

// 查找目录中最大的 .exe 文件
WCHAR* FindLargestExe(const WCHAR *dir) {
    WCHAR searchPath[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE hFind;
    LARGE_INTEGER maxSize = {0};
    WCHAR *largestFile = NULL;

    wsprintfW(searchPath, L"%s\\*.exe", dir);
    hFind = FindFirstFileW(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return NULL;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            LARGE_INTEGER size;
            size.HighPart = fd.nFileSizeHigh;
            size.LowPart = fd.nFileSizeLow;
            if (size.QuadPart > maxSize.QuadPart) {
                maxSize = size;
                if (largestFile) free(largestFile);
                largestFile = wcsdup(fd.cFileName);
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
    return largestFile;
}

// 使用 rcedit 提取图标并设置到新 exe
BOOL ApplyIcon(const WCHAR *originalExe, const WCHAR *newExe) {
    if (!PathFileExistsW(RCEDIT_EXE)) {   // 修正拼写
        wprintf(L"Warning: rcedit.exe not found, skip icon operations.\n");
        return FALSE;
    }

    WCHAR tempIco[MAX_PATH];
    GetTempPathW(MAX_PATH, tempIco);
    wcscat(tempIco, L"temp_icon_");        // 使用 wcscat 替代 wcscat_s
    GUID guid;
    CoCreateGuid(&guid);
    WCHAR guidStr[64];
    StringFromGUID2(&guid, guidStr, 64);
    wcscat(tempIco, guidStr);
    wcscat(tempIco, L".ico");

    // Extract icon
    WCHAR cmdLine[2048];
    wsprintfW(cmdLine, L"\"%s\" \"%s\" --extract-icon \"%s\"", RCEDIT_EXE, originalExe, tempIco);
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        wprintf(L"Failed to extract icon, error: %d\n", GetLastError());
        return FALSE;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0 || !PathFileExistsW(tempIco)) {
        wprintf(L"Extract icon failed, exit code: %d\n", exitCode);
        DeleteFileW(tempIco);
        return FALSE;
    }

    // Set icon
    wsprintfW(cmdLine, L"\"%s\" \"%s\" --set-icon \"%s\"", RCEDIT_EXE, newExe, tempIco);
    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        wprintf(L"Failed to set icon, error: %d\n", GetLastError());
        DeleteFileW(tempIco);
        return FALSE;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    DeleteFileW(tempIco);
    return (exitCode == 0);
}

// 处理单个安装目录（增加 C 盘 Windows 目录过滤）
// 处理单个安装目录（增加 C 盘 Windows 目录过滤，使用 Windows API 规避编译错误）
void ProcessDirectory(const WCHAR *dir) {
    wprintf(L"\nProcessing directory: %s\n", dir);

    // ====== 安全过滤：如果路径在 C 盘且包含 \Windows\，则跳过 ======
    // 判断是否为 C 盘（不区分大小写）
    if (wcslen(dir) >= 3 && (dir[0] == L'C' || dir[0] == L'c') && dir[1] == L':' && dir[2] == L'\\') {
        // 检查路径中是否包含 \Windows\（不区分大小写，使用 StrStrIW）
        if (StrStrIW(dir, L"\\Windows\\") != NULL) {
            wprintf(L"  Skipped (contains \\Windows\\ in path).\n");
            return;
        }
        // 精确匹配 C:\Windows
        if (lstrcmpiW(dir, L"C:\\Windows") == 0) {
            wprintf(L"  Skipped (exact C:\\Windows).\n");
            return;
        }
    }

    // ====== 防重复检查：如果已存在 lock_app.exe，跳过 ======
    WCHAR lockAppPath[MAX_PATH];
    wsprintfW(lockAppPath, L"%s\\lock_app.exe", dir);
    if (PathFileExistsW(lockAppPath)) {
        wprintf(L"  Directory already processed (lock_app.exe exists), skipped.\n");
        return;
    }

    // ----- 以下为原有逻辑（重命名、复制、图标） -----
    WCHAR *mainExeName = FindLargestExe(dir);
    if (!mainExeName) {
        wprintf(L"  No .exe found, skipped.\n");
        return;
    }

    WCHAR originalFullPath[MAX_PATH];
    wsprintfW(originalFullPath, L"%s\\%s", dir, mainExeName);
    wprintf(L"  Main executable: %s\n", mainExeName);

    // 1. Rename main exe to lock_app.exe（若已存在则先备份）
    wsprintfW(lockAppPath, L"%s\\lock_app.exe", dir);
    if (PathFileExistsW(lockAppPath)) {
        WCHAR bakPath[MAX_PATH];
        wsprintfW(bakPath, L"%s\\lock_app.exe.bak", dir);
        DeleteFileW(bakPath);
        MoveFileW(lockAppPath, bakPath);
    }
    if (!MoveFileW(originalFullPath, lockAppPath)) {
        wprintf(L"  Rename failed, error: %d\n", GetLastError());
        free(mainExeName);
        return;
    }
    wprintf(L"  Renamed to lock_app.exe\n");

    // 2. Copy lockrun_m.exe and rename to original name
    WCHAR newExePath[MAX_PATH];
    wsprintfW(newExePath, L"%s\\%s", dir, mainExeName);
    if (!CopyFileW(LOCKRUN_EXE, newExePath, FALSE)) {
        wprintf(L"  Copy lockrun_m.exe failed, error: %d\n", GetLastError());
        MoveFileW(lockAppPath, originalFullPath);
        free(mainExeName);
        return;
    }
    wprintf(L"  Copied and renamed to %s\n", mainExeName);

    // 3. Icon handling
    if (PathFileExistsW(RCEDIT_EXE)) {
        if (ApplyIcon(lockAppPath, newExePath)) {
            wprintf(L"  Icon applied.\n");
        } else {
            wprintf(L"  Icon application failed, continuing.\n");
        }
    } else {
        wprintf(L"  Skip icon (rcedit.exe not found).\n");
    }

    free(mainExeName);
}

int main() {
    // Check administrator
    BOOL isElevated = FALSE;
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elev;
        DWORD size = sizeof(elev);
        if (GetTokenInformation(hToken, TokenElevation, &elev, size, &size))
            isElevated = elev.TokenIsElevated;
        CloseHandle(hToken);
    }
    if (!isElevated) {
        wprintf(L"Error: Please run as Administrator.\n");
        return 1;
    }

    // Check required files
    if (!PathFileExistsW(LOCKRUN_EXE)) {
        wprintf(L"Error: %s not found.\n", LOCKRUN_EXE);
        return 1;
    }
    if (!PathFileExistsW(HIDE_EXE)) {
        wprintf(L"Warning: %s not found, HideDirectory.exe will not be started.\n", HIDE_EXE);
    }

    // Enumerate installation directories
    WCHAR **dirs = (WCHAR**)malloc(4096 * sizeof(WCHAR*));
    int count = 0;
    EnumInstallDirs(dirs, &count, 4096);
    wprintf(L"Found %d installation directories.\n", count);

    // Process each
    for (int i = 0; i < count; i++) {
        ProcessDirectory(dirs[i]);
        free(dirs[i]);
    }
    free(dirs);

    // Free linked list
    while (processedDirs) {
        DirNode *next = processedDirs->next;
        free(processedDirs);
        processedDirs = next;
    }

    // Finally run HideDirectory.exe
    if (PathFileExistsW(HIDE_EXE)) {
        wprintf(L"\nRunning HideDirectory.exe ...\n");
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcessW(HIDE_EXE, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            wprintf(L"Failed to run HideDirectory.exe, error: %d\n", GetLastError());
        }
    }

    wprintf(L"\nAll operations completed.\n");
    return 0;
}