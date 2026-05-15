#ifndef ASDF_GWCS_TRANSFORM_CONCATENATE_H
#define ASDF_GWCS_TRANSFORM_CONCATENATE_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of the ``transform/concatenate-1.3.0`` schema.
 *
 * Parallel composition of N transforms applied to disjoint input ranges:
 * ``(out0, out1, ...) = (forward[0](in0), forward[1](in1), ...)``.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    /** Number of sub-transforms. */
    uint32_t n_forward;
    /** Heap array of ``n_forward`` owned pointers. */
    asdf_gwcs_transform_t **forward;
} asdf_gwcs_concatenate_t;

ASDF_GWCS_DECLARE_TRANSFORM(concatenate, CONCATENATE, asdf_gwcs_concatenate_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_CONCATENATE_H */
