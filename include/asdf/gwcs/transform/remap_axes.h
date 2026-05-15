#ifndef ASDF_GWCS_TRANSFORM_REMAP_AXES_H
#define ASDF_GWCS_TRANSFORM_REMAP_AXES_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of the ``transform/remap_axes-1.4.0`` schema.
 *
 * Selects and/or reorders input axes.  ``mapping[idx]`` gives the index of the
 * input axis to use for output axis ``idx``.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    /** Heap-allocated array of length ``base.n_outputs``. */
    const uint32_t *mapping;
} asdf_gwcs_remap_axes_t;

ASDF_GWCS_DECLARE_TRANSFORM(remap_axes, REMAP_AXES, asdf_gwcs_remap_axes_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_REMAP_AXES_H */
