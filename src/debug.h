#ifndef GSON_DEBUG_H
#define GSON_DEBUG_H

#include "gson_int.h"
#include "time.h"

#define PARSING_TIMER(parser) \
    clock_t t; \
    t = clock(); \
    JSONNode *node = gson_parse(parser, NULL); \
    t = clock() - t; \
    double time_taken = ((double)t)/CLOCKS_PER_SEC; \
    if (node->type == JSON_ROOT) \
    { \
            printf("parsing took %f seconds to execute \n", time_taken); \
    } \



void gson_debug_print_tree(JSONNode *node, int depth);
void gson_debug_print_key(JSONNode *node);
void gson_debug_print_str_val(JSONNode *node);
void gson_debug_print_num_val(JSONNode *node);
void gson_debug_general(Parser *parser, JSONNode *node);

#endif
