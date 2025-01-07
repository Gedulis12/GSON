#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gson.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char *json_string = (char *)malloc(size + 1);
    if (json_string == NULL) {
        return 1;
    }
    memcpy(json_string, data, size);
    json_string[size] = '\0';

    Parser *parser = parser_init(json_string);
    JSONNode *node = gson_parse(parser, NULL);

    free(json_string);
    if (node)
    {
        gson_destroy(node);
    }
    parser_destroy(parser);
    return 0;
}
