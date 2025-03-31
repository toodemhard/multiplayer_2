#pragma once

#include <stdint.h>

#define internal static
#define global static
#define local_persist static

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef float f32;
typedef double f64;

#define kilobytes(n) 1024ULL * n
#define megabytes(n) kilobytes(1024ULL) * n
#define gigabytes(n) megabytes(1024ULL) * n

#if COMPILER_MSVC
    #define alignof(T) __alignof(T)
#elif COMPILER_CLANG
    #define alignof(T) __alignof(T)
#else
    #error AlignOf not defined for this compiler
#endif

#define zero_struct(structp)\
    ( memset((structp), 0, sizeof(*(structp))) )

#define array_length(array) (sizeof(array) / sizeof((array)[0]))

#define array_copy(dst, src) (memcpy((dst), (src), sizeof(dst) / sizeof((dst)[0])))

// https://github.com/EpicGamesExt/raddebugger/blob/master/src/base/base_core.h
#define defer_loop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

// #define defer_loop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
