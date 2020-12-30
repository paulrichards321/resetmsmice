#ifndef _BASIC_TYPES_H
#define _BASIC_TYPES_H

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
#ifndef HAVE__BOOL
#  ifdef __cplusplus
#    ifndef bool
typedef bool _Bool;
#    endif
#  else
#    ifndef _Bool
#      define _Bool signed char
#    endif
#  endif
#endif
#ifndef bool
#define bool _Bool
#define false 0
#define true 1
#define __bool_true_false_are_defined 1
#endif
#endif

#ifndef __u8
#define __u8 uint8_t
#define __s8 int8_t
#define __u16 uint16_t
#define __s16 int16_t
#define __le16 uint16_t
#define __u32 uint32_t
#define __s32 int32_t
#define __le32 uint32_t
#define __u64 uint64_t
#define __s64 int64_t
#define __le64 uint64_t
#endif

#ifndef u8
#define u8 uint8_t
#define s8 int8_t
#define u16 uint16_t
#define s16 int16_t
#define u32 uint32_t
#define s32 int32_t
#define u64 uint64_t
#define s64 int64_t
#endif

#ifndef max_t
#define max_t(type, x, y) ({            \
    type __max1 = (x);          \
    type __max2 = (y);          \
    __max1 > __max2 ? __max1: __max2; })
#endif

#endif
