#include <stdlib.h>
#include "memory.h"

// Our one memory allocation routine, which will grow as needed and also free if nothing is to be allocated.
void* reallocate( void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc( pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}
