#ifndef SRC_NES_TYPES_H_
#define SRC_NES_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;

#if 0
typedef u32_t u8f_t;
typedef u32_t u16f_t;
typedef u32_t u32f_t;
#else
typedef u8_t u8f_t;
typedef u16_t u16f_t;
typedef u32_t u32f_t;
#endif
#endif /* SRC_NES_TYPES_H_ */
