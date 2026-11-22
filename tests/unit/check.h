#ifndef STREAMFORGE_TEST_CHECK_H
#define STREAMFORGE_TEST_CHECK_H

#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(EXIT_FAILURE);                                             \
        }                                                                   \
    } while (0)

#endif
