#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;

Chunk* compilingChunk;

// get the current chunk to compile to.
static Chunk* currentChunk() {
    return compilingChunk;
}

// error reporting.
// Arguments:
//  token - the token which had the error.
//  message - the error message to report.
static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token-> type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

// handle an error at the current token.
// Arguments: message - the error message to report.
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

// handle an error with the previous token.
// Arguments: message - the error message to report.
static void error(const char* message) {
    errorAt(&parser.previous, message);
}

// advance one token.
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}

// consume one token. If it's not of the expected type, report an error.
// Arguments:
//  type - the expected type.
//  message - the error message if it's not correct.
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

// emit a byte to the current chunk.
// Argument: byte - the byte to emit.
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// convenience function to emit two bytes (generally an opcode plus an operand).
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte( byte2);
}

// emit a "return".
static void emitReturn() {
    emitByte(OP_RETURN);
}

// add a constant to the pool in the current chunk.
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);

    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

// emit a constant.
// Arguments: value - the number.
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// end the compiling phase.
static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

// forward declarations so we can put them in the rules table.
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence( Precedence precedence);

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule-> precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return; // Unreachable.
    }
}

// handle a grouping.
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// emit a constant that is a number.
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

// handle a unary operator.
static void unary() {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return; // Unreachable.
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = {grouping,    NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_COMMA]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_DOT]             = {NULL,        NULL,   PREC_NONE},
    [TOKEN_MINUS]           = {unary,       binary, PREC_TERM},
    [TOKEN_PLUS]            = {NULL,        binary, PREC_TERM},
    [TOKEN_SEMICOLON]       = {NULL,        NULL,   PREC_NONE},
    [TOKEN_SLASH]           = {NULL,        binary, PREC_FACTOR},
    [TOKEN_STAR]            = {NULL,        binary, PREC_FACTOR},
    [TOKEN_BANG]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_EQUAL]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL,        NULL,   PREC_NONE},
    [TOKEN_GREATER]         = {NULL,        NULL,   PREC_NONE},
    [TOKEN_GREATER_EQUAL]   = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LESS]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_LESS_EQUAL]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IDENTIFIER]      = {NULL,        NULL,   PREC_NONE},
    [TOKEN_STRING]          = {NULL,        NULL,   PREC_NONE},
    [TOKEN_NUMBER]          = {number,      NULL,   PREC_NONE},
    [TOKEN_AND]             = {NULL,        NULL,   PREC_NONE},
    [TOKEN_CLASS]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ELSE]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_FALSE]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_FOR]             = {NULL,        NULL,   PREC_NONE},
    [TOKEN_FUN]             = {NULL,        NULL,   PREC_NONE},
    [TOKEN_IF]              = {NULL,        NULL,   PREC_NONE},
    [TOKEN_NIL]             = {NULL,        NULL,   PREC_NONE},
    [TOKEN_OR]              = {NULL,        NULL,   PREC_NONE},
    [TOKEN_PRINT]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_RETURN]          = {NULL,        NULL,   PREC_NONE},
    [TOKEN_SUPER]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_THIS]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_TRUE]            = {NULL,        NULL,   PREC_NONE},
    [TOKEN_VAR]             = {NULL,        NULL,   PREC_NONE},
    [TOKEN_WHILE]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_ERROR]           = {NULL,        NULL,   PREC_NONE},
    [TOKEN_EOF]             = {NULL,        NULL,   PREC_NONE},
};

// parse out the expression.
// Arguments: precedence - the minimum precedence to continue the current expression.
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error(" Expect expression.");
        return;
    }
    prefixRule();
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

// get the rule for the given token.
// Arguments: type - the token type.
// Returns: the rule function.
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

// compile an expression
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

// compile an expresion.
// Arguments:
//  source - the source code to compile.
//  chunk - the chunk to compile the code to.
// Returns: true if OK, false if there was an error.
bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression");
    endCompiler();

    return !parser.hadError;
}