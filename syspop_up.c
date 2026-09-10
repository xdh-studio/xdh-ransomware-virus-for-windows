// lock_pic.c
// 全屏图片，点击弹随机码，然后绘制输入框（明文密码，无闪烁）
// 密码规则：2011 - 随机码（数值相减）
// 错误等待20秒
// 编译：gcc -mwindows lock_pic.c -o lock_pic.exe -lgdiplus -lgdi32 -luser32 -lole32 -loleaut32

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PROPID
#define PROPID long
#endif

#include <propidl.h>
#include <gdiplus.h>

// 全局变量
HWND g_hMainWnd;
char g_randomCode[16] = {0};
char g_correctPassword[256] = {0};
GpBitmap* g_pBitmap = NULL;
GdiplusStartupInput g_gdiplusStartupInput;
ULONG_PTR g_gdiplusToken;

// 输入框状态
int g_showInput = 0;
char g_inputText[256] = {0};
int g_inputLen = 0;

// 控件矩形
RECT g_inputRect, g_editRect, g_okRect, g_cancelRect;

// 错误等待状态
int g_isWaiting = 0;
int g_waitSeconds = 0;
UINT_PTR g_waitTimer = 0;

// ---------- 密码生成（2011 - 随机码） ----------
void compute_password(const char* randomCode, char* output) {
    int code = atoi(randomCode);
    int result = 2011 - code;
    sprintf(output, "%d", result);
}

// ---------- 加载 JPG ----------
int LoadJPG(const char* filename) {
    WCHAR wpath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wpath, MAX_PATH);
    GpStatus status = GdipCreateBitmapFromFile(wpath, &g_pBitmap);
    return (status == Ok);
}

// ---------- 初始化控件矩形 ----------
void InitRects(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right, h = rc.bottom;

    int boxW = 320, boxH = 150;
    int boxX = (w - boxW) / 2;
    int boxY = (h - boxH) / 2;

    g_inputRect = (RECT){boxX, boxY, boxX + boxW, boxY + boxH};
    g_editRect = (RECT){boxX + 20, boxY + 40, boxX + boxW - 20, boxY + 40 + 28};
    g_okRect = (RECT){boxX + 40, boxY + 80, boxX + 100, boxY + 110};
    g_cancelRect = (RECT){boxX + 180, boxY + 80, boxX + 240, boxY + 110};
}

