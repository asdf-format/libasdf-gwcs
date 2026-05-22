/** Common internal frame routines */

#include <stdint.h>

#include <asdf/util.h>
#include <asdf/value.h>

#include "gwcs.h"


/**
 * Convenience struct for passing around common frame parameters for frame parsing
 */
typedef struct {
    uint32_t max_axes;
    uint32_t min_axes;
    char **axes_names;
    uint32_t *axes_order;
    char **unit;
    char **axis_physical_types;
    /* When non-NULL, filled from the "reference_frame" mapping key if present. */
    asdf_gwcs_baseframe_t **reference_frame;
} asdf_gwcs_frame_common_params_t;


/**
 * Internal helper for parsing different frame types
 */
ASDF_LOCAL asdf_value_err_t asdf_gwcs_frame_parse(
    asdf_value_t *value, asdf_gwcs_frame_t *frame, asdf_gwcs_frame_common_params_t *params);

/**
 * Common serialization helper for all frame types
 *
 * Writes name, axes_names, axes_order, unit, axis_physical_types, and
 * (when non-NULL) reference_frame into the supplied mapping.
 * Pass naxes=0 and NULL arrays for a bare frame.
 */
ASDF_LOCAL asdf_value_err_t asdf_gwcs_frame_serialize_common(
    asdf_file_t *file,
    const char *name,
    uint32_t naxes,
    const char *const *axes_names,
    const uint32_t *axes_order,
    const char *const *unit,
    const char *const *axis_physical_types,
    const asdf_gwcs_baseframe_t *reference_frame,
    asdf_mapping_t *map);

/**
 * Polymorphic value constructor: dispatches to the appropriate typed
 * asdf_value_of_gwcs_frame* function based on frame->type.
 */
ASDF_LOCAL asdf_value_t *asdf_gwcs_frame_value_of(
    asdf_file_t *file, const asdf_gwcs_frame_t *frame);
