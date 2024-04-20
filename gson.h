#ifndef GSON_H_
#define GSON_H_

typedef void Parser;
typedef void JSONNode;

extern Parser* parser_init(char* source);
extern void parser_destroy(Parser *parser);
extern JSONNode* gson_parse(Parser *parser, JSONNode *node);

#endif
