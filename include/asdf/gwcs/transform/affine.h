#ifndef ASDF_GWCS_TRANSFORM_AFFINE_H
#define ASDF_GWCS_TRANSFORM_AFFINE_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * A general affine transformation
 *
 * Implements the :transform-schema:`transform/affine <affine-1.3.0>` schema.
 *
 * ``out = matrix * in + translation``
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;

    /** row-major, ``n_inputs * n_inputs`` elements */
    double *matrix;

    /** ``n_inputs`` elements */
    double *translation;
} asdf_gwcs_affine_t;

ASDF_GWCS_DECLARE_TRANSFORM(affine, AFFINE, asdf_gwcs_affine_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_AFFINE_H */
