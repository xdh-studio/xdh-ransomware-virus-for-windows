// yz.c
// 全局搜索固定驱动器上的 lock_app.exe 并隐藏，同时隐藏 C:\syslock_app
// 编译（Cygwin/MinGW）：gcc yz.c -o yz.exe
// 需要以管理员身份运行

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include <string.h>

// 设置文件或目录为隐藏 + 系统属性
void HidePath(const char* path) {
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        printf("无法获取属性: %s (错误 %d)\n", path, GetLastError());
        return;
    }
    DWORD newAttrs = attrs | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    if (SetFileAttributesA(path, newAttrs)) {
        printf("已隐藏: %s\n", path);
    } else {
        printf("设置属性失败: %s (错误 %d)\n", path, GetLastError());
    }
}

// 递归搜索指定目录下的所有 lock_app.exe 文件，并调用 HidePath
void SearchAndHideInDirectory(const char* rootDir) {
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", rootDir);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        // 跳过 . 和 ..
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;

        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", rootDir, findData.cFileName);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 递归进入子目录
            SearchAndHideInDirectory(fullPath);
        } else {
            // 使用 lstrcmpiA 不区分大小写比较文件名
            if (lstrcmpiA(findData.cFileName, "lock_app.exe") == 0) {
                HidePath(fullPath);
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

// 枚举所有逻辑驱动器，对固定驱动器启动搜索
void SearchAllFixedDrives() {
    DWORD drives = GetLogicalDrives();
    char drivePath[4] = "C:\\";
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            drivePath[0] = 'A' + i;
            UINT type = GetDriveTypeA(drivePath);
            if (type == DRIVE_FIXED) {
                printf("正在搜索驱动器: %s\n", drivePath);
                SearchAndHideInDirectory(drivePath);
            }
        }
    }
}

int main() {
    printf("开始全局搜索 lock_app.exe 并隐藏...\n");
    SearchAllFixedDrives();

    // 隐藏 C:\syslock_app 文件夹（如果存在）
    const char* syslockDir = "C:\\syslock_app";
    if (GetFileAttributesA(syslockDir) != INVALID_FILE_ATTRIBUTES) {
        HidePath(syslockDir);
    } else {
        printf("目录 %s 不存在，跳过。\n", syslockDir);
    }

    printf("操作完成。\n");
    return 0;
}