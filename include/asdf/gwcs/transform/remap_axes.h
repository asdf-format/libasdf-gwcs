#ifndef ASDF_GWCS_TRANSFORM_REMAP_AXES_H
#define ASDF_GWCS_TRANSFORM_REMAP_AXES_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Selects and/or reorders input axes
 *
 * Implements version 1.4.0 of the
 * :external+asdf-transform-schemas:doc:`transform/remap_axes <generated/schemas/remap_axes-1.3.0>`
 * schema.  ``mapping[idx]`` gives the index of the input axis to use for
 * output axis ``idx``.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    /** Heap-allocated array of length ``n_outputs``. */
    const uint32_t *mapping;
} asdf_gwcs_remap_axes_t;

ASDF_GWCS_DECLARE_TRANSFORM(remap_axes, REMAP_AXES, asdf_gwcs_remap_axes_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_REMAP_AXES_H */
