#ifndef GSON_H_
#define GSON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void Parser;
typedef void JSONNode;

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE_VAL,
    JSON_FALSE_VAL,
    JSON_NULL_VAL,
} JSONType;

/*
 * initialise parser, which will be passed in gson_parse() 
 */
extern Parser* parser_init(char* source);

/* destroy parser */
extern void parser_destroy(Parser *parser);

/*
 * takes parser and NULL as arguments, returns JSONNode type object pointing to the top level json object in the source
 */
extern JSONNode* gson_parse(Parser *parser, JSONNode *node);

/*
 * cleanup - destroys the tree of JSONObjects recursively starting from 'node'
 */
extern void gson_destroy(JSONNode *node);

/*
 * returns 'node' pointing to the first JSON object with matching 'key'
 */
extern JSONNode* gson_find_by_key(JSONNode *node, char *key);

/*
 * returns 'node' pointing to the first JSON object with matching 'value'
 */
extern JSONNode* gson_find_by_str_val(JSONNode *node, char *value);

/*
 * returns 'node' pointing to the first JSON object with matching 'value'
 */
extern JSONNode* gson_find_by_float_val(JSONNode *node, float value);

/*
 * returns the type of 'node'
 */
extern JSONType gson_get_object_type(JSONNode *node);

/*
 * returns key value 'node'
 */
extern char* gson_get_key_value(JSONNode *node);

/*
 * returns string value 'node'
 */
extern char* gson_get_str_value(JSONNode *node);

/*
 * returns numeric value 'node'
 */
extern float gson_get_float_value(JSONNode *node);

/*
 * returns JSON_FALSE_VAL, JSON_TRUE_VAL or JSON_NULL if the 'node' is of appropriate type
 */
extern JSONType gson_get_bool_value(JSONNode *node);

#ifdef __cplusplus
}
#endif

#endif // GSON_H
