#ifndef GSON_H_
#define GSON_H_

typedef struct parser Parser;
typedef struct JSONNode JSONNode;

extern Parser* parser_init(char* source);
extern void parser_destroy(Parser *parser);
extern JSONNode* gson_parse(Parser *parser, JSONNode *node, int depth);

#endif
