#include "../include/editor.h"

#include <string>

internal uint get_cursor_row(Buffer* buffer) {
    for (uint i = 0; i < buffer->lines.count; i++) {
        Line l = buffer->lines.items[i];
        if (buffer->cursor_pos >= l.start && buffer->cursor_pos <= l.end) {
            return i;
        }
    }
    return buffer->lines.count > 0 ? buffer->lines.count - 1 : 0;
}

internal uint get_cursor_col(Buffer* buffer) {
    uint row = get_cursor_row(buffer);
    if (row >= buffer->lines.count) return 0;

    Line l = buffer->lines.items[row];
    uint col = 0;
    for (uint index = l.start; index < buffer->cursor_pos; ++index) {
        if (index >= buffer->gap_start && index < buffer->gap_end) {
            continue;
        }
        col++;
    }
    return col;
}

internal POINT get_cursor_pixel_pos(Buffer* buffer, int font_w, int font_h) {
    int x = 0, y = 0;
    for (uint index = 0; index < buffer->cursor_pos; index++) {
        if (index >= buffer->gap_start && index < buffer->gap_end) {
            continue;
        }
        char ch = buffer->data.chars[index];
        if (ch == '\n') {
            x = 0;
            y += font_h;
        } else {
            x += font_w;
        }
    }
    return {x, y};
}

void editor_init(Editor* editor, HWND hwnd) {
    editor->window.handle = hwnd;

    editor->options.background = CreateSolidBrush(RGB(18, 18, 18));
    editor->options.font_color = RGB(208, 208, 208);
    editor->options.font = CreateFont(-24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    editor->options.cursor_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));

    // Initialize Buffer
    buffer_init(&editor->buffer, "test.txt");

    editor->running = TRUE;
}

void editor_free(Editor* editor) {
    buffer_free(&editor->buffer);
    DeleteObject(editor->options.background);
    DeleteObject(editor->options.font);
    DeleteObject(editor->options.cursor_pen);
}

void editor_handle_char(Editor* editor, WPARAM wParam) {
    bool ctrl_down = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (!ctrl_down) {
        buffer_insert_char(&editor->buffer, (char)wParam);
        buffer_calculate_lines(&editor->buffer);
        InvalidateRect(editor->window.handle, NULL, TRUE);
    }
}

void editor_handle_keydown(Editor* editor, WPARAM wParam) {
    Buffer* buffer = &editor->buffer;
    bool moved = false;
    bool forward = false;

    switch (wParam) {
        case 'S':
            if (GetKeyState(VK_CONTROL) & 0x8000) buffer_save(buffer);
            break;

        case VK_LEFT:
            if (buffer->cursor_pos > 0) {
                buffer->cursor_pos--;
                if (buffer->cursor_pos > buffer->gap_start && buffer->cursor_pos < buffer->gap_end) {
                    buffer->cursor_pos = buffer->gap_start;
                }
                moved = true;
            }
            break;

        case VK_RIGHT:
            if (buffer->cursor_pos < buffer->data.capacity) {
                 buffer->cursor_pos++;
                if (buffer->cursor_pos > buffer->gap_start && buffer->cursor_pos < buffer->gap_end) {
                    buffer->cursor_pos = buffer->gap_end;
                }
                moved = true;
            }
            break;

        case VK_UP: {
            uint col = get_cursor_col(buffer);
            uint row = get_cursor_row(buffer);
            if (row > 0) {
                Line prev_line = buffer->lines.items[row - 1];
                buffer->cursor_pos = prev_line.start + MIN(col, prev_line.end - prev_line.start);
            }
        } break;

        case VK_DOWN: {
            uint col = get_cursor_col(buffer);
            uint row = get_cursor_row(buffer);
            if (row < buffer->lines.count - 1) {
                Line next_line = buffer->lines.items[row + 1];
                buffer->cursor_pos = next_line.start + MIN(col, next_line.end - next_line.start);
            }
        } break;
    }

    if (moved) {
        uint max_pos = buffer->gap_start + (buffer->data.capacity - buffer->gap_end);
        if (buffer->cursor_pos > max_pos) buffer->cursor_pos = max_pos;
    }

    InvalidateRect(editor->window.handle, NULL, TRUE);
}

void editor_paint(Editor* editor, HDC hdc) {
    RECT rect;
    GetClientRect(editor->window.handle, &rect);
    FillRect(hdc, &rect, editor->options.background);

    HFONT old_font = (HFONT)SelectObject(hdc, editor->options.font);
    SetTextColor(hdc, editor->options.font_color);
    SetBkMode(hdc, TRANSPARENT);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    int font_w = tm.tmAveCharWidth;
    int font_h = tm.tmHeight;

    int y = 0;
    for (uint i = 0; i < editor->buffer.lines.count; i++) {
        auto [start, end] = editor->buffer.lines.items[i];
        const uint line_length = end - start;
        std::string line_text;
        // pre allocating needed memory to the string
        line_text.reserve(line_length);

        for (uint j = start; j < end; j++) {
            if (j >= editor->buffer.gap_start && j < editor->buffer.gap_end) {
                continue; // skip gaps
            }
            line_text.push_back(editor->buffer.data.chars[j]);
        }
        TextOutA(hdc, 0, y, line_text.c_str(), line_text.length());
        y += font_h;
    }

    HPEN old_pen = (HPEN)SelectObject(hdc, editor->options.cursor_pen);
    POINT cur_pos = get_cursor_pixel_pos(&editor->buffer, font_w, font_h);
    MoveToEx(hdc, cur_pos.x, cur_pos.y, NULL);
    LineTo(hdc, cur_pos.x, cur_pos.y + font_h);

    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_font);
}