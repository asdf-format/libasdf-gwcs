/** Internal utilities for parsing generic transforms */
#pragma once

#define ASDF_GWCS_INTERNAL
#include "asdf/gwcs/transform.h" // IWYU pragma: export
#undef ASDF_GWCS_INTERNAL

#include <asdf/value.h>
#include "util.h"


ASDF_LOCAL asdf_value_err_t
asdf_gwcs_transform_parse(asdf_value_t *value, asdf_gwcs_transform_t *transform);

/**
 * Return the tag string (without version) for a given transform type, or
 * NULL if the type is unknown.
 */
ASDF_LOCAL const char *asdf_gwcs_transform_type_to_tag(asdf_gwcs_transform_type_t type);

/**
 * Serialize the base transform fields (name, bounding_box) into an existing
 * mapping.  Called by type-specific serializers.
 */
ASDF_LOCAL asdf_value_err_t asdf_gwcs_transform_serialize_base(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform, asdf_mapping_t *map);

/**
 * Polymorphic value constructor: dispatches to asdf_value_of_gwcs_fits for
 * fitswcs_imaging transforms, or uses a temporary extension for generic ones.
 */
ASDF_LOCAL asdf_value_t *asdf_gwcs_transform_value_of(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform);

/**
 * Release memory held by fields in an `asdf_gwcs_transform_t` and clear it in preparation
 * for releasing the transform's memory
 *
 * Does not free the transform struct's own memory.
 */
ASDF_LOCAL void asdf_gwcs_transform_clean(asdf_gwcs_transform_t *transform);

/**
 * Read an `asdf_value_t *` as any type of GWCS transform
 */
ASDF_EXPORT asdf_value_err_t
asdf_value_as_gwcs_transform(asdf_value_t *value, asdf_gwcs_transform_t **out);
ASDF_EXPORT void asdf_gwcs_transform_destroy(asdf_gwcs_transform_t *transform);
