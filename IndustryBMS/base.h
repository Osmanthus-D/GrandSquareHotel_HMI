#ifndef BASE_H
#define BASE_H

#include <bms.h>

typedef struct _base_t_ {
    const unsigned int MODULE_NUM;

    _base_t_(int value = DEFAULT_MODULE_NUM) : MODULE_NUM(value) {
    }
} Base;

#endif // BASE_H
