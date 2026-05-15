/* Hashmap from backend name strings to asdf_gwcs_backend_t pointers */

#pragma once

#include <stc/cstr.h>

#include <asdf/gwcs/backend.h>

#define i_type asdf_gwcs_backend_map
#define i_keypro cstr
#define i_val const asdf_gwcs_backend_t *
#include <stc/hmap.h>

typedef asdf_gwcs_backend_map asdf_gwcs_backend_map_t;
