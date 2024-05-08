#include "./include/gson.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

#if DEBUG==1
    gson_debug_print_tree(node, 0);
#endif
    parser_destroy(parser);
    gson_destroy(node);
    free(source);

    return 0;
}
