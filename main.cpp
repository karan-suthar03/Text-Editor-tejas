
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "base.h"

#define MAX_LINES 1024
#define GAP_SIZE  1024

struct Window {
    const char* name;
    uint  width;
    uint  height;
    HWND  handle;
    BOOL  running;
};

struct EditorOptions {
    HFONT    font;
    COLORREF font_color;
    HBRUSH   background;
    HPEN     cursor_color;
};

struct Line {
    uint start;
    uint end;
};

struct Lines {
    Line *items;
    uint count;
    uint capacity;
};

struct Data {
    char *chars;
    uint capacity;
    uint count;
};

struct Buffer {
    Data  data;
    Lines lines;
    uint  gap_start;
    uint  gap_end;
    uint  cursor_pos;
    const char* file_name;
};

global Window        G_window;
global EditorOptions G_editor_opt;
global Buffer        G_buffer;

static void printBuffer() {
    for (int i = 0, index = 0; i < G_buffer.data.count; i++, index++) {
        if (index >= G_buffer.gap_start && index < G_buffer.gap_end)
            index = (int)G_buffer.gap_end;
        char ch = G_buffer.data.chars[index];
        if (ch == '\n') {
            printf("\\n");
        }
        else {
            printf("%c", ch);
        }
    }
    printf("\n");
}

static char* loadFileIntoBuffer(const char* file_path, int *file_size) {
    
    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ,
                                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                     NULL);

    DWORD f_size     = GetFileSize(file_handle, NULL);
    DWORD total_size = f_size + GAP_SIZE;
    char* raw        = (char*)GlobalAlloc(GMEM_FIXED, total_size);
    memset(raw, 0, total_size);

    DWORD chars_read = 0;
    ReadFile(file_handle, raw, f_size, &chars_read, NULL);

    CloseHandle(file_handle);

    // normalizing new lines form /r/n to /n
    char* normalized = (char*) GlobalAlloc(GMEM_FIXED, total_size);
    int chars_written = 0;
    for (DWORD i = 0; i < chars_read; i++) {

        if (raw[i] == '\r') {
            if (i + 1 < chars_read && raw[i + 1] == '\n') {
                normalized[chars_written++] = '\n';
                i++;
            } else {
                normalized[chars_written++] = '\n';
            }
        } else {
            normalized[chars_written++] = raw[i];
        }
    }

    if (file_size) *file_size = chars_written;

    GlobalFree(raw);
    return normalized;
}

static void saveFile() {
    
    HANDLE file_handle = CreateFileA(G_buffer.file_name, GENERIC_WRITE, 0,
                                     NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                     NULL);


    uint size = G_buffer.data.count + 1;
    char* buffer = (char*) GlobalAlloc(GMEM_FIXED, sizeof(char) * size * 2);
    int index = 0, i = 0;

    while (index < G_buffer.data.capacity) {

        if (index >= G_buffer.gap_start && index < G_buffer.gap_end) {
            index = (int) G_buffer.gap_end + 1;
            continue;
        }

        char ch = G_buffer.data.chars[index];

        if (ch == '\n') {
            buffer[i++] = '\r';
            buffer[i++] = '\n';
        }

        else {
            buffer[i++] = ch;
        }

        index++;
    }

    buffer[i] = '\0';

    DWORD bytesWritten;
    WriteFile(file_handle, buffer, (DWORD)strlen(buffer),
              &bytesWritten, NULL);

    GlobalFree(buffer);
    CloseHandle(file_handle);
}

static POINT getCursorPos(int font_w, int font_h) {

    int x = 0, y = 0;

    for (uint index = 0; index < G_buffer.data.capacity; index++) {

        if (index == G_buffer.cursor_pos) break;

        if (index >= G_buffer.gap_start && index < G_buffer.gap_end) {
            continue;
        }

        char ch = G_buffer.data.chars[index];
        if (ch == '\n') {
            x = 0;
            y += font_h;
        } else {
            x += font_w;
        }
    }

    POINT pos = { x, y };
    return pos;
}

