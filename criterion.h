#ifndef CRITERION_H_
#define CRITERION_H_

#include <stdlib.h>

#define Test(suite, name) void test_##suite##__##name(void)

void cr_assert(bool ok) {
    if (!ok)
        abort();
}

#endif
