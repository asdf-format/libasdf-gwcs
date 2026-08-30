#ifndef ASDF_GWCS_TRANSFORM_DIVIDE_H
#define ASDF_GWCS_TRANSFORM_DIVIDE_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * The quotient of two transforms evaluated on the same input
 *
 * Implements the :external+asdf-transform-schemas:doc:`transform/divide <generated/schemas/divide-1.2.0>`
 * schema.
 *
 * ``out = numerator(in) / denominator(in)``
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;

    /** The transform supplying the dividend; owned by this object */
    asdf_gwcs_transform_t *numerator;

    /** The transform supplying the divisor; owned by this object */
    asdf_gwcs_transform_t *denominator;
} asdf_gwcs_divide_t;

ASDF_GWCS_DECLARE_TRANSFORM(divide, DIVIDE, asdf_gwcs_divide_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_DIVIDE_H */
