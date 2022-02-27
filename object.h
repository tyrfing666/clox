#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

// get the object type.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// is it a class?
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)

// is it a closure?
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)

// is it a Lox function?
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// is it a class instance?
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)

// is it a native function?
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

// is it a string?
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// cast to class
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))

// cast to closure.
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

// cast to a pointer to function (assuming it's safe)
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

// cast to a class instance
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))

// cast to a native function object.
#define AS_NATIVE(value) \
(((ObjNative*)AS_OBJ(value))->function)

// get the ObjString pointer (assuming it's safe).
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// get pointer to the actual C string.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

// basic structure for an object.
struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj* next;
};

// a function.
typedef struct {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

// structure for a string. Note the Obj is the first member, so pointer to 
// it can be used to reference the type.
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

// structure for a closure.
typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

// structure for a class.
typedef struct {
    Obj obj;
    ObjString* name;
} ObjClass;

// structure for a class instance.
typedef struct {
    Obj obj;
    ObjClass* klass;
    Table fields;
} ObjInstance;

// create a new class.
ObjClass* newClass(ObjString* name);

// create a new closure.
ObjClosure* newClosure(ObjFunction* function);

// create a Lox function.
ObjFunction* newFunction();

// create a new class instance.
ObjInstance* newInstance(ObjClass* klass);

// create a new representation for a native C function
ObjNative* newNative( NativeFn function);

ObjString* takeString(char* chars, int length);

// copy a C string into a Value.
ObjString* copyString(const char* chars, int length);

// create a new upvalue item.
ObjUpvalue* newUpvalue(Value* slot);

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
