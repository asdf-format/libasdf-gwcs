#ifndef ASDF_GWCS_TRANSFORM_SHIFT_H
#define ASDF_GWCS_TRANSFORM_SHIFT_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of version 1.3.0 of the
 * :external+asdf-transform-schemas:doc:`transform/shift <generated/schemas/shift-1.2.0>` schema.
 *
 * Applies a scalar offset to a single axis: ``out = in + offset``.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    double offset;
} asdf_gwcs_shift_t;

ASDF_GWCS_DECLARE_TRANSFORM(shift, SHIFT, asdf_gwcs_shift_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_SHIFT_H */
