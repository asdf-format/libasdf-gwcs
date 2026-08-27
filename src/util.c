#include <stdlib.h>
#include <string.h>

#include <asdf/extension.h>

#include "util.h"


char *tag_canonicalize(const char *tag) {
    if (strncmp(tag, "tag:", 4) != 0) {
        size_t len = strlen(tag);
        char *full = malloc(4 + len + 1);
        if (!full)
            return NULL;
        memcpy(full, "tag:", 4);
        memcpy(full + 4, tag, len + 1);
        return full;
    }
    return strdup(tag);
}


void tag_type_name(char *dest, size_t size, const char *tag) {
    assert(dest && size > 0);
    dest[0] = '\0';

    if (!tag)
        return;

    /* Everything up to the last '/' is the schema's path within its
     * namespace; asdf_tag_parse handles splitting off the version. */
    const char *base = strrchr(tag, '/');
    asdf_tag_t *parsed = asdf_tag_parse(base ? base + 1 : tag);

    if (!parsed)
        return;

    if (parsed->name) {
        strncpy(dest, parsed->name, size - 1);
        dest[size - 1] = '\0';
    }

    asdf_tag_destroy(parsed);
}
