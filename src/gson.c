#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gson_int.h"
#if DEBUG==1
#include "debug.h"
#endif

static JSONNode* _gson_parse(Parser *parser, JSONNode *curr);

static Scanner* scanner_init(const char *source)
{
    Scanner *scanner = (Scanner*)malloc(sizeof(Scanner));
    if (scanner == NULL)
    {
        perror("Memory allocation for scanner failed\n");
        return NULL;
    }
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    return scanner;
}

static void scanner_destroy(Scanner *scanner)
{
    scanner->current = NULL;
    scanner->start = NULL;
    free(scanner);
}

static Token token_make(TokenType type, Scanner *scanner)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (uint32_t)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static Token token_error(Scanner *scanner, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (uint32_t)strlen(message);
    token.line = scanner->line;
    return token;
}

static bool scanner_is_at_end(Scanner *scanner)
{
    return *scanner->current == '\0';
}

static char scanner_peek(Scanner *scanner)
{
    return *scanner->current;
}

static char scanner_peek_next(Scanner *scanner)
{
    if (scanner_is_at_end(scanner)) return '\0';
    return scanner->current[1];
}

static char scanner_advance(Scanner *scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

static void scanner_skip_white_space(Scanner *scanner)
{
    for (;;)
    {
        char c = scanner_peek(scanner);
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
                scanner_advance(scanner);
                break;
            case '\n':
                scanner->line++;
                scanner_advance(scanner);
                break;
            default:
                return;
        }
    }
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_hex(char c)
{
    return (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F') ||
           (c >= '0' && c <= '9');
}

static Token token_string(Scanner *scanner)
{

    while ((scanner_peek(scanner) != '"' || (scanner_peek(scanner) == '"' && scanner->current[-1] == '\\')) && !scanner_is_at_end(scanner))
    {
        if (scanner_peek(scanner) == '\n') scanner->line++;

        if ((scanner_peek(scanner) & 0x80) == 0) // if not 10000000
        {
            scanner_advance(scanner);
        }
        // order of checking is important as checking if char & 0xc0 will also be true for 0xe0 and 0xf0
        else if ((scanner_peek(scanner) & 0xf0) == 0xf0 && (scanner_peek_next(scanner) & 0xc0) == 0x80) // if 11110000 10000000
        {
            scanner_advance(scanner);
            scanner_advance(scanner);
            if ((scanner_peek(scanner) & 0xc0) == 0x80 && (scanner_peek_next(scanner) & 0xc0) == 0x80) // if 10000000 10000000
            {
                scanner_advance(scanner);
                scanner_advance(scanner);
            }
        }
        else if ((scanner_peek(scanner) & 0xe0) == 0xe0 && (scanner_peek_next(scanner) & 0xc0) == 0x80) // if 11100000 10000000
        {
            scanner_advance(scanner);
            if ((scanner_peek_next(scanner) & 0xc0) == 0x80) // if 10000000
            {
                scanner_advance(scanner);
                scanner_advance(scanner);
            }
        }
        else if ((scanner_peek(scanner) & 0xc0) == 0xc0 && (scanner_peek_next(scanner) & 0xc0) == 0x80) // if 11000000 10000000
        {
            scanner_advance(scanner);
            scanner_advance(scanner);
        }
        else
        {
            return token_error(scanner, "Invalid UTF-8 in string");
        }
    }

    if (scanner_is_at_end(scanner)) return token_error(scanner, "Unterminated string");
    scanner_advance(scanner);
    return token_make(TOKEN_STRING, scanner);
}

static Token token_number(Scanner *scanner)
{
    while (is_digit(scanner_peek(scanner))) scanner_advance(scanner);

    if (scanner_peek(scanner) == '.' && is_digit(scanner_peek_next(scanner)))
    {
        scanner_advance(scanner);
        while (is_digit(scanner_peek(scanner))) scanner_advance(scanner);
    }

    return token_make(TOKEN_NUMBER, scanner);
}

static Token token_boolean(Scanner *scanner)
{
    const char *true_str = "true";
    const char *false_str = "false";

    if (strncmp(true_str, scanner->current - 1, strlen(true_str)) == 0)
    {
        while (scanner_peek(scanner) != 'e') scanner_advance(scanner);
        scanner_advance(scanner);
        return token_make(TOKEN_TRUE, scanner);
    }
    else if (strncmp(false_str, scanner->current - 1, strlen(false_str)) == 0)
    {
        while (scanner_peek(scanner) != 'e') scanner_advance(scanner);
        scanner_advance(scanner);
        return token_make(TOKEN_FALSE, scanner);
    }

    return token_error(scanner, "Only 'true' and 'false' are valid boolean keywords");
}

static Token token_null(Scanner *scanner)
{
    const char *null_str = "null";
    if (strncmp(null_str, scanner->current - 1, strlen(null_str)) == 0)
    {
        while (scanner_peek(scanner) != 'l') scanner_advance(scanner);
        scanner_advance(scanner);
        scanner_advance(scanner);
        return token_make(TOKEN_NULL_VAL, scanner);
    }

    return token_error(scanner, "Only 'null' is a valid keyword for the null value");
}

static Token scan_token(Scanner *scanner)
{
    scanner_skip_white_space(scanner);
    scanner->start = scanner->current;

    if (scanner_is_at_end(scanner)) return token_make(TOKEN_EOF, scanner);

    char c = scanner_advance(scanner);
    if (is_digit(c)) return token_number(scanner);

    switch(c)
    {
        case '{': return token_make(TOKEN_LEFT_BRACE, scanner);
        case '}': return token_make(TOKEN_RIGHT_BRACE, scanner);
        case '[': return token_make(TOKEN_LEFT_BRACKET, scanner);
        case ']': return token_make(TOKEN_RIGHT_BRACKET, scanner);
        case ',': return token_make(TOKEN_COMMA, scanner);
        case ':': return token_make(TOKEN_COLON, scanner);
        case '"': return token_string(scanner);
        case 't': return token_boolean(scanner);
        case 'f': return token_boolean(scanner);
        case 'n': return token_null(scanner);
    }
    if (c == '-' && is_digit(scanner_peek(scanner))) return token_number(scanner);
    return token_error(scanner, "Unexpected Character.");
}

Parser* parser_init(char* source)
{
    Parser *parser = (Parser*)malloc(sizeof(Parser));
    if (parser == NULL)
    {
        perror("Memory allocation for parser failed\n");
        return NULL;
    }
    parser->scanner = scanner_init(source);
    parser->depth = 0;
    parser->had_error = false;
    parser->has_next = false;
    parser->current.type = TOKEN_INIT;
    return parser;
}


void parser_destroy(Parser *parser)
{
    scanner_destroy(parser->scanner);
    free(parser);
}

static JSONNode* gson_root_node(JSONNode *node) // given any node of the gson tree returns a root node
{
    if (!node) return NULL;
    JSONNode *root = node;

    while (root->parent && root->type != JSON_ROOT)
    {
        root = root->parent;
    }
    return root;
}

static void gson_node_free(JSONNode *node)
{
    if (!node) return;

    if (node->key != NULL)
        free(node->key);

    if (node->str_val != NULL)
        free(node->str_val);

    free(node);
    node = NULL;
}

static void _gson_destroy(JSONNode *node)
{
    if (!node) return;

    _gson_destroy(node->child);
    _gson_destroy(node->next);
    gson_node_free(node);
}

void gson_destroy(JSONNode *node)
{
    if (!node) return;

    JSONNode *root = gson_root_node(node);
    if (!root) return;

    _gson_destroy(root);
}


static void gson_error(Parser *parser, char *message)
{
    fprintf(stderr, "%s at line %i\n", message, parser->current.line);
    parser->had_error = true;
}

static void parser_advance(Parser *parser)
{
    parser->previous = parser->current;
    Scanner *scanner = parser->scanner;

    for (;;)
    {
        parser->current = scan_token(scanner);
        if (parser->current.type != TOKEN_ERROR) break;
        size_t length = parser->current.length;
        char *debug_str = malloc(sizeof(char) * (length + 1));
        if (!debug_str)
        {
            perror("Memory allocation failed\n");
            break;
        }
        debug_str[length] = '\0';
        memcpy(debug_str, parser->current.start, length);
        parser->had_error = true;
        gson_error(parser, debug_str);
        free(debug_str);
        break;
    }
}

static JSONNode* gson_node_create()
{
    JSONNode *node = (JSONNode*)malloc(sizeof(JSONNode));
    if (!node)
    {
        return NULL;
    }
    node->child = NULL;
    node->parent = NULL;
    node->next = NULL;
    node->key = NULL;
    node->str_val = NULL;
    return node;
}

static JSONNode* gson_node_object(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB)
    {
        curr->type = JSON_OBJECT;
        curr->depth = parser->depth;
    }
    else
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }
        new->type = JSON_OBJECT;
        new->depth = parser->depth;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        curr = new;
    }

    _gson_parse(parser, curr);

    if (parser->had_error)
        return curr;

    if (parser->current.type == TOKEN_RIGHT_BRACKET)
    {
        if (curr->type == JSON_OBJECT)
        {
            gson_error(parser, "Error");
        }
        if (curr->type == JSON_ARRAY)
        {
            gson_error(parser, "Error");
        }
        return curr;
    }

    if (parser->current.type == TOKEN_EOF)
    {
        gson_error(parser, "Error");
        return curr;
    }

    if (parser->has_next)
    {
        gson_error(parser, "trailing ','");
        return curr;
    }

    parser->depth--;
    parser->next_string_key = true;
    return curr;
}

