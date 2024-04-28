#include "./include/gson.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

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
    JSONNode *key = gson_find_by_key(node, "\"gender\"");
    if (key != NULL)
        printf("Found!\n");
#if DEBUG==1
    gson_debug_print_tree(node, 0);
#endif
    parser_destroy(parser);
    gson_destroy(node);
    free(source);

    return 0;
}
