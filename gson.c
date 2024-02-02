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

typedef struct {
    Token previous;
    Token current;
    int depth;
    bool had_error;
} Parser;

static Scanner* scanner_init(const char *source)
{
    Scanner *scanner = (Scanner*)malloc(sizeof(Scanner));
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    return scanner;
}

static Token token_make(TokenType type, Scanner *scanner)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static Token token_error(Scanner *scanner, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;
    return token;
}

static bool scanner_is_at_end(Scanner *scanner)
{
    return *scanner->current == '\0';
}

static char peek(Scanner *scanner)
{
    return *scanner->current;
}

static char peek_next(Scanner *scanner)
{
    if (scanner_is_at_end(scanner)) return '\0';
    return scanner->current[1];
}

static char scanner_advance(Scanner *scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

static void scanner_skip_white_space(Scanner *scanner)
{
    for (;;)
    {
        char c = peek(scanner);
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
                scanner_advance(scanner);
                break;
            case '\n':
                scanner->line++;
                scanner_advance(scanner);
                break;
            default:
                return;
        }
    }
}

static bool match(char expected, Scanner *scanner)
{
    if (scanner_is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_hex(char c)
{
    return (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F') ||
           (c >= '0' && c <= '9');
}

//TODO: handle the escape sequences correctly
static Token token_string(Scanner *scanner)
{
    while ((peek(scanner) != '"') && !scanner_is_at_end(scanner))
    {
        if (peek(scanner) == '\n') scanner->line++;
        scanner_advance(scanner);
    }

    if (scanner_is_at_end(scanner)) return token_error(scanner, "Unterminated string");

    scanner_advance(scanner);
    return token_make(TOKEN_STRING, scanner);
}

static Token token_number(Scanner *scanner)
{
    while (is_digit(peek(scanner))) scanner_advance(scanner);

    if (peek(scanner) == '.' && is_digit(peek_next(scanner)))
    {
        scanner_advance(scanner);
        while (is_digit(peek(scanner))) scanner_advance(scanner);
    }

    return token_make(TOKEN_NUMBER, scanner);
}

static Token token_boolean(Scanner *scanner)
{
    const char *true_str = "true";
    const char *false_str = "false";

    if (strncmp(true_str, scanner->current - 1, strlen(true_str)) == 0)
    {
        while (peek(scanner) != 'e') scanner_advance(scanner);
        scanner_advance(scanner);
        return token_make(TOKEN_TRUE, scanner);
    }
    else if (strncmp(false_str, scanner->current - 1, strlen(false_str)) == 0)
    {
        while (peek(scanner) != 'e') scanner_advance(scanner);
        scanner_advance(scanner);
        return token_make(TOKEN_FALSE, scanner);
    }

    return token_error(scanner, "Only 'true' and 'false' are valid boolean keywords");
}

static Token token_null(Scanner *scanner)
{
    const char *null_str = "null";
    char debugstr[120];
    if (strncmp(null_str, scanner->current - 1, strlen(null_str)) == 0)
    {
        while (peek(scanner) != 'l') scanner_advance(scanner);
        scanner_advance(scanner);
        scanner_advance(scanner);
        return token_make(TOKEN_NULL_VAL, scanner);
    }

    strncpy(debugstr, scanner->start, scanner->current - scanner->start);
    printf("DEBUG: %s\n", debugstr);

    return token_error(scanner, "Only 'null' is a valid keyword for the null value");
}

static Token scan_token(Scanner *scanner)
{
    scanner_skip_white_space(scanner);
    scanner->start = scanner->current;

    if (scanner_is_at_end(scanner)) return token_make(TOKEN_EOF, scanner);

    char c = scanner_advance(scanner);
    if (is_digit(c)) return token_number(scanner);

    switch(c)
    {
        case '{': return token_make(TOKEN_LEFT_BRACE, scanner);
        case '}': return token_make(TOKEN_RIGHT_BRACE, scanner);
        case '[': return token_make(TOKEN_LEFT_BRACKET, scanner);
        case ']': return token_make(TOKEN_RIGHT_BRACKET, scanner);
        case ',': return token_make(TOKEN_COMMA, scanner);
        case ':': return token_make(TOKEN_COLON, scanner);
        case '"': return token_string(scanner);
        case 't': return token_boolean(scanner);
        case 'f': return token_boolean(scanner);
        case 'n': return token_null(scanner);
    }
    return token_error(scanner, "Unexpected Character.");
}

static Parser* parser_init()
{
    Parser *parser = (Parser*)malloc(sizeof(Parser));
    parser->depth = 0;
    parser->had_error = false;
    return parser;
}

static void parser_advance(Parser *parser, Scanner *scanner)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = scan_token(scanner);
        if (parser->current.type != TOKEN_ERROR) break;
        char *debug_str = malloc((sizeof(char) * parser->current.length) + 1);
        strncpy(debug_str, parser->current.start, parser->current.length);
        printf("Error at line %i: %s\n", parser->current.line, debug_str);
    }
}

static bool parser_check_type(Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

static bool parser_match_token(Parser *parser, TokenType type)
{
    if (!parser_check_type(parser, type)) return false;
//    parser_advance(parser);
    return true;
}


static JSONNode* gson_node_create()
{
    JSONNode *node = (JSONNode*)malloc(sizeof(JSONNode));
    return node;
}

// either pass parser and scanner to this function and continue recursivelly or handle the closing of objects somewhere else
static JSONNode* gson_node_object(Parser *parser, Scanner *scanner)
{
    JSONNode *node = gson_node_create();
    node->type = JSON_OBJECT;
    int depth = parser->depth;

    if (parser->current.type == TOKEN_RIGHT_BRACE && parser->depth == depth)
    {
        return node;
    }



    return node;
}

JSONNode* gson_parse(char *source)
{
    JSONNode *node;
    Scanner *scanner = scanner_init(source);
    Parser *parser = parser_init();
    TokenType type;

    while (parser->current.type != TOKEN_EOF)
    {
        parser_advance(parser, scanner);
        type = parser->current.type;
        switch (type)
        {
            case TOKEN_LEFT_BRACE:
                parser->depth++;
                //node = gson_node_object(parser, scanner);
            case TOKEN_RIGHT_BRACE:
                parser->depth--;
            default: NULL;
        }


        // DEBUG
        int line = parser->current.line;
        const char *start = parser->current.start;
        int length = parser->current.length;

        char *debug_str = malloc((sizeof(char) * length) + 1);
        strncpy(debug_str, start, length);
        printf("DEBUG PARSER: type: %d, line: %d, value: %s, length: %d\n", type, line, debug_str, length);
    }

    return NULL;
}

int main()
{
    char *source = "{               \n\
    \"name\": \"Matt\",             \n\
    \"gender\": \"male\",           \n\
    \"age\": 30,                    \n\
    \"has_kids\": true,             \n\
    \"kids\":                       \n\
    [                               \n\
        {                           \n\
            \"name\": \"Sissy\",    \n\
            \"age\": 5              \n\
            \"toys\": false         \n\
        },                          \n\
        {                           \n\
            \"name\": \"Augu\\nst\",\n\
            \"age\": 7              \n\
            \"toys\": null          \n\
        }                           \n\
    ]                               \n\
}";

    printf("DEBUG STRING:\n%s\n", source);
    gson_parse(source);

//    Token token;
//    while (token.type != TOKEN_EOF)
//    {
//        token = scan_token(scanner);
//        TokenType type = token.type;
//        int line = token.line;
//        const char *start = token.start;
//        int length = token.length;
//
//        char *debug_str = malloc(sizeof(char) * 256);
//        strncpy(debug_str, start, length);
//
//        printf("DEBUG: type: %d, line: %d, value: %s, length: %d\n", type, line, debug_str, length);
//    }

}