static JSONNode* gson_node_array(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB)
    {
        curr->type = JSON_ARRAY;
        curr->depth = parser->depth;
    }
    else
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_ARRAY;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        curr = new;
    }

    _gson_parse(parser, curr);

    if (parser->had_error)
        return curr;

    if (parser->current.type == TOKEN_RIGHT_BRACE)
    {
        if (curr->type == JSON_OBJECT)
        {
            gson_error(parser, "Error");
        }
        if (curr->type == JSON_OBJECT)
        {
            gson_error(parser, "Error");
        }
        return curr;
    }

    if (parser->current.type == TOKEN_EOF)
    {
        gson_error(parser, "Error");
        return curr;
    }

    if (parser->has_next)
    {
        gson_error(parser, "trailing ','");
        return curr;
    }

    parser->depth--;
    parser->next_string_key = true;
    return curr;
}

static void gson_string_substitute(Parser *parser, char **check, char subst, uint32_t curr, uint32_t *key_len)
{
    (*check)[curr+1] = subst;
    memmove(&(*check)[curr], &(*check)[curr+1], (*key_len - 1 - curr));
    *key_len = *key_len - 1;
    (*check)[*key_len] = '\0';
    char *tmp = realloc(*check, sizeof(char)*(*key_len+1));
    if (!tmp)
    {
        gson_error(parser, "memory allocation failed on escape sequence substitution");
    }
    *check = tmp;
}


