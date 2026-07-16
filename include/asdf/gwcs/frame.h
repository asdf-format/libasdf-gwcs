/**
 * Partial implementation of the gwcs/frame-1.2.0 schema
 */

#ifndef ASDF_GWCS_FRAME_H
#define ASDF_GWCS_FRAME_H

#include <stdint.h>

#include <asdf/file.h>
#include <asdf/extension.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS


/**
 * Enum for tagging which type of frame a give `asdf_frame_t *` contains
 */
typedef enum {
    ASDF_GWCS_FRAME_GENERIC,
    ASDF_GWCS_FRAME_2D,
    ASDF_GWCS_FRAME_CELESTIAL,
} asdf_gwcs_frame_type_t;

typedef struct {
    asdf_gwcs_frame_type_t type;
    const char *name;
} asdf_gwcs_frame_t;

/* Extension name kept as gwcs_base_frame to avoid colliding with the
 * polymorphic asdf_gwcs_frame_destroy. */
ASDF_DECLARE_EXTENSION(gwcs_base_frame, asdf_gwcs_frame_t);

/**
 * Polymorphic value constructor: dispatches to the appropriate typed
 * asdf_value_of_gwcs_frame* function based on frame->type.
 */
ASDF_EXPORT asdf_value_t *asdf_gwcs_frame_value_of(
    asdf_file_t *file, const asdf_gwcs_frame_t *frame);

/** TODO: Document */
ASDF_EXPORT asdf_value_err_t asdf_value_as_gwcs_frame(asdf_value_t *value, asdf_gwcs_frame_t **out);
ASDF_EXPORT asdf_gwcs_frame_t *asdf_gwcs_frame_copy(asdf_file_t *file, const asdf_gwcs_frame_t *frame);
ASDF_EXPORT bool asdf_gwcs_frame_copy_into(
    asdf_file_t *file, const asdf_gwcs_frame_t *frame, asdf_gwcs_frame_t *copy);
ASDF_EXPORT void asdf_gwcs_frame_deinit(asdf_gwcs_frame_t *frame);
ASDF_EXPORT void asdf_gwcs_frame_destroy(asdf_gwcs_frame_t *frame);

ASDF_END_DECLS

#endif /* ASDF_GWCS_FRAME_H */
