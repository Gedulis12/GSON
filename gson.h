#ifndef GSON_H_
#define GSON_H_

typedef struct parser Parser;

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE_VAL,
    JSON_FALSE_VAL,
    JSON_NULL_VAL,
} JSONType;

typedef struct JSONNode {
    JSONType type;
    char *key;
    char *str_val;
    float num_val;
    struct JSONNode *child;
    struct JSONNode *next;
} JSONNode;


extern Parser* parser_init(char* source);
extern JSONNode* gson_parse(Parser *parser, JSONNode *node);

#endif
