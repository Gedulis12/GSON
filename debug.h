#ifndef GSON_DEBUG_H
#define GSON_DEBUG_H

#include "gson.h"

void gson_debug_print_tree(JSONNode *node);
void gson_debug_print_key(JSONNode *node);
void gson_debug_print_str_val(JSONNode *node);
void gson_debug_print_num_val(JSONNode *node);

#endif
