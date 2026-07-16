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
 * Free the heap-allocated per-axis string arrays owned by a concrete frame.
 *
 * Frees up to ``naxes`` elements of each of ``axes_names``, ``unit``, and
 * ``axis_physical_types`` (any of which may be NULL).  Does not free the
 * arrays themselves (they are inline members of the frame struct).
 */
ASDF_LOCAL void asdf_gwcs_frame_cleanup_axes(
    uint32_t naxes, char **axes_names, char **unit, char **axis_physical_types);


/**
 * Copy / deinitialize just the base frame fields (name, type).
 *
 * Frames have no registry/shim like transforms yet, so concrete frame copy and
 * deinit methods call these explicitly to handle the embedded base frame.
 */
ASDF_LOCAL bool asdf_gwcs_base_frame_copy_impl(asdf_file_t *file, const void *src, void *dst);
ASDF_LOCAL void asdf_gwcs_base_frame_deinit_impl(void *value);
