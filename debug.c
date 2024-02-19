#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"


void gson_debug_print_tree(JSONNode *node)
{
    if (node == NULL)
    {
        return;
    }
    char* type_val;
    size_t len;

    switch (node->type)
    {
        case JSON_ROOT:
            len = strlen("JSON_ROOT") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_ROOT");
            break;
        case JSON_ARRAY:
            len = strlen("JSON_ARRAY") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_ARRAY");
            break;
        case JSON_NUMBER:
            len = strlen("JSON_NUMBER") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_NUMBER");
            break;
        case JSON_OBJECT:
            len = strlen("JSON_OBJECT") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_OBJECT");
            break;
        case JSON_STRING:
            len = strlen("JSON_STRING") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_STRING");
            break;
        case JSON_NULL_VAL:
            len = strlen("JSON_NULL_VAL") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_NULL_VAL");
            break;
        case JSON_TRUE_VAL:
            len = strlen("JSON_TRUE_VAL") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_TRUE_VAL");
            break;
        case JSON_FALSE_VAL:
            len = strlen("JSON_FALSE_VAL") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_FALSE_VAL");
            break;
        case JSON_TEMP_STUB:
            len = strlen("JSON_TEMP_STUB") + 1;
            type_val = malloc(sizeof(char) * len);
            strcpy(type_val, "JSON_TEMP_STUB");
            break;
        default: break;
    }
    if (node->next != NULL)
    {
        printf("%s -> ", type_val);
    }
    else
    {
        printf("%s\n", type_val);
    }
    free(type_val);
    gson_debug_print_tree(node->next);
    gson_debug_print_tree(node->child);
}

void gson_debug_print_key(JSONNode *node)
{
    printf("DEBUG Key: %s\n", node->key);
}
void gson_debug_print_str_val(JSONNode *node)
{
    printf("DEBUG Value: %s\n", node->str_val);
}
void gson_debug_print_num_val(JSONNode *node)
{
    printf("DEBUG Value: %f\n", node->num_val);
}
