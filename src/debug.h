#ifndef GSON_DEBUG_H
#define GSON_DEBUG_H

#include "gson_int.h"
#include "time.h"
#include <stdio.h>
#include <stdint.h>

void gson_debug_print_tree(JSONNode *node, int depth);
void gson_debug_print_key(JSONNode *node);
void gson_debug_print_str_val(JSONNode *node);
void gson_debug_print_num_val(JSONNode *node);
void gson_debug_general(Parser *parser, JSONNode *node);

#endif