// ---------- 绘制输入框 ----------
void DrawInputBox(HDC hdc) {
    HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &g_inputRect, hBrush);
    DeleteObject(hBrush);
    FrameRect(hdc, &g_inputRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "微软雅黑");
    HFONT oldFont = SelectObject(hdc, hFont);
    RECT textRect = g_inputRect;
    textRect.top += 8;
    textRect.bottom = textRect.top + 30;
    DrawTextA(hdc, "请输入密码：", -1, &textRect, DT_LEFT | DT_SINGLELINE);

    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &g_editRect, hWhite);
    DeleteObject(hWhite);
    FrameRect(hdc, &g_editRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SetTextColor(hdc, RGB(0, 0, 0));
    RECT drawRect = g_editRect;
    drawRect.left += 5;
    DrawTextA(hdc, g_inputText, -1, &drawRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    HBRUSH hBtn;
    if (g_isWaiting) {
        hBtn = CreateSolidBrush(RGB(220, 220, 220));
    } else {
        hBtn = CreateSolidBrush(RGB(200, 200, 200));
    }
    FillRect(hdc, &g_okRect, hBtn);
    DeleteObject(hBtn);
    FrameRect(hdc, &g_okRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc, g_isWaiting ? RGB(128,128,128) : RGB(0,0,0));
    DrawTextA(hdc, "确定", -1, &g_okRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    hBtn = CreateSolidBrush(RGB(200, 200, 200));
    FillRect(hdc, &g_cancelRect, hBtn);
    DeleteObject(hBtn);
    FrameRect(hdc, &g_cancelRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc, RGB(0, 0, 0));
    DrawTextA(hdc, "取消", -1, &g_cancelRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    if (g_isWaiting) {
        char waitMsg[64];
        snprintf(waitMsg, sizeof(waitMsg), "密码错误，请等待 %d 秒后再试", g_waitSeconds);
        SetTextColor(hdc, RGB(255, 0, 0));
        RECT waitRect = g_inputRect;
        waitRect.top = g_inputRect.bottom - 25;
        waitRect.bottom = g_inputRect.bottom - 5;
        DrawTextA(hdc, waitMsg, -1, &waitRect, DT_CENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

// ---------- 处理鼠标点击 ----------
void HandleClick(HWND hWnd, int x, int y) {
    if (!g_showInput) {
        char msg[128];
        snprintf(msg, sizeof(msg), "随机码：%s", g_randomCode);
        MessageBoxA(hWnd, msg, "提示", MB_OK);
        g_showInput = 1;
        g_inputText[0] = '\0';
        g_inputLen = 0;
        g_isWaiting = 0;
        g_waitSeconds = 0;
        if (g_waitTimer) {
            KillTimer(hWnd, g_waitTimer);
            g_waitTimer = 0;
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return;
    }

    if (g_isWaiting) return;

    if (x >= g_okRect.left && x <= g_okRect.right && y >= g_okRect.top && y <= g_okRect.bottom) {
        if (strcmp(g_inputText, g_correctPassword) == 0) {
            char cmd[MAX_PATH + 64];
            snprintf(cmd, sizeof(cmd), "C:\\syslock_app\\unlockrun.exe \"%s\"", g_correctPassword);
            STARTUPINFOA si = {sizeof(si)};
            PROCESS_INFORMATION pi;
            if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            PostQuitMessage(0);
        } else {
            g_isWaiting = 1;
            g_waitSeconds = 20;
            g_waitTimer = SetTimer(hWnd, 2, 1000, NULL);
            g_inputText[0] = '\0';
            g_inputLen = 0;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        return;
    }

    if (x >= g_cancelRect.left && x <= g_cancelRect.right && y >= g_cancelRect.top && y <= g_cancelRect.bottom) {
        g_showInput = 0;
        g_inputText[0] = '\0';
        g_inputLen = 0;
        g_isWaiting = 0;
        g_waitSeconds = 0;
        if (g_waitTimer) {
            KillTimer(hWnd, g_waitTimer);
            g_waitTimer = 0;
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return;
    }
}

// ---------- 处理键盘输入 ----------
void HandleKey(HWND hWnd, WPARAM wParam) {
    if (!g_showInput || g_isWaiting) return;

    if (wParam >= 0x20 && wParam <= 0x7E && g_inputLen < sizeof(g_inputText) - 1) {
        g_inputText[g_inputLen++] = (char)wParam;
        g_inputText[g_inputLen] = '\0';
        InvalidateRect(hWnd, NULL, TRUE);
    } else if (wParam == VK_BACK && g_inputLen > 0) {
        g_inputText[--g_inputLen] = '\0';
        InvalidateRect(hWnd, NULL, TRUE);
    } else if (wParam == VK_RETURN) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        HandleClick(hWnd, pt.x, pt.y);
    }
}

// ---------- 主窗口过程 ----------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            InitRects(hWnd);
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            if (g_pBitmap) {
                GpGraphics* graphics = NULL;
                GdipCreateFromHDC(hdc, &graphics);
                if (graphics) {
                    GdipDrawImageRectI(graphics, g_pBitmap, 0, 0, rc.right, rc.bottom);
                    GdipDeleteGraphics(graphics);
                }
            } else {
                HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
                FillRect(hdc, &rc, hBrush);
                DeleteObject(hBrush);
            }

            if (g_showInput) {
                DrawInputBox(hdc);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
            HandleClick(hWnd, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_KEYDOWN:
            HandleKey(hWnd, wParam);
            return 0;

        case WM_TIMER:
            if (wParam == 2) {
                g_waitSeconds--;
                if (g_waitSeconds <= 0) {
                    g_isWaiting = 0;
                    g_waitSeconds = 0;
                    KillTimer(hWnd, g_waitTimer);
                    g_waitTimer = 0;
                    g_inputText[0] = '\0';
                    g_inputLen = 0;
                    InvalidateRect(hWnd, NULL, TRUE);
                } else {
                    InvalidateRect(hWnd, &g_inputRect, TRUE);
                }
            }
            break;

        case WM_CLOSE:
            return 0;

        case WM_DESTROY:
            if (g_waitTimer) KillTimer(hWnd, g_waitTimer);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------- 主函数 ----------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_gdiplusStartupInput.GdiplusVersion = 1;
    GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);

    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    if (CreateProcessA("C:\\syslock_app\\syspass.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    FILE* fp = fopen("C:\\syslock_app\\passw.txt", "r");
    if (fp) {
        fgets(g_randomCode, sizeof(g_randomCode), fp);
        char* p = strchr(g_randomCode, '\n');
        if (p) *p = '\0';
        p = strchr(g_randomCode, '\r');
        if (p) *p = '\0';
        fclose(fp);
    } else {
        strcpy(g_randomCode, "000");
    }

    compute_password(g_randomCode, g_correctPassword);

    const char* imgPaths[] = {
        "C:\\syslock_app\\lock_bg.jpg",
        "lock_bg.jpg",
        "Screenshot_20260823_134329_com.lemon.lv_edit_96967177183164.jpg"
    };
    for (int i = 0; i < 3; i++) {
        if (LoadJPG(imgPaths[i])) break;
    }

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "LockPicClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClassA(&wc)) {
        GdiplusShutdown(g_gdiplusToken);
        return 0;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    g_hMainWnd = CreateWindowExA(WS_EX_TOPMOST, "LockPicClass", "锁定",
                                  WS_POPUP, 0, 0, screenW, screenH,
                                  NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd) {
        GdiplusShutdown(g_gdiplusToken);
        return 0;
    }

    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_pBitmap) GdipDisposeImage(g_pBitmap);
    GdiplusShutdown(g_gdiplusToken);
    return 0;
}