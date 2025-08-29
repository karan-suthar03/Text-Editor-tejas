#include "../include/buffer.h"
#include <windows.h>

internal char* loadFileIntoBuffer(const char* file_path, int *file_size) {
    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ,
                                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                     NULL);
    if (file_handle == INVALID_HANDLE_VALUE) return NULL;

    DWORD f_size = GetFileSize(file_handle, NULL);
    DWORD total_size = f_size + GAP_SIZE;
    char* raw = (char*)GlobalAlloc(GMEM_FIXED, total_size);
    memset(raw, 0, total_size);

    DWORD chars_read = 0;
    ReadFile(file_handle, raw, f_size, &chars_read, NULL);
    CloseHandle(file_handle);

    // Normalize new lines from /r/n to /n
    char* normalized = (char*)GlobalAlloc(GMEM_FIXED, total_size);
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

bool buffer_init(Buffer* buffer, const char* file_path) {
    int file_size;
    buffer->file_name = file_path;
    buffer->data.chars = loadFileIntoBuffer(buffer->file_name, &file_size);
    if (!buffer->data.chars) {
        // Handle file not found by creating an empty buffer
        file_size = 0;
        buffer->data.chars = (char*)GlobalAlloc(GMEM_FIXED, GAP_SIZE);
        memset(buffer->data.chars, 0, GAP_SIZE);
    }

    buffer->data.capacity = file_size + GAP_SIZE;
    buffer->data.count = file_size;
    buffer->gap_start = file_size;
    buffer->gap_end = (file_size + GAP_SIZE);
    buffer->cursor_pos = buffer->gap_start;

    buffer->lines.capacity = 1024; // Initial capacity
    buffer->lines.items = (Line*)GlobalAlloc(GMEM_FIXED, buffer->lines.capacity * sizeof(Line));
    buffer->lines.count = 0;

    buffer_calculate_lines(buffer);
    return true;
}

void buffer_free(Buffer* buffer) {
    if (buffer->data.chars != NULL)  GlobalFree(buffer->data.chars);
    if (buffer->lines.items != NULL) GlobalFree(buffer->lines.items);
}

void buffer_save(Buffer* buffer) {
    HANDLE file_handle = CreateFileA(buffer->file_name, GENERIC_WRITE, 0,
                                     NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                     NULL);
    if (file_handle == INVALID_HANDLE_VALUE) return;

    DWORD bytesWritten;
    for (uint i = 0, index = 0; i < buffer->data.count; i++, index++) {
        if (index >= buffer->gap_start && index < buffer->gap_end) {
            index = (int)buffer->gap_end;
        }

        char ch = buffer->data.chars[index];
        if (ch == '\n') {
            WriteFile(file_handle, "\r\n", 2, &bytesWritten, NULL);
        } else {
            WriteFile(file_handle, &ch, 1, &bytesWritten, NULL);
        }
    }
    CloseHandle(file_handle);
}

void buffer_move_gap_to_cursor(Buffer* buffer) {
    uint gap_size = buffer->gap_end - buffer->gap_start;

    if (buffer->cursor_pos < buffer->gap_start) {
        uint cursor_difference = buffer->gap_start - buffer->cursor_pos;
        memmove(&buffer->data.chars[buffer->gap_end - cursor_difference],
                &buffer->data.chars[buffer->cursor_pos],
                cursor_difference);
        buffer->gap_start = buffer->cursor_pos;
        buffer->gap_end = buffer->gap_start + gap_size;
    } else if (buffer->cursor_pos > buffer->gap_start) {
        uint cursor_difference = buffer->cursor_pos - buffer->gap_start;
        memmove(&buffer->data.chars[buffer->gap_start],
                &buffer->data.chars[buffer->gap_end],
                cursor_difference);
        buffer->gap_start = buffer->cursor_pos;
        buffer->gap_end = buffer->gap_start + gap_size;
    }
}


void buffer_insert_char(Buffer* buffer, char ch) {
    if (buffer->cursor_pos != buffer->gap_start) {
        buffer_move_gap_to_cursor(buffer);
    }

    if (ch == '\b') { // Backspace
        if (buffer->gap_start > 0) {
            buffer->gap_start--;
            buffer->data.count--;
            buffer->cursor_pos = buffer->gap_start;
        }
        return;
    }

    if (buffer->gap_start >= buffer->gap_end) {
        // TODO: Reallocate buffer if gap is full
        return;
    }

    if (ch == '\r') ch = '\n';

    if (ch == '\t') {
        // Insert 4 spaces for a tab
        for (int i = 0; i < 4; ++i) {
            buffer_insert_char(buffer, ' ');
        }
        return;
    }

    buffer->data.chars[buffer->gap_start++] = ch;
    buffer->cursor_pos = buffer->gap_start;
    buffer->data.count++;
}

void buffer_calculate_lines(Buffer* buffer) {
    buffer->lines.count = 0;
    uint start = 0;
    uint index = 0;

    for (uint i = 0; i <= buffer->data.count; ++i) {
        uint current_char_physical_index = index;

        // Determine the character
        char ch;
        if (i < buffer->data.count) {
            if (index >= buffer->gap_start && index < buffer->gap_end) {
                index = buffer->gap_end;
            }
            ch = buffer->data.chars[index];
        } else {
            ch = '\n';
        }

        if (ch == '\n') {
            // Check if we need to expand the lines array
            if (buffer->lines.count >= buffer->lines.capacity) {
                buffer->lines.capacity = (buffer->lines.capacity == 0) ? 16 : buffer->lines.capacity * 2;
                Line* new_lines = (Line*)GlobalReAlloc(buffer->lines.items,
                                                       buffer->lines.capacity * sizeof(Line),
                                                       GMEM_MOVEABLE);
                if (!new_lines) return; // Allocation failed
                buffer->lines.items = new_lines;
            }

            buffer->lines.items[buffer->lines.count++] = {start, current_char_physical_index};

            start = index + 1;
        }

        if (i < buffer->data.count) {
            index++;
        }
    }
}