static void calculateLines() {

    G_buffer.lines.count = 0;
    uint start = 0, index = 0;

    // FIXME(Tejas): no idea why, but I changed < to <= here and the last
    //               line renders properly.
    for (int i = 0; i <= G_buffer.data.count; i++) {

        if (index >= G_buffer.gap_start && index < G_buffer.gap_end) {
            index = (int)(G_buffer.gap_end);
            continue;
        }

        if (index >= G_buffer.data.capacity) break;   

        char ch = G_buffer.data.chars[index];

        if (ch == '\n') {
            Line line = { };
            line.start = start;
            line.end   = index;

            if (G_buffer.lines.count >= G_buffer.lines.capacity) {

                if (G_buffer.lines.capacity == 0) G_buffer.lines.capacity = 16;

                G_buffer.lines.capacity *= 2;

                Line* new_lines = (Line*)GlobalReAlloc(G_buffer.lines.items,
                                                       G_buffer.lines.capacity * sizeof(Line),
                                                       GMEM_FIXED);
                if (!new_lines) return;
                G_buffer.lines.items = new_lines;
            }

            G_buffer.lines.items[G_buffer.lines.count++] = line;
            start = line.end + 1;
        }

        index++;
    }

    if (start < index) {
        
        Line line = { };
        line.start = start;
        line.end   = index;

        if (G_buffer.lines.count >= G_buffer.lines.capacity) {

            if (G_buffer.lines.capacity == 0) G_buffer.lines.capacity = 16;

            G_buffer.lines.capacity *= 2;

            Line* new_lines = (Line*)GlobalReAlloc(G_buffer.lines.items,
                                                   G_buffer.lines.capacity * sizeof(Line),
                                                   GMEM_FIXED);
            if (!new_lines) return;
            G_buffer.lines.items = new_lines;
        }

        G_buffer.lines.items[G_buffer.lines.count++] = line;
    }
}

static void moveGapToCursor() {

    uint gap_size = G_buffer.gap_end - G_buffer.gap_start;

    if (G_buffer.cursor_pos < G_buffer.gap_start) {
        
        uint cursor_difference = G_buffer.gap_start - G_buffer.cursor_pos;
        for (int i = 0; i < cursor_difference; i++) {
            uint src  = G_buffer.cursor_pos + i;
            uint dest = G_buffer.cursor_pos + gap_size + i;

            G_buffer.data.chars[dest] = G_buffer.data.chars[src];
        }

        G_buffer.gap_start = G_buffer.cursor_pos;
        G_buffer.gap_end   = G_buffer.gap_start + gap_size;
    }

    else if (G_buffer.cursor_pos > G_buffer.gap_start){
        
        int cursor_difference = (int)(G_buffer.cursor_pos - G_buffer.gap_end);
        for (int i = 0; i < cursor_difference; i++) {
            uint src  = G_buffer.gap_end + i;
            uint dest = G_buffer.gap_start + i;

            G_buffer.data.chars[dest] = G_buffer.data.chars[src];
        }

        G_buffer.gap_start = G_buffer.cursor_pos - gap_size;
        G_buffer.gap_end = G_buffer.cursor_pos;
    }
}

static void insertCharAtGap(char ch) {

    if (G_buffer.cursor_pos != G_buffer.gap_start) {
        moveGapToCursor();
    }

    if (ch == '\b') {
        if (G_buffer.gap_start > 0) {
            G_buffer.gap_start--;
            G_buffer.data.count--;
            G_buffer.cursor_pos = G_buffer.gap_start;
        }
        return;
    }

    if (G_buffer.gap_start >= G_buffer.gap_end) {
        // TODO(Tejas):
        // reallocate G_buffer.data.chars, 
        // shift the data from (gap_end + 1) to GAP_SIZE to the right
        // move gap_start to gap_end,
        // move gap_end to gap_end + GAP_SIZE
        return;
    }

    if (ch == '\r') {
        ch = '\n';
    }

    if (ch == '\t') {
        insertCharAtGap(' ');
        insertCharAtGap(' ');
        insertCharAtGap(' ');
        insertCharAtGap(' ');
        return;
    }

    G_buffer.data.chars[G_buffer.gap_start++] = ch;
    G_buffer.cursor_pos = G_buffer.gap_start;
    G_buffer.data.count++;
}

static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    switch (msg) {

    case WM_CREATE: {

        G_editor_opt.background = CreateSolidBrush(RGB(18, 18, 18));
        G_editor_opt.font_color = RGB(208, 208, 208);
        G_editor_opt.font = CreateFont(-24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");

        G_editor_opt.cursor_color = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));

        int file_size;
        G_buffer.file_name     = "test.txt";
        G_buffer.data.chars    = loadFileIntoBuffer(G_buffer.file_name, &file_size);
        G_buffer.data.capacity = file_size + GAP_SIZE;
        G_buffer.data.count    = file_size;
        G_buffer.gap_start     = file_size;
        G_buffer.gap_end       = (file_size + GAP_SIZE) - 1;
        G_buffer.cursor_pos    = G_buffer.gap_start;

        G_buffer.lines.capacity = MAX_LINES;
        G_buffer.lines.items    = (Line*)GlobalAlloc(GMEM_FIXED, G_buffer.lines.capacity * sizeof(Line));
        G_buffer.lines.count    = 0;

        calculateLines();
        
    } break;

    case WM_CHAR: {

        insertCharAtGap((char)wParam);
        calculateLines();
        InvalidateRect(hwnd, NULL, TRUE);

    } break;

    case WM_KEYDOWN: {
        bool moved = false;
        bool forward = false;

        switch (wParam) {

        case VK_LEFT:
            if (G_buffer.cursor_pos > 0) {
                G_buffer.cursor_pos--;
                moved = true;
                forward = false;
            }
            break;

        case VK_RIGHT:
            if (G_buffer.cursor_pos + 1 < G_buffer.data.capacity) {
                G_buffer.cursor_pos++;
                moved = true;
                forward = true;
            }
            break;

        case VK_UP:
            printBuffer();
            break;

        case VK_DOWN:
            saveFile();
            break;
        }

        if (moved) {
            if (G_buffer.cursor_pos >= G_buffer.gap_start &&
                G_buffer.cursor_pos <= G_buffer.gap_end) {

                if (forward) {
                    G_buffer.cursor_pos = G_buffer.gap_end + 1;
                } else {
                    G_buffer.cursor_pos = (G_buffer.gap_start > 0) ? (G_buffer.gap_start - 1) : 0;
                }
            }

            if (G_buffer.cursor_pos >= G_buffer.data.capacity) {
                G_buffer.cursor_pos = (G_buffer.data.capacity > 0) ? (G_buffer.data.capacity - 1) : 0;
            }
        }

        InvalidateRect(hwnd, NULL, TRUE);

    } break;

    case WM_PAINT: {

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, G_editor_opt.background);

        HFONT old_font = (HFONT)SelectObject(hdc, G_editor_opt.font);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);

        int cur_w = tm.tmAveCharWidth;
        int cur_h = tm.tmHeight;

        SetTextColor(hdc, G_editor_opt.font_color);
        SetBkMode(hdc, TRANSPARENT);

        int x = 0;
        int y = 0;

        for (int i = 0; i < G_buffer.lines.count; i++) {

            Line line = G_buffer.lines.items[i];

            char temp[4096];
            int len = 0;

            for (uint j = line.start; j < line.end; j++) {
                if (j >= G_buffer.gap_start && j < G_buffer.gap_end) {
                    continue;
                }
                temp[len++] = G_buffer.data.chars[j];
            }

            if (i == G_buffer.lines.count - 1)
                2;

            TextOutA(hdc, x, y, temp, len);
            y += cur_h;
        }

        // drawing cursor
        HPEN old_pen = (HPEN)SelectObject(hdc, G_editor_opt.cursor_color);

        POINT cur_pos = getCursorPos(cur_w, cur_h);
        Rectangle(hdc, cur_pos.x, cur_pos.y, cur_pos.x + 3, cur_pos.y + cur_h);

        SelectObject(hdc, old_pen);

        SelectObject(hdc, old_font);
        EndPaint(hwnd, &ps);

    } break;

    case WM_DESTROY: {
        if (G_buffer.data.chars != NULL)  GlobalFree(G_buffer.data.chars);
        if (G_buffer.lines.items != NULL) GlobalFree(G_buffer.lines.items);
        DeleteObject(G_editor_opt.background);
        DeleteObject(G_editor_opt.font);
        PostQuitMessage(0);
    } break;

    default: {
        result = DefWindowProc(hwnd, msg, wParam, lParam);
    } break;

    }

    return result;
}

// int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);

    G_window.width  = 1000;
    G_window.height = 800;

    G_window.name = "Text-Editor";

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSA wc = { };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = G_window.name;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, G_window.name,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                (int)G_window.width, (int)G_window.height,
                                NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
