#ifndef SRC_NES_ASSERT_H_
#define SRC_NES_ASSERT_H_

#include <stdint.h>

#define NES_ASSERT(x)           NES_ASSERT_ALWAYS(x)
#define NES_ASSERT_ALWAYS(x)    if (!(x)) do {AssertFailed(__FILE__, __LINE__, #x);} while(0)

void AssertFailed(const char *file, int32_t line, const char* msg);

#endif /* SRC_NES_ASSERT_H_ */