static void gson_string_validate(Parser *parser, char **check)
{
    uint32_t len = strlen(*check);
    uint32_t *len_p = &len;
    for (size_t i = 0; i < len; i++)
    {
        if ((*check)[i] == '\\')
        {
            if (i == len - 2) // at the last character of the string before the closing quote
            {
                gson_error(parser, "Error: closing quote of the string cannot be escaped");
            }

            switch((*check)[i+1])
            {
                case '\\':
                    gson_string_substitute(parser, check, '\\', i, len_p);
                    break;
                case '/':
                    gson_string_substitute(parser, check, '/', i, len_p);
                    break;
                case '"':
                    gson_string_substitute(parser, check, '"', i, len_p);
                    break;
                case 'b':
                    gson_string_substitute(parser, check, '\b', i, len_p);
                    break;
                case 'f':
                    gson_string_substitute(parser, check, '\f', i, len_p);
                    break;
                case 'n':
                    gson_string_substitute(parser, check, '\n', i, len_p);
                    break;
                case 'r':
                    gson_string_substitute(parser, check, '\r', i, len_p);
                    break;
                case 't':
                    gson_string_substitute(parser, check, '\r', i, len_p);
                    break;
                case 'u':
                    {
                        for (uint8_t j = 0; j < 4; j++)
                        {
                            if (!is_hex((*check)[i+2+j]))
                                gson_error(parser, "wrong unicode escape code");
                        }
                        char unicode[5];
                        strncpy(unicode, (*check) + i + 2, 4);
                        unicode[4] = '\0';
                        int32_t codepoint = (int32_t)strtol(unicode, NULL, 16);
                        if ((codepoint >> 13) == 6) // 00000110
                        {
                            char b1 = (codepoint >> 8); // first byte of codepoint
                            char b2 = (codepoint << 8) >> 8; // second byte of codepoint
                            (*check)[i] = b1;
                            (*check)[i+1] = b2;
                            memmove(&(*check)[i+2], &(*check)[i+6], (len - 4 - i));
                            len = len - 4;
                            (*check)[len] = '\0';
                            char *tmp = realloc(*check, sizeof(char)*(len+1));
                            if (!tmp)
                            {
                                gson_error(parser, "memory allocation failed on escape sequence substitution");
                            }
                            *check = tmp;

                        }
                        else
                        {
                            (*check)[i] = (char)codepoint;
                            memmove(&(*check)[i+1], &(*check)[i+6], (len - 5 - i));
                            len = len - 5;
                            (*check)[len] = '\0';
                            char *tmp = realloc(*check, sizeof(char)*(len+1));
                            if (!tmp)
                            {
                                gson_error(parser, "memory allocation failed on escape sequence substitution");
                            }
                            *check = tmp;
                        }
                        break;
                    }
                default:
                    gson_error(parser, "Invalid escape");
                    break;
            }
        }
    }
}

