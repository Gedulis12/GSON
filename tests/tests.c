#include "../src/gson_int.h"
#include <stdio.h>
#include <stdlib.h>

static char* load_source_file(char* filename)
{
    char *source = NULL;
    FILE *fp = fopen(filename, "r");
    if (fp != NULL)
    {
        if (fseek(fp, 0L, SEEK_END) == 0)
        {
            long bufsize = ftell(fp);
            if (bufsize == -1)
            {
                return NULL;
            }

            source = malloc(sizeof(char) * (bufsize + 1));
            if (source == NULL)
            {
                perror("Memory allocation failed");
                return NULL;
            }
            if (fseek(fp, 0L, SEEK_SET) != 0)
            {
                return NULL;
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
    return source;
}

static JSONNode* parse_single_file(char *filename)
{
    char *source = load_source_file(filename);
    Parser *parser = parser_init(source);
    JSONNode *node = gson_parse(parser, NULL);
    if (node == NULL)
    {
        parser_destroy(parser);
        return NULL; // failed to parse
    }
    parser_destroy(parser);
    free(source);
    return node;
}

int main()
{
    int total, passed, current = 0;
    char *test_files_should_pass[] = {
        "./tests/mock_data/1_simple_json.json",
        "./tests/mock_data/2_empty_brackets.json",
        "./tests/mock_data/3_empty_braces.json",
        "./tests/mock_data/4_empty_brackets_nested.json",
        "./tests/mock_data/5_utf-8.json",
        "./tests/mock_data/6_escape-sequences.json",
    };

    char *test_files_should_fail[] = {
        "./tests/mock_data/7_binary.json",
        "./tests/mock_data/8_binary.json",
        "./tests/mock_data/9_unopened_brace.json",
        "./tests/mock_data/10_unopened_bracket.json",
        "./tests/mock_data/11_unclosed_brace.json",
        "./tests/mock_data/12_unclosed_bracket.json",
    };

    int total_should_pass = (sizeof(test_files_should_pass) / sizeof(test_files_should_pass[0]));
    int total_should_fail = (sizeof(test_files_should_fail) / sizeof(test_files_should_fail[0]));
    total = total_should_pass + total_should_fail;

    for (int i = 0; i < total_should_pass; i++)
    {
        JSONNode *node = parse_single_file(test_files_should_pass[i]);
        current++;
        if (node == NULL)
        {
            printf("test %d/%d FAILED\n", current, total);
        }
        else
        {
            passed++;
            printf("test %d/%d PASSED\n", current, total);
        }
        gson_destroy(node);
    }

    for (int i = 0; i < total_should_fail; i++)
    {
        JSONNode *node = parse_single_file(test_files_should_fail[i]);
        current++;
        if (node == NULL)
        {
            printf("test %d/%d PASSED\n", current, total);
            passed++;
        }
        else
        {
            printf("test %d/%d FAILED\n", current, total);
        }
        gson_destroy(node);
    }

    // print summary
    if (total == passed)
    {
        printf("SUCCESS: %d/%d test PASSED\n", passed, total);
        return 0;
    }
    printf("FAILURE: %d/%d test PASSED\n", passed, total);
    return 1;
}
