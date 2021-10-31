#ifndef __MAP_H
#define __MAP_H

#include "./buffer.h"

typedef struct {
    __buffer_t *key;
    void       *value;
} __map_t;

__map_t *new_map(void);

#endif // __MAP_H