static JSONNode* gson_string(Parser *parser, JSONNode *curr)
{
    if (parser->next_string_key == true)
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_TEMP_STUB;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        size_t length = parser->current.length;
        new->key = malloc(sizeof(char) * (length + 1));
        if (!new->key)
        {
            gson_error(parser, "Memory allocation failed\n");
            return curr;
        }
        new->key[length] = '\0';
        memcpy(new->key, parser->current.start, length);
        gson_string_validate(parser, &(new->key));
        curr = new;
#if DEBUG==1
        gson_debug_print_key(curr);
#endif
        parser->next_string_key = false;

        if (scanner_peek(parser->scanner) != ':')
        {
            parser->had_error = true;
            gson_error(parser, "Expecting ':' after key");
        }
    }
    else if (parser->next_string_key == false && curr->type == JSON_TEMP_STUB && !parser->has_next)
    {
        curr->type = JSON_STRING;
        size_t length = parser->current.length;
        curr->str_val = malloc(sizeof(char) * (length + 1));
        if (!curr->str_val)
        {
            gson_error(parser, "Memory allocation failed\n");
            return curr;
        }
        curr->str_val[length] = '\0';
        memcpy(curr->str_val, parser->current.start, length);
        gson_string_validate(parser, &(curr->str_val));
#if DEBUG==1
        gson_debug_print_str_val(curr);
#endif
        if (curr->parent->type != JSON_ARRAY)
        {
            parser->next_string_key = true;
        }
    }
    else if (parser->next_string_key == false && (parser->has_next || curr->type == JSON_ARRAY)) // in array
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_STRING;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        size_t length = parser->current.length;
        new->str_val = malloc(sizeof(char) * (length + 1));
        if (!new->str_val)
        {
            gson_error(parser, "Memory allocation failed\n");
            return new;
        }
        new->str_val[length] = '\0';
        memcpy(new->str_val, parser->current.start, length);
        gson_string_validate(parser, &(new->str_val));
        curr = new;
#if DEBUG==1
        gson_debug_print_str_val(curr);
#endif
    } else
    {
        gson_error(parser, "Error");
    }
    return curr;
}

