#ifndef ASDF_GWCS_TRANSFORM_AFFINE_H
#define ASDF_GWCS_TRANSFORM_AFFINE_H

#include <stdint.h>

#include <asdf/gwcs/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    asdf_gwcs_transform_t base;
    /** row-major, base.n_inputs * base.n_inputs elements */
    double *matrix;
    /** base.n_inputs elements */
    double *translation;
} asdf_gwcs_affine_t;

ASDF_DECLARE_EXTENSION(gwcs_affine, asdf_gwcs_affine_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_AFFINE_H */
