
#ifndef _BASE_H_
#define _BASE_H_

#include <cstdint>
#include <cstdio>

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef int8_t    i8;
typedef uint8_t   u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

typedef size_t uint;
typedef int    sint;

#define global   static 
#define persist  static 
#define internal static 

#endif // _BASE_H_
