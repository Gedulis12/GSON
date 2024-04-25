#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <wchar.h>
#include "gson_int.h"
#if DEBUG==1
#include "debug.h"
#endif

static Scanner* scanner_init(const char *source)
{
    Scanner *scanner = (Scanner*)malloc(sizeof(Scanner));
    if (scanner == NULL)
    {
        perror("Memory allocation for scanner failed\n");
        exit(1);
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
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static Token token_error(Scanner *scanner, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
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

static bool match(char expected, Scanner *scanner)
{
    if (scanner_is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
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

//TODO: handle the escape sequences correctly
static Token token_string(Scanner *scanner)
{
    while ((scanner_peek(scanner) != '"' || (scanner_peek(scanner) == '"' && scanner->current[-1] == '\\')) && !scanner_is_at_end(scanner))
    {
        if (scanner_peek(scanner) == '\n') scanner->line++;
        scanner_advance(scanner);
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
    return token_error(scanner, "Unexpected Character.");
}

Parser* parser_init(char* source)
{
    Parser *parser = (Parser*)malloc(sizeof(Parser));
    if (parser == NULL)
    {
        perror("Memory allocation for parser failed\n");
        exit(1);
    }
    parser->scanner = scanner_init(source);
    parser->depth = 0;
    parser->had_error = false;
    parser->has_next = false;
    return parser;
}

void parser_destroy(Parser *parser)
{
    scanner_destroy(parser->scanner);
    free(parser);
}

static void parser_advance(Parser *parser)
{
    parser->previous = parser->current;
    Scanner *scanner = parser->scanner;

    for (;;)
    {
        parser->current = scan_token(scanner);
        if (parser->current.type != TOKEN_ERROR) break;
        char *debug_str = malloc((sizeof(char) * parser->current.length) + 1);
        strncpy(debug_str, parser->current.start, parser->current.length);
        printf("Error at line %i: %s\n", parser->current.line, debug_str);
        free(debug_str);
    }
}

static JSONNode* gson_node_create()
{
    JSONNode *node = (JSONNode*)malloc(sizeof(JSONNode));
    node->child = NULL;
    node->parent = NULL;
    node->next = NULL;
    node->key = NULL;
    node->str_val = NULL;
    return node;
}

static void gson_error(Parser *parser, char *message)
{
    printf("%s at line %i\n", message, parser->current.line);
    parser->had_error = true;
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

    gson_parse(parser, curr);

    if (parser->had_error) return curr;

    if (parser->current.type == TOKEN_RIGHT_BRACKET)
    {
        if (curr->type == JSON_OBJECT)
        {
            gson_error(parser, "1error");
        }
        if (curr->type == JSON_ARRAY)
        {
            gson_error(parser, "2error");
        }
        return curr;
    }

    if (parser->current.type == TOKEN_EOF)
    {
        gson_error(parser, "3error");
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

    gson_parse(parser, curr);

    if (parser->had_error) return curr;

    if (parser->current.type == TOKEN_RIGHT_BRACE)
    {
        if (curr->type == JSON_OBJECT)
        {
            gson_error(parser, "4error");
        }
        if (curr->type == JSON_OBJECT)
        {
            gson_error(parser, "5error");
        }
        return curr;
    }

    if (parser->current.type == TOKEN_EOF)
    {
        gson_error(parser, "6error");
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

static void gson_string_substitute(char *check, char subst, int curr, int *key_len)
{
    check[curr+1] = subst;
    memmove(&check[curr], &check[curr+1], (*key_len - 1 - curr));
    *key_len = *key_len - 1;
    check[*key_len] = '\0';
    if (realloc(check, sizeof(char)*(*key_len+1)) == NULL)
    {
        printf("realloc failed\n");
    }
}


static void gson_string_validate(Parser *parser, char *check)
{
    int key_len = strlen(check);
    int *key_len_p = &key_len;
    for (int i = 0; i < key_len; i++)
    {
        if (check[i] == '\\')
        {
            if (i == key_len - 2) // at the last character of the string before the closing quote
            {
                gson_error(parser, "Error: closing quote of the string cannot be escaped");
            }

            switch(check[i+1])
            {
                case '\\':
                    gson_string_substitute(check, '\\', i, key_len_p);
                    break;
                case '/':
                    gson_string_substitute(check, '/', i, key_len_p);
                    break;
                case '"':
                    gson_string_substitute(check, '"', i, key_len_p);
                    break;
                case 'b':
                    gson_string_substitute(check, '\b', i, key_len_p);
                    break;
                case 'f':
                    gson_string_substitute(check, '\f', i, key_len_p);
                    break;
                case 'n':
                    gson_string_substitute(check, '\n', i, key_len_p);
                    break;
                case 'r':
                    gson_string_substitute(check, '\r', i, key_len_p);
                    break;
                case 't':
                    gson_string_substitute(check, '\r', i, key_len_p);
                    break;
                case 'u':
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            if (!is_hex(check[i+2+j]))
                                gson_error(parser, "wrong unicode escape code");
                        }
                        char unicode[5];
                        strncpy(unicode, check + i + 2, 4);
                        unicode[4] = '\0';
                        int codepoint = (int)strtol(unicode, NULL, 16);
                        if ((codepoint >> 13) == 6) // 00000110
                        {
                            char b1 = (codepoint >> 8); // first byte of codepoint
                            char b2 = (codepoint << 8) >> 8; // second byte of codepoint
                            check[i] = b1;
                            check[i+1] = b2;
                            memmove(&check[i+2], &check[i+6], (key_len - 4 - i));
                            key_len = key_len - 4;
                            check[key_len] = '\0';
                            if (realloc(check, sizeof(char)*(key_len+1)) == NULL)
                            {
                                printf("realloc failed\n");
                            }

                        }
                        else
                        {
                            check[i] = (char)codepoint;
                            memmove(&check[i+1], &check[i+6], (key_len - 5 - i));
                            key_len = key_len - 5;
                            check[key_len] = '\0';
                            if (realloc(check, sizeof(char)*(key_len+1)) == NULL)
                            {
                                printf("realloc failed\n");
                            }
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
        new->key = malloc(sizeof(char) * (parser->current.length + 1));
        memset(new->key, 0, sizeof(char) * (parser->current.length + 1));
        strncpy(new->key, parser->current.start, parser->current.length);
        gson_string_validate(parser, new->key);
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
        curr->str_val = malloc(sizeof(char) * (parser->current.length + 1));
        memset(curr->str_val, 0, sizeof(char) * (parser->current.length + 1));
        strncpy(curr->str_val, parser->current.start, parser->current.length);
        gson_string_validate(parser, curr->str_val);
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
        new->str_val = malloc(sizeof(char) * (parser->current.length + 1));
        memset(new->str_val, 0, sizeof(char) * (parser->current.length + 1));
        strncpy(new->str_val, parser->current.start, parser->current.length);
        gson_string_validate(parser, new->str_val);
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
    char *num_str_val = malloc((sizeof(char) * parser->current.length) + 1);
    memset(num_str_val, 0, sizeof(char) * (parser->current.length + 1));
    strncat(num_str_val, parser->current.start, parser->current.length);
    float num_val = strtof(num_str_val, NULL);
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
    char *val = "true";
    curr->str_val = malloc(sizeof(char) * (strlen(val) + 1));
    strcpy(curr->str_val, val);
#if DEBUG==1
    gson_debug_print_str_val(curr);
#endif
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
    char *val = "false";
    curr->str_val = malloc(sizeof(char) * (strlen(val) + 1));
    strcpy(curr->str_val, val);
#if DEBUG==1
    gson_debug_print_str_val(curr);
#endif
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
    char *val = "null";
    curr->str_val = malloc(sizeof(char) * (strlen(val) + 1));
    strcpy(curr->str_val, val);
#if DEBUG==1
    gson_debug_print_str_val(curr);
#endif
    return curr;
}

JSONNode* gson_parse(Parser *parser, JSONNode *curr)
{
    if (curr == NULL)
    {
        curr = gson_node_create(); // root node
        curr->type = JSON_ROOT;
        curr->depth = 0;
    }
    TokenType type;

    while (parser->current.type != TOKEN_EOF)
    {
        parser_advance(parser);
        type = parser->current.type;
        if (parser->had_error) return NULL;

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
                            gson_error(parser, "7error");
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
                            gson_error(parser, "8error");
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
                        gson_error(parser, "9error");
                    }
                    curr = gson_string(parser, curr);
                    break;
                }
            case TOKEN_NUMBER:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "10error");
                }
                curr = gson_number(parser, curr);
                break;
            case TOKEN_TRUE:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "11error");
                }
                curr = gson_true_val(parser, curr);
                break;
            case TOKEN_FALSE:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "12error");
                }
                curr = gson_false_val(parser, curr);
                break;
            case TOKEN_NULL_VAL:
                if (curr->type == JSON_ROOT)
                {
                    gson_error(parser, "13error");
                }
                curr = gson_null_val(parser, curr);
                break;
            default: break;
        }
        if (parser->had_error) return NULL;
    }
    return curr;
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./gson <input.json>");
        return 1;
    }

    char *source = NULL;
    FILE *fp = fopen(argv[1], "r");
    if (fp != NULL)
    {
        if (fseek(fp, 0L, SEEK_END) == 0)
        {
            long bufsize = ftell(fp);
            if (bufsize == -1)
            {
                return 1;
            }

            source = malloc(sizeof(char) * (bufsize + 1));
            if (source == NULL)
            {
                perror("Memory allocation failed");
                return 1;
            }
            if (fseek(fp, 0L, SEEK_SET) != 0)
            {
                return 1;
            }

            size_t new_len = fread(source, sizeof(char), bufsize, fp);
            if (ferror(fp) != 0)
            {
                fputs("Error reading file", stderr);
            }
            else
            {
                source[new_len++] = '\0';
            }
        }
        fclose(fp);
    }


    Parser *parser = parser_init(source);
    clock_t t;
    t = clock();
    JSONNode *node = gson_parse(parser, NULL);
    if (node == NULL)
    {
        return 1; // failed to parse
    }
    t = clock() - t; 
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
    printf("parsing took %f seconds to execute \n", time_taken);
#if DEBUG==1
    gson_debug_print_tree(node, 0);
#endif
    parser_destroy(parser);
    free(source);

    return 0;
}
