#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

// reset the stack to empty.
static void resetStack() {
    vm.stackTop = vm.stack;
}

// handle a runtime error.
// Arguments:
//  format - the format to print.
//  ... the things to print using the format.
static void runtimeError(const char* format, ...) {
    // first line of error - print the arguments
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // second line - print where it occurred.
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    resetStack();
}

void initVM() {
    resetStack();
} 
 
void freeVM() {

}

// push an operand onto the stack.
void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

// pop an operand from the stack.
Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

// peek into the stack.
// Arguments: distance - how far into the stack do we want to peek?
// Returns: the value at that distance. The stack is unchanged.
static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

// grab the next opcode and process it.
InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);
    
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    InterpretResult result = run();
    freeChunk(&chunk);
    return result;
}

// process an opcode.
static InterpretResult run() {
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define BINARY_OP(valueType, op) \
    do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
    runtimeError("Operands must be numbers."); \
    return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
    } while (false)

    for (;;) {
        #ifdef DEBUG_TRACE_EXECUTION
            // print stack contents
            printf(" ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }
            printf("\n");
            
            // print instruction we're going to interpret
            disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif

        // interpret the instruction
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD: {
                BINARY_OP(NUMBER_VAL, +);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
}
