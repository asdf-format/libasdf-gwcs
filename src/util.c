#include <stdlib.h>
#include <string.h>

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
