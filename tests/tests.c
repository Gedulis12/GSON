#include "../include/gson.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    uint16_t total = 0;
    uint16_t passed = 0;
    uint16_t current = 0;

    // Parsing tests
    char *test_files_parsing_pass[] = {
        "./tests/mock_data/1_simple_json.json",
        "./tests/mock_data/2_empty_brackets.json",
        "./tests/mock_data/3_empty_braces.json",
        "./tests/mock_data/4_empty_brackets_nested.json",
        "./tests/mock_data/5_utf-8.json",
        "./tests/mock_data/6_escape-sequences.json",
    };
    int total_parsing_pass = (sizeof(test_files_parsing_pass) / sizeof(test_files_parsing_pass[0]));
    total += total_parsing_pass;

    char *test_files_parsing_fail[] = {
        "./tests/mock_data/7_binary.json",
        "./tests/mock_data/8_binary.json",
        "./tests/mock_data/9_unopened_brace.json",
        "./tests/mock_data/10_unopened_bracket.json",
        "./tests/mock_data/11_unclosed_brace.json",
        "./tests/mock_data/12_unclosed_bracket.json",
    };
    int total_parsing_fail = (sizeof(test_files_parsing_fail) / sizeof(test_files_parsing_fail[0]));
    total += total_parsing_fail;

    for (int i = 0; i < total_parsing_pass; i++)
    {
        JSONNode *node = parse_single_file(test_files_parsing_pass[i]);
        current++;
        if (node == NULL)
        {
            printf("test %d FAILED\n", current);
        }
        else
        {
            passed++;
            printf("test %d PASSED\n", current);
        }
        gson_destroy(node);
    }

    for (int i = 0; i < total_parsing_fail; i++)
    {
        JSONNode *node = parse_single_file(test_files_parsing_fail[i]);
        current++;
        if (node == NULL)
        {
            printf("test %d PASSED\n", current);
            passed++;
        }
        else
        {
            printf("test %d FAILED\n", current);
        }
        gson_destroy(node);
    }

    // API tests
    JSONNode *node = parse_single_file("./tests/mock_data/1_simple_json.json");
    if (node == NULL)
    {
        {
            printf("Parsing for API tests FAILED\n");
        }
    }

    /*  Search for node by existing key  */
    total++;
    current++;
    JSONNode *key_existing = gson_find_by_key(node, "gender");
    if (key_existing != NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
            printf("test %d FAILED\n", current);
    }

    /*  Get the key value of node with existing key  */
    total++;
    current ++;
    if (strcmp(gson_get_key_value(key_existing), "gender") == 0)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }

    /*  Search for node by non-existing key  */
    total++;
    current++;
    JSONNode *key_nexisting = gson_find_by_key(node, "gander");
    if (key_nexisting == NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /*  Get the key value of node with non-existing key  */
    total++;
    current++;
    if (gson_get_key_value(key_nexisting) == NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }


    /*  Search for node by existing string value  */
    total++;
    current++;
    JSONNode *str_existing = gson_find_by_str_val(node, "Willson");
    if (str_existing != NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* Get the key value of node with existing string value */
    total++;
    current++;
    if (strcmp(gson_get_key_value(str_existing), "surname") == 0)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /*  Search for node by non-existing string value  */
    total++;
    current++;
    JSONNode *str_nexisting = gson_find_by_str_val(node, "ABUGABUGA");
    if (str_nexisting == NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);

    }
    /* Get the key value of node with non-existing string value */
    total++;
    current++;
    if (gson_get_key_value(str_nexisting) == NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }


    /*  Search for node by existing float value  */
    total++;
    current++;
    JSONNode *float_existing = gson_find_by_float_val(node, 30);
    if (float_existing != NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* Get the key value of node with existing float value */
    total++;
    current++;
    if (strcmp(gson_get_key_value(float_existing), "age") == 0)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /*  Search for node by non-existing float value  */
    total++;
    current++;
    JSONNode *float_nexisting = gson_find_by_float_val(node, 777);
    if (float_nexisting == NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* Get the key value of node with non-existing float value */
    total++;
    current++;
    if (gson_get_key_value(float_nexisting) == NULL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_OBJECT */
    total++;
    current++;
    if (gson_get_object_type(node) == JSON_OBJECT)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_ARRAY */
    total++;
    current++;
    JSONNode *node_array = gson_find_by_key(node, "kids");
    if (gson_get_object_type(node_array) == JSON_ARRAY)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_STRING */
    total++;
    current++;
    if (gson_get_object_type(str_existing) == JSON_STRING)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_NUMBER */
    total++;
    current++;
    if (gson_get_object_type(float_existing) == JSON_NUMBER)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_TRUE_VAL */
    total++;
    current++;
    JSONNode *node_true = gson_find_by_key(node, "has_kids");
    if (gson_get_object_type(node_true) == JSON_TRUE_VAL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_FALSE_VAL */
    total++;
    current++;
    JSONNode *node_false = gson_find_by_key(node, "is_sad");
    if (gson_get_object_type(node_false) == JSON_FALSE_VAL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    /* get object type of JSON_NULL_VAL */
    total++;
    current++;
    JSONNode *node_null = gson_find_by_key(node, "grandkids");
    if (gson_get_object_type(node_null) == JSON_NULL_VAL)
    {
        printf("test %d PASSED\n", current);
        passed++;
    }
    else
    {
        printf("test %d FAILED\n", current);
    }

    gson_destroy(node);

    // print summary
    if (total == passed)
    {
        printf("SUCCESS: %d/%d test PASSED\n", passed, total);
        return 0;
    }
    printf("FAILURE: %d/%d test PASSED\n", passed, total);
    return 1;
}
