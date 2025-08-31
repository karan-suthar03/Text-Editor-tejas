/*===============================================================================
  @File:   editor.h
  @Brief:  
  @Author: Tejas
  @Date:   28-08-2025
  @Notice: Released under the MIT License. See LICENSE file for details.
  ===============================================================================*/

#ifndef EDITOR_H
#define EDITOR_H

#include "platform.h"

#define GET_GAP_SIZE(gb)    (((gb)->gap_end) - ((gb)->gap_start) + 1)
#define GET_TEXT_LENGTH(gb) ((gb)->data.capacity - GET_GAP_SIZE(gb))

struct Line {
    int start;
    int end;
};

struct Lines {
    Line *items;
    int count;
    int capacity;
};

struct Data {
    char *chars;
    int capacity;
};

// NOTE(Tejas): gap_start and gap_end are inclusive.
struct GapBuffer {
    Data  data;
    Lines lines;
    int   gap_start;
    int   gap_end;
    int   cur_pos;
};

// NOTE(Tejas): Editor Theme and stuff can go in here
struct Editor {
    GapBuffer gb;
    const char* file_name;
};

void ed_Init(Editor **ed, const char *file_name);
void ed_Close(Editor *ed);

void ed_RecalculateLines(Editor *ed);

void ed_InsertCharAtCursor(Editor *ed, char ch);

void ed_MoveCursorRight(Editor *ed);
void ed_MoveCursorLeft(Editor *ed);
void ed_MoveCursorUp(Editor *ed);
void ed_MoveCursorDown(Editor *ed);

#endif // EDITOR_H
