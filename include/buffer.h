#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "base.h"

#define GAP_SIZE 1024

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

bool buffer_init(Buffer* buffer, const char* file_path);

void buffer_free(Buffer* buffer);

void buffer_save(Buffer* buffer);

void buffer_insert_char(Buffer* buffer, char ch);

void buffer_calculate_lines(Buffer* buffer);

void buffer_move_gap_to_cursor(Buffer* buffer);

#endif // _BUFFER_H_