#include "./include/gson.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define PARSING_TIMER(source) \
    do { \
        clock_t t; \
        Parser *__p = parser_init(source); \
        t = clock(); \
        JSONNode *__n = gson_parse(__p, NULL); \
        t = clock() - t; \
        double time_taken = ((double)t)/CLOCKS_PER_SEC; \
        if (__n != NULL) \
        { \
                printf("parsing took %f seconds to execute \n", time_taken); \
        } \
        gson_destroy(__n); \
        parser_destroy(__p); \
        break; \
    } while (1); \

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


    PARSING_TIMER(source);
    Parser *parser = parser_init(source);
    JSONNode *node = gson_parse(parser, NULL);
    if (node == NULL)
    {
        return 1; // failed to parse
    }
    // SOME TESTING
    JSONNode *key = gson_find_by_key(node, "gender");
    if (key != NULL)
        printf("1Found %s:%s\n", gson_get_key_value(key), gson_get_str_value(key));

    JSONNode *val = gson_find_by_str_val(node, "male");
    if (val != NULL)
        printf("2Found: %s\n", gson_get_str_value(val));

    JSONNode *_float = gson_find_by_float_val(node, 30);
    if (_float != NULL)
        printf("3Found: %s:%f\n", gson_get_key_value(_float), gson_get_float_value(_float));


    if (_float != NULL)
    {
        JSONType _float_type = gson_get_object_type(_float);
        if (_float_type == JSON_NUMBER)
        {
            printf("4correct type found, JSON_NUMBER\n");
        }
    }

    JSONNode *_bool = gson_find_by_key(node, "has_kids");
    if (_bool != NULL)
    {
        JSONType bool_type = gson_get_object_type(_bool);
        if (bool_type == JSON_TRUE_VAL)
        {
            printf("5correct type found, JSON_TRUE_VAL\n");
        }
    }

#if DEBUG==1
    gson_debug_print_tree(node, 0);
#endif
    parser_destroy(parser);
    gson_destroy(node);
    free(source);

    return 0;
}
