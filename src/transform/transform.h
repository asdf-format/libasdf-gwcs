/** Internal utilities for parsing generic transforms */
#pragma once

#define ASDF_GWCS_INTERNAL
#include "asdf/gwcs/transform/transform.h" // IWYU pragma: export
#undef ASDF_GWCS_INTERNAL

#include <asdf/util.h>
#include <asdf/value.h>

#include "util.h"


ASDF_LOCAL asdf_value_err_t
asdf_gwcs_transform_parse(asdf_value_t *value, asdf_gwcs_transform_t *transform);

/**
 * Cross-check YAML-derived n_inputs / n_outputs against implicit values
 * computed from a transform's own properties, then set the fields.
 *
 * If the YAML did not supply an inputs/outputs sequence (field is 0), the
 * implicit value is assigned directly.  If both are non-zero and differ, a
 * warning is logged and the YAML-derived value is kept.
 */
ASDF_LOCAL void asdf_gwcs_transform_arity_set(
    asdf_gwcs_transform_t *transform,
    UNUSED(const asdf_file_t *file),
    uint32_t implicit_n_inputs,
    uint32_t implicit_n_outputs);

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
