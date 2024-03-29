#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "gson.h"
#include "debug.h"

//#define DEBUG

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
    while ((scanner_peek(scanner) != '"') && !scanner_is_at_end(scanner))
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
    char debugstr[120];
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

static void gson_error_unopened(Parser *parser)
{
    switch (parser->current.type)
    {
        case TOKEN_RIGHT_BRACKET:
            gson_error(parser, "Unopened ']'");
            break;
        case TOKEN_RIGHT_BRACE:
            gson_error(parser, "Unopened '}'");
            break;
        default: break;
    }
}

static JSONNode* get_current_node(Parser *parser, JSONNode *curr, JSONType type)
{
    JSONNode *new = gson_node_create();
    new->type = type;
    new->depth = parser->depth;

    if (parser->has_next)
    {
        if (!curr->child)
        {
            gson_error(parser, "Error");
            return NULL;
        }

        curr = curr->child;
        while (curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = new;
        new->parent = curr;
        parser->has_next = false;
    }
    else
    {
        curr->child = new;
        new->parent = curr;
    }
    curr = new;
    return curr;
}

static JSONNode* gson_node_object(Parser *parser, JSONNode *curr)
{

    curr = get_current_node(parser, curr, JSON_OBJECT);

    if (curr->depth == parser->depth && parser->current.type != TOKEN_RIGHT_BRACE)
    {
        gson_parse(parser, curr);
        if (parser->current.type == TOKEN_EOF)
        {
            if (parser->had_error == true)
            {
                return curr;
            }
            gson_error(parser, "Unclosed '{'");
            return curr;
        }
    }
    return curr;
}

static JSONNode* gson_node_array(Parser *parser, JSONNode *curr)
{
    curr = get_current_node(parser, curr, JSON_ARRAY);

    if (curr->depth == parser->depth && parser->current.type != TOKEN_RIGHT_BRACKET)
    {
        gson_parse(parser, curr);
        if (parser->current.type == TOKEN_EOF)
        {
            if (parser->had_error == true)
            {
                return curr;
            }
            gson_error(parser, "Unclosed '['");
            return curr;
        }
    }
    return curr;
}

static JSONNode* gson_string(Parser *parser, JSONNode *curr)
{
    if (curr->type != JSON_STRING && parser->next_string_key == true)
    {
        curr = get_current_node(parser, curr, JSON_TEMP_STUB);
        curr->key = malloc(sizeof(char) * (parser->current.length + 1));
        memset(curr->key, 0, sizeof(char) * (parser->current.length + 1));
        strncpy(curr->key, parser->current.start, parser->current.length);
#ifdef DEBUG
        gson_debug_print_key(curr);
#endif
        parser->next_string_key = false;

        if (scanner_peek(parser->scanner) != ':')
        {
            parser->had_error = true;
            gson_error(parser, "Expecting ':' after key");
        }
        return curr;
    }
    else if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->str_val = malloc(sizeof(char) * (parser->current.length + 1));
        memset(curr->str_val, 0, sizeof(char) * (parser->current.length + 1));
        strncpy(curr->str_val, parser->current.start, parser->current.length);
#ifdef DEBUG
        gson_debug_print_str_val(curr);
#endif
        parser->next_string_key = true;
        return curr;
    }
    else if (curr->type == JSON_ARRAY && parser->next_string_key == false)
    {
        curr = get_current_node(parser, curr, JSON_STRING);
        curr->str_val = malloc((sizeof(char) * parser->current.length) + 1);
        memset(curr->str_val, 0, sizeof(char) * (parser->current.length + 1));
        strncpy(curr->str_val, parser->current.start, parser->current.length);
#ifdef DEBUG
        gson_debug_print_str_val(curr);
#endif
        parser->next_string_key = false;
        return curr;
    }
    gson_error(parser, "Error");
    return NULL;
}

JSONNode* gson_number(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_NUMBER;
    }
    else if (curr->type == JSON_ARRAY && parser->next_string_key == false)
    {
        curr = get_current_node(parser, curr, JSON_NUMBER);
        parser->next_string_key = false;
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
#ifdef DEBUG
    gson_debug_print_num_val(curr);
#endif
    return curr;
}

JSONNode* gson_true_val(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_TRUE_VAL;
    }
    else if (curr->type == JSON_ARRAY && parser->next_string_key == false)
    {
        curr = get_current_node(parser, curr, JSON_TRUE_VAL);
        parser->next_string_key = false;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    char *val = "true";
    curr->str_val = malloc(sizeof(char) * (strlen(val) + 1));
    strcpy(curr->str_val, val);
#ifdef DEBUG
    gson_debug_print_str_val(curr);
#endif
    return curr;
}
JSONNode* gson_false_val(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_FALSE_VAL;
    }
    else if (curr->type == JSON_ARRAY && parser->next_string_key == false)
    {
        curr = get_current_node(parser, curr, JSON_FALSE_VAL);
        parser->next_string_key = false;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    char *val = "false";
    curr->str_val = malloc(sizeof(char) * (strlen(val) + 1));
    strcpy(curr->str_val, val);
#ifdef DEBUG
    gson_debug_print_str_val(curr);
#endif
    return curr;
}

