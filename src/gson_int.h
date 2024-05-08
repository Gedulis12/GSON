#ifndef GSON_INT_H_
#define GSON_INT_H_

#include <stdbool.h>
#include <stdint.h>

// Order of enum items must match to gson.h so that correct values gets returnes for the user facing api
typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE_VAL,
    JSON_FALSE_VAL,
    JSON_NULL_VAL,
    JSON_ROOT,
    JSON_TEMP_STUB,
} JSONType;

typedef enum {
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_TRUE, TOKEN_FALSE, TOKEN_NULL_VAL,
    TOKEN_ERROR, TOKEN_EOF, TOKEN_INIT,
} TokenType;

typedef struct jsonnode {
    JSONType type;
    uint32_t depth;
    char *key;
    char *str_val;
    float num_val;
    struct jsonnode *child;
    struct jsonnode *parent;
    struct jsonnode *next;
} JSONNode;

typedef struct {
    TokenType type;
    const char* start;
    uint32_t length;
    uint32_t line;
} Token;

typedef struct {
    const char *start;
    const char *current;
    uint32_t line;
} Scanner;

typedef struct parser {
    Scanner *scanner;
    Token previous;
    Token current;
    uint32_t depth;
    bool had_error;
    bool has_next; // helper context to determine whether the next node is a child or a sibling of the current node
    bool next_string_key; // helper context to determine whether the next JSON_STRING token is a key or a value
} Parser;

Parser* parser_init(char* source);
void parser_destroy(Parser *parser);
JSONNode* gson_parse(Parser *parser, JSONNode *node);
void gson_destroy(JSONNode *node);

#endif //GSON_INT_H_