JSONNode* gson_number(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_NUMBER;
        if (curr->parent->type != JSON_ARRAY)
        {
            parser->next_string_key = true;
        }
    }
    else if (parser->next_string_key == false && (parser->has_next || curr->type == JSON_ARRAY)) // in array
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_NUMBER;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        curr = new;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    size_t length = parser->current.length;
    char *num_str_val = malloc(sizeof(char) * (length + 1));
    if (!num_str_val)
    {
        gson_error(parser, "Memory allocation failed\n");
        return curr;
    }
    num_str_val[length] = '\0';
    memcpy(num_str_val, parser->current.start, length);
    float num_val = strtof(num_str_val, NULL);
    free(num_str_val);
    curr->num_val = num_val;

    curr->num_val = num_val;
#if DEBUG==1
    gson_debug_print_num_val(curr);
#endif
    return curr;
}

JSONNode* gson_true_val(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_TRUE_VAL;
        if (curr->parent->type != JSON_ARRAY)
        {
            parser->next_string_key = true;
        }
    }
    else if (parser->next_string_key == false && (parser->has_next || curr->type == JSON_ARRAY)) // in array
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_TRUE_VAL;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        curr = new;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    return curr;
}
JSONNode* gson_false_val(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_FALSE_VAL;
        if (curr->parent->type != JSON_ARRAY)
        {
            parser->next_string_key = true;
        }
    }
    else if (parser->next_string_key == false && (parser->has_next || curr->type == JSON_ARRAY)) // in array
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_FALSE_VAL;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        curr = new;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    return curr;
}

JSONNode* gson_null_val(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_NULL_VAL;
        if (curr->parent->type != JSON_ARRAY)
        {
            parser->next_string_key = true;
        }
    }
    else if (parser->next_string_key == false && (parser->has_next || curr->type == JSON_ARRAY)) // in array
    {
        JSONNode *new = gson_node_create();
        if (new == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return curr;
        }

        new->type = JSON_NULL_VAL;
        new->depth = parser->depth;
        new->parent = curr;
        if (parser->has_next)
        {
            new->parent = curr->parent;
            curr->next = new;
            parser->has_next = false;
        }
        else
        {
            new->parent = curr;
            curr->child = new;
        }
        curr = new;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    return curr;
}

JSONNode* gson_parse(Parser *parser, JSONNode *curr)
{
    curr = _gson_parse(parser, curr);

    if (parser->had_error)
    {
        gson_destroy(curr);
        curr = NULL;
        return NULL;
    }

    return curr;

}

static JSONNode* _gson_parse(Parser *parser, JSONNode *curr)
{
    if (curr == NULL)
    {
        curr = gson_node_create(); // root node
        if (curr == NULL)
        {
            gson_error(parser, "Memory allocation failed");
            return NULL;
        }
        curr->type = JSON_ROOT;
        curr->depth = 0;
    }
    TokenType type;

    while (parser->current.type != TOKEN_EOF)
    {
        parser_advance(parser);
        type = parser->current.type;
        if (parser->had_error)
            break;

        switch (type)
        {
            case TOKEN_LEFT_BRACE:
                {
#if DEBUG==1
gson_debug_general(parser, curr);
#endif
                    parser->depth++;
                    parser->next_string_key = true;
                    curr = gson_node_object(parser, curr);
                    break;
                }
            case TOKEN_RIGHT_BRACE:
                {
#if DEBUG==1
gson_debug_general(parser, curr);
#endif
                    if ((parser->previous.type == TOKEN_RIGHT_BRACE && curr->type == JSON_OBJECT) || (parser->previous.type == TOKEN_RIGHT_BRACKET && curr->type == JSON_ARRAY))
                    {
                        curr = curr->parent;
                    }


                    if (curr->depth != parser->depth || curr->type == JSON_ROOT || curr->type == JSON_ARRAY)
                    {
                            gson_error(parser, "Error");
                    }

                    if (parser->has_next)
                    {
                        gson_error(parser, "trailing ','");
                    }
                    return curr;
                }
            case TOKEN_LEFT_BRACKET:
                {
#if DEBUG==1
gson_debug_general(parser, curr);
#endif
                    parser->depth++;
                    parser->next_string_key = false;
                    curr = gson_node_array(parser, curr);
                    break;
                }
            case TOKEN_RIGHT_BRACKET:
                {
#if DEBUG==1
gson_debug_general(parser, curr);
#endif
                    if ((parser->previous.type == TOKEN_RIGHT_BRACE && curr->type == JSON_OBJECT) || (parser->previous.type == TOKEN_RIGHT_BRACKET && curr->type == JSON_ARRAY))
                    {
                        curr = curr->parent;
                    }

                    if (curr->depth != parser->depth || curr->type == JSON_ROOT || curr->type == JSON_OBJECT)
                    {
                            gson_error(parser, "Error");
                    }

                    if (parser->has_next)
                    {
                        gson_error(parser, "trailing ','");
                    }
                    return curr;
                }
            case TOKEN_COMMA:
                parser->has_next = true;
                break;
            case TOKEN_COLON:
                parser->next_string_key = false;
                break;
            case TOKEN_STRING:
                {
                    if (curr->type == JSON_ROOT)
                    {
                        gson_error(parser, "Error");
                    }
                    curr = gson_string(parser, curr);
                    break;
                }
            case TOKEN_NUMBER:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "Error");
                }
                curr = gson_number(parser, curr);
                break;
            case TOKEN_TRUE:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "Error");
                }
                curr = gson_true_val(parser, curr);
                break;
            case TOKEN_FALSE:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "Error");
                }
                curr = gson_false_val(parser, curr);
                break;
            case TOKEN_NULL_VAL:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "Error");
                }
                curr = gson_null_val(parser, curr);
                break;
            default: break;
        }
        if (parser->had_error)
            break;
    }
    return curr;
}

