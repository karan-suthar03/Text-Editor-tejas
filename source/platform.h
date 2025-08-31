/*===============================================================================
  @File:   platform.h
  @Brief:  
  @Author: Tejas
  @Date:   30-08-2025
  @Notice: Released under the MIT License. See LICENSE file for details.
  ===============================================================================*/

#ifndef PLATFORM_H
#define PLATFORM_H

#include "base.h"

// TODO(Tejas): Create a memory pool to allocate memory in batches
void* AllocateMem(int size);
void* ReallocateMem(void* ptr, int new_size);
void FreeMem(void* ptr);

char* LoadFileIntoGapBuffer(const char* file_path, int *file_size, int *buffer_size, int gap_size);
void SaveGapBufferToFile(const char* file_path, char *buffer, int buffer_count, int gap_start, int gap_end);

#endif // PLATFORM_H
