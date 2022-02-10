#include <stdio.h>
#include "memory.h"
#include "value.h"

// intialize an array of values
void initValueArray(ValueArray* array) {
    array-> values = NULL;
    array-> capacity = 0;
    array-> count = 0;
}

// write a value to a value array, growing it if needed
void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

// free up a value array
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

// print a single value to debug
void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
    }
}

// Are two values equal?
// Arguments:
//  a - first value.
//  b - second value.
// Returns: false if they are different types or they are not equal, true otherwise.
// Note that NIL == NIL in Lox.
bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;

    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL( b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER( b);
        default:
            return false; // Unreachable.
    }
}
