#include <stdio.h>
#include <string.h>
#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;     // start of the string we're trying to match to a token
    const char* current;   // current character in the string.
    int line;              // source code line number we can pass to the error or debugger.
} Scanner;

// the scanner.
Scanner scanner;

// initialize the scanner.
void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

// is the character an alphabetical character (used for identifiers)
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// is this character a number?
static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

// are we at the end of the source code?
// Returns: true if we are.
static bool isAtEnd() {
    return *scanner.current == '\0';
}

// advance one character.
// Returns: the character we have advanced to.
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

// peek ahead one character, not consuming it.
static char peek() {
    return *scanner.current;
}

// peek at the second character ahead, not consuming it.
static char peekNext() {
    if (isAtEnd()) return '\0';

    return scanner.current[1];
}

// look ahead one character and see if it matches the argument, consuming it only if it matches.
// Arguments: expected - character to match against.
// Returns: true if matched.
static bool match(char expected) {
    if (isAtEnd()) return false;

    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

// make a token of a given type out of the string we have recognized in the scanner.
static Token makeToken(TokenType type) {
    Token token;

    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

// make an error 'token' we can pass back to the compiler.
// Arguments: message to use as the 'token' source.
// Returns: the token.
static Token errorToken(const char* message) {
    Token token;
    
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    
    return token;
}

// skip all whitespace (including comments) to the start of the next token.
static void skipWhiteSpace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

// what type of identifier is it? Uses a trie to match against keywords, if it doesn't then it's an identifier (variable).
// Returns: the token type.
static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

// make and return an identifier token.
// returns: the identifier token.
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();

    return makeToken(identifierType());
}

// make and return a number token.
// Returns: the number token.
static Token number() {
    while (isDigit(peek())) advance();
    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the ".".
        advance();
        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

// make and return a string token.
// Returns: the string token.
static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
        }
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string.");
    // The closing quote.
    advance();
    return makeToken(TOKEN_STRING);
}

// do the work of scanning until we recognize a token then return it.
// Returns: the next token (possibily the error token).
Token scanToken() {
    skipWhiteSpace();
    scanner.start = scanner.current;
    
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();
    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}
