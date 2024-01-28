#ifndef GSON_H_
#define GSON_H_

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE_VAL,
    JSON_FALSE_VAL,
    JSON_NULL_VAL,
} JSONType;

struct JSONNode {
    JSONType type;
    char *key;
    char *str_val;
    float num_val;
    struct JSONNode *child;
    struct JSONNode *parent;
    struct JSONNode *next;
};

#endif
