#ifndef ASDF_GWCS_TRANSFORMS_REMAP_AXES_H
#define ASDF_GWCS_TRANSFORMS_REMAP_AXES_H

#include <stdint.h>

#include <asdf/gwcs/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of the ``transform/remap_axes-1.4.0`` schema.
 *
 * Selects and/or reorders input axes.  ``mapping[idx]`` gives the index of the
 * input axis to use for output axis ``idx``.
 */
typedef struct {
    asdf_gwcs_transform_t base;
    uint32_t n_mapping;
    /** heap-allocated array of length n_mapping */
    const uint32_t *mapping;
} asdf_gwcs_remap_axes_t;

ASDF_DECLARE_EXTENSION(gwcs_remap_axes, asdf_gwcs_remap_axes_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORMS_REMAP_AXES_H */
