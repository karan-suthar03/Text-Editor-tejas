#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../include/editor.h"

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const char* window_name = "Text-Editor";
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = window_name;
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, window_name,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                1000, 800,
                                NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);

    Editor editor = {};
    editor_init(&editor, hwnd);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&editor);

    while (editor.running) {
        MSG msg = {};
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            editor.running = FALSE;
        }
    }

    editor_free(&editor);

    return 0;
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Editor* editor = (Editor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (!editor && msg != WM_CREATE) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_CHAR:
            editor_handle_char(editor, wParam);
            break;

        case WM_KEYDOWN:
            editor_handle_keydown(editor, wParam);
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            editor_paint(editor, hdc);
            EndPaint(hwnd, &ps);
        } break;

        case WM_DESTROY:
            editor->running = FALSE;
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}