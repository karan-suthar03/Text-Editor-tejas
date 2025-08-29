#ifndef _EDITOR_H_
#define _EDITOR_H_

#include "buffer.h"
#include <windows.h>

struct Window {
    const char* name;
    uint  width;
    uint  height;
    HWND  handle;
};

struct EditorOptions {
    HFONT    font;
    COLORREF font_color;
    HBRUSH   background;
    HPEN     cursor_pen;
};

struct Editor {
    Window        window;
    EditorOptions options;
    Buffer        buffer;
    BOOL          running;
};

void editor_init(Editor* editor, HWND hwnd);

void editor_free(Editor* editor);

void editor_handle_char(Editor* editor, WPARAM wParam);

void editor_handle_keydown(Editor* editor, WPARAM wParam);

void editor_paint(Editor* editor, HDC hdc);

#endif // _EDITOR_H_