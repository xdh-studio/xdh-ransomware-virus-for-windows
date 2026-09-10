// syslockcoun.c
// 逻辑：若 coun.txt 不存在或内容 <= 1 → 输出 lock，返回 0，不修改
//       若 coun.txt 存在且内容 > 1  → 输出 unlock，返回 1，并将计数减 1
// 编译：gcc syslockcoun.c -o syslockcoun.exe

#include <stdio.h>
#include <stdlib.h>

int main() {
    const char* path = "C:\\syslock_app\\coun.txt";
    FILE* fp = fopen(path, "r+");
    int count;

    if (fp == NULL) {
        // 文件不存在 → lock
        printf("lock\n");
        return 0;
    }

    // 读取当前计数
    if (fscanf(fp, "%d", &count) != 1) {
        // 内容无效 → lock，不修改
        fclose(fp);
        printf("lock\n");
        return 0;
    }

    if (count > 1) {
        // 满足条件 → unlock 并减 1
        count--;
        fseek(fp, 0, SEEK_SET);
        fprintf(fp, "%d", count);
        fclose(fp);
        printf("unlock\n");
        return 1;
    } else {
        // count <= 1 → lock，不修改
        fclose(fp);
        printf("lock\n");
        return 0;
    }
}