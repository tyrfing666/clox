#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// get the object type.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// is it a string?
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// get the ObjString pointer.
#define AS_STRING(value) ((ObjString*) AS_OBJ(value))

// get pointer to the actual C string.
#define AS_CSTRING(value) (((ObjString*) AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
} ObjType;

// basic structure for an object.
struct Obj {
    ObjType type;
    struct Obj* next;
};

// structure for a string. Note the Obj is the first member, so pointer to 
// it can be used to reference the type.
struct ObjString {
    Obj obj;
    int length;
    char* chars;
};

ObjString* takeString(char* chars, int length);

// copy a C string into a Value.
ObjString* copyString(const char* chars, int length);

// print an object.
void printObject(Value value);

// test if the Value is an object of a given type.
// Arguments:
//  value - the value to test.
//  type - the type of object expected.
// Returns: true if it is an object of the expected type, false otherwise.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
