#include <stdio.h>
#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

// reset the stack to empty.
static void resetStack() {
    vm.stackTop = vm.stack;
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

// grab the next opcode and process it.
InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk-> code;
    return run();
}

// process an opcode.
static InterpretResult run() {
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    
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
            case OP_NEGATE: {
                push(-pop());
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
}