JSONNode* gson_find_by_key(JSONNode *node, char *key)
{
    JSONNode *orig = node;
    while (node != NULL)
    {
        if (node->key && strcmp(key, node->key) == 0)
        {
            if (node != orig)
            {
                return node;
            }
        }
        if (node->child)
        {
            node = node->child;
        }
        else
        {
            while (node != NULL && node->next == NULL)
            {
                node = node->parent;
            }
            if (node != NULL)
            {
                node = node->next;
            }
        }
    }
    return NULL;
}

JSONNode* gson_find_by_str_val(JSONNode *node, char *value)
{
    JSONNode *orig = node;
    while (node != NULL)
    {
        if (node->str_val
                && strcmp(value, node->str_val) == 0
                && node->type == JSON_STRING)
        {
            if (node != orig)
            {
                return node;
            }
        }
        if (node->child)
        {
            node = node->child;
        }
        else
        {
            while (node != NULL && node->next == NULL)
            {
                node = node->parent;
            }
            if (node != NULL)
            {
                node = node->next;
            }
        }
    }
    return NULL;
}

JSONNode* gson_find_by_float_val(JSONNode *node, float value)
{
    JSONNode *orig = node;
    while (node != NULL)
    {
        if (node->num_val && value == node->num_val && node->type == JSON_NUMBER)
        {
            if (node != orig)
            {
                return node;
            }
        }
        if (node->child)
        {
            node = node->child;
        }
        else
        {
            while (node != NULL && node->next == NULL)
            {
                node = node->parent;
            }
            if (node != NULL)
            {
                node = node->next;
            }
        }
    }
    return NULL;
}

JSONType gson_get_object_type(JSONNode *node)
{
    if (node != NULL) return node->type;

}

char* gson_get_key_value(JSONNode *node)
{
    if (node && node->key)
    {
        return node->key;
    }
    return NULL;
}

char* gson_get_str_value(JSONNode *node)
{
    if (node && node->str_val && node->type == JSON_STRING)
    {
        return node->str_val;
    }
    return NULL;
}

float gson_get_float_value(JSONNode *node)
{
    if (node && node->num_val && node->type == JSON_NUMBER)
    {
        return node->num_val;
    }
}

JSONType gson_get_bool_value(JSONNode *node)
{
    if (node->type == JSON_TRUE_VAL
            || node->type == JSON_FALSE_VAL
            || node->type == JSON_NULL_VAL)
    {
        return node->type;
    }
}