JSONNode* gson_null_val(Parser *parser, JSONNode *curr)
{
    if (curr->type == JSON_TEMP_STUB && parser->next_string_key == false)
    {
        curr->type = JSON_NULL_VAL;
    }
    else if (curr->type == JSON_ARRAY && parser->next_string_key == false)
    {
        curr = get_current_node(parser, curr, JSON_NULL_VAL);
        parser->next_string_key = false;
    }
    else
    {
        gson_error(parser, "Error");
        return NULL;
    }
    char *val = "null";
    curr->str_val = malloc(sizeof(char) * (strlen(val) + 1));
    strcpy(curr->str_val, val);
#ifdef DEBUG
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
    JSONNode *keyptr;


    while (parser->current.type != TOKEN_EOF)
    {
        parser_advance(parser);
        type = parser->current.type;

#ifdef DEBUG
        /* **************************DEBUG****************************** */
        int line = parser->current.line;
        const char *start = parser->current.start;
        int length = parser->current.length;
        int depth = parser->depth;
        int node_depth = curr->depth;

        char *debug_str = malloc(sizeof(char) * (length + 1));
        memset(debug_str, 0, sizeof(char) * (length) + 1);
        strncpy(debug_str, start, length);
        printf("DEBUG PARSER: type: %d, depth: %d, node depth: %d line: %d, value: %s, length: %d\n", type, depth, node_depth, line, debug_str, length);
        /* ************************************************************* */
#endif

        switch (type)
        {
            case TOKEN_LEFT_BRACE:
                {
                    parser->depth++;
                    parser->next_string_key = true;
                    gson_node_object(parser, curr);
                    break;
                }
            case TOKEN_RIGHT_BRACE:
                {
                    if (curr->depth != parser->depth || curr->type != JSON_OBJECT)
                    {
                        gson_error_unopened(parser);
                    }
                    if (parser->has_next)
                    {
                        gson_error(parser, "trailing ','");
                    }
                    parser->depth--;
                    return curr;
                }
            case TOKEN_LEFT_BRACKET:
                {
                    parser->depth++;
                    parser->next_string_key = false;
                    gson_node_array(parser, curr);
                    break;
                }
            case TOKEN_RIGHT_BRACKET:
                {
                    if (curr->depth != parser->depth || curr->type != JSON_ARRAY)
                    {
                        gson_error_unopened(parser);
                    }
                    if (parser->has_next)
                    {
                        gson_error(parser, "trailing ','");
                    }
                    parser->depth--;
                    return curr;
                }
            case TOKEN_COMMA:
                parser->has_next = true;
                if (curr->type == JSON_ARRAY)
                {
                    parser->next_string_key = false;
                }
                else
                {
                    parser->next_string_key = true;
                }
                break;
            case TOKEN_COLON:
                parser->next_string_key = false;
                break;
            case TOKEN_STRING:
                {
                    if (parser->next_string_key)
                    {
                        keyptr = gson_string(parser, curr);
                        break;
                    }
                    else if (curr->type == JSON_ARRAY)
                    {
                        gson_string(parser, curr);
                        break;
                    }
                    gson_string(parser, keyptr);
                    break;
                }
            case TOKEN_NUMBER:
                if (curr->type == JSON_ARRAY)
                {
                    gson_number(parser, curr);
                    break;
                }
                gson_number(parser, keyptr);
                break;
            case TOKEN_TRUE:
                if (curr->type == JSON_ARRAY)
                {
                    gson_true_val(parser, curr);
                    break;
                }
                gson_true_val(parser, keyptr);
                break;
            case TOKEN_FALSE:
                if (curr->type == JSON_ARRAY)
                {
                    gson_false_val(parser, curr);
                    break;
                }
                gson_false_val(parser, keyptr);
                break;
            case TOKEN_NULL_VAL:
                if (curr->type == JSON_ARRAY)
                {
                    gson_null_val(parser, curr);
                    break;
                }
                gson_null_val(parser, keyptr);
                break;
            default: break;
        }

        // TODO deallocate
        if (parser->had_error == true) return curr;

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
    t = clock() - t; 
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
    if (node->type == JSON_ROOT)
    {
            printf("parsing took %f seconds to execute \n", time_taken);
    }
    gson_debug_print_tree(node);
    parser_destroy(parser);
    free(source);

    return 0;
}
