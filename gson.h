#ifndef GSON_H_
#define GSON_H_

#include "gson_int.h"

extern Parser* parser_init(char* source);
extern void parser_destroy(Parser *parser);
extern JSONNode* gson_parse(Parser *parser, JSONNode *node);

#endif
