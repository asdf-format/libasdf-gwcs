/**
 * Hashmap from coordinate frame tag strings to `asdf_gwcs_coordinate_frame_type_t`
 */

#pragma once

#include <stc/cstr.h>

#define i_type asdf_gwcs_coordinate_frame_map
#define i_keypro cstr
#define i_val asdf_gwcs_coordinate_frame_type_t
#include <stc/hmap.h>

typedef asdf_gwcs_coordinate_frame_map asdf_gwcs_coordinate_frame_map_t;
