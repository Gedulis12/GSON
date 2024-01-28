#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "gson.h"

typedef enum {
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_COLON,
    TOKEN_SINGLE_QUOTE, TOKEN_DOUBLE_QUOTE,
    TOKEN_STRING, TOKEN_NUMBER,
    TOKEN_TRUE, TOKEN_FALSE, TOKEN_NULL_VAL,
    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

static Scanner* initScanner(const char *source)
{
    Scanner *scanner = (Scanner*)malloc(sizeof(Scanner));
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    return scanner;
}

static Token makeToken(TokenType type, Scanner *scanner)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;

    return token;
}

static Token errorToken(Scanner *scanner, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;
    return token;
}

static bool isAtEnd(Scanner *scanner)
{
    return *scanner->current == '\0';
}

static char peek(Scanner *scanner)
{
    return *scanner->current;
}

static char peekNext(Scanner *scanner)
{
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static char advance(Scanner *scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

static void skipWhiteSpace(Scanner *scanner)
{
    for (;;)
    {
        char c = peek(scanner);
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            default:
                return;
        }
    }
}

static bool match(char expected, Scanner *scanner)
{
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static Token string(Scanner *scanner, char quoteSymbol)
{
    while ((peek(scanner) != '"') && !isAtEnd(scanner))
    {
        if (peek(scanner) == '\n') scanner->line++;
        advance(scanner);
    }

    if (isAtEnd(scanner)) return errorToken(scanner, "Unterminated string");

    if (advance(scanner) != quoteSymbol)
    {
        return errorToken(scanner, "String quoted with non-matching quotes");
    }
    return makeToken(TOKEN_STRING, scanner);
}

static Token number(Scanner *scanner)
{
    while (isDigit(peek(scanner))) advance(scanner);

    if (peek(scanner) == '.' && isDigit(peekNext(scanner)))
    {
        advance(scanner);
        while (isDigit(peek(scanner))) advance(scanner);
    }

    return makeToken(TOKEN_NUMBER, scanner);
}

static Token boolean(Scanner *scanner)
{
    const char *true_str = "true";
    const char *false_str = "false";

    if (strncmp(true_str, scanner->current - 1, strlen(true_str)) == 0)
    {
        while (peek(scanner) != 'e') advance(scanner);
        advance(scanner);
        return makeToken(TOKEN_TRUE, scanner);
    }
    else if (strncmp(false_str, scanner->current - 1, strlen(false_str)) == 0)
    {
        while (peek(scanner) != 'e') advance(scanner);
        advance(scanner);
        return makeToken(TOKEN_FALSE, scanner);
    }

    return errorToken(scanner, "Unexpected keyword");
}

static Token null(Scanner *scanner)
{
    const char *null_str = "null";
    char debugstr[120];
    if (strncmp(null_str, scanner->current - 1, strlen(null_str)) == 0)
    {
        while (peek(scanner) != 'l') advance(scanner);
        advance(scanner);
        advance(scanner);
        return makeToken(TOKEN_NULL_VAL, scanner);
    }

    strncpy(debugstr, scanner->start, scanner->current - scanner->start);
    printf("DEBUG: %s\n", debugstr);

    return errorToken(scanner, "Unexpected keyword");
}

static Token scanToken(Scanner *scanner)
{
    skipWhiteSpace(scanner);
    scanner->start = scanner->current;

    if (isAtEnd(scanner)) return makeToken(TOKEN_EOF, scanner);

    char c = advance(scanner);
    if (isDigit(c)) return number(scanner);

    switch(c)
    {
        case '{': return makeToken(TOKEN_LEFT_BRACE, scanner);
        case '}': return makeToken(TOKEN_RIGHT_BRACE, scanner);
        case '[': return makeToken(TOKEN_LEFT_BRACKET, scanner);
        case ']': return makeToken(TOKEN_RIGHT_BRACKET, scanner);
        case ',': return makeToken(TOKEN_COMMA, scanner);
        case ':': return makeToken(TOKEN_COLON, scanner);
        case '"': return string(scanner, '"');
        case '\'': return string(scanner, '\'');
        case 't': return boolean(scanner);
        case 'f': return boolean(scanner);
        case 'n': return null(scanner);
    }
    return errorToken(scanner, "Unexpected Character.");
}

int main()
{
    const char *source = "{ \n\
    \"name\": \"Matt\",         \n\
    \"gender\": \"male\",       \n\
    \"age\": 30,              \n\
    \"has_kids\": true,       \n\
    \"kids\":                 \n\
    [                       \n\
        {                   \n\
            \"name\": \"Sissy\",\n\
            \"age\": 5        \n\
            \"toys\": false\n\
        },                  \n\
        {                   \n\
            \"name\": \"August\",\n\
            \"age\": 7\n\
            \"toys\": null\n\
        }\n\
    ]\n\
}";
    printf("DEBUG STRING:\n%s\n", source);

    Scanner *scanner = initScanner(source);
    Token token;
    while (token.type != TOKEN_EOF)
    {
        token = scanToken(scanner);
        TokenType type = token.type;
        int line = token.line;
        const char *start = token.start;
        int length = token.length;

        char *debug_str = malloc(sizeof(char) * 256);
        strncpy(debug_str, start, length);

        printf("DEBUG: type: %d, line: %d, value: %s, length: %d\n", type, line, debug_str, length);
    }
}
