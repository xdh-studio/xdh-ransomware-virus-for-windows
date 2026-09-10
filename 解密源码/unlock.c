#include <windows.h>
#include <stdio.h>
#include <string.h>

#define ID_EDIT_INPUT    1001
#define ID_BTN_CALC      1002
#define ID_STATIC_RESULT 1003
#define ID_BTN_DECRYPT   1004

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char buf_in[16];
    char buf_out[16];
    int in_val, out_val;
    int len;
    SHELLEXECUTEINFOA sei;

    switch (msg)
    {
    case WM_CREATE:
        CreateWindowExA(0, "STATIC", "随机码：",
            WS_CHILD | WS_VISIBLE, 20, 20, 100, 20,
            hWnd, (HMENU)-1, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);

        CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            120, 12, 80, 24,
            hWnd, (HMENU)ID_EDIT_INPUT, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);

        CreateWindowExA(0, "BUTTON", "计算",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            210, 10, 70, 28,
            hWnd, (HMENU)ID_BTN_CALC, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);

        CreateWindowExA(0, "STATIC", "请先输入3位随机码!!!",
            WS_CHILD | WS_VISIBLE, 20, 50, 260, 22,
            hWnd, (HMENU)ID_STATIC_RESULT, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);

        CreateWindowExA(0, "BUTTON", "直接尝试解密",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 80, 260, 32,
            hWnd, (HMENU)ID_BTN_DECRYPT, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_CALC)
        {
            GetWindowTextA(GetDlgItem(hWnd, ID_EDIT_INPUT), buf_in, sizeof(buf_in));
            len = (int)strlen(buf_in);

            // 校验：必须恰好3位数字
            if(len != 3)
            {
                SetDlgItemTextA(hWnd, ID_STATIC_RESULT, "您输入的内容有误，请重试！");
                break;
            }

            in_val = atoi(buf_in);
            out_val = 2011 - in_val;
            sprintf(buf_out, "解密密码是：%04d", out_val);
            SetDlgItemTextA(hWnd, ID_STATIC_RESULT, buf_out);
        }
        else if (LOWORD(wParam) == ID_BTN_DECRYPT)
        {
            ZeroMemory(&sei, sizeof(sei));
            sei.cbSize = sizeof(SHELLEXECUTEINFOA);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.hwnd = hWnd;
            sei.lpVerb = "open";
            sei.lpFile = "unlockrun.exe";
            sei.lpParameters = NULL;
            sei.lpDirectory = ".";
            sei.nShow = SW_SHOWNORMAL;
            ShellExecuteExA(&sei);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXA wc = {0};
    HWND hWnd;
    MSG msg;

    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "DecryptToolClass";
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);

    RegisterClassExA(&wc);

    hWnd = CreateWindowExA(
        0,
        "DecryptToolClass",
        "解密工具",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        310, 160,
        NULL, NULL,
        hInstance,
        NULL
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
