#ifndef ASDF_GWCS_TRANSFORM_CONSTANT_H
#define ASDF_GWCS_TRANSFORM_CONSTANT_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * A transform returning a fixed value, ignoring its input
 *
 * Implements the :external+asdf-transform-schemas:doc:`transform/constant <generated/schemas/constant-1.4.0>`
 * schema.
 *
 * ``out = value``
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;

    /** The constant value produced for every input */
    double value;
} asdf_gwcs_constant_t;

ASDF_GWCS_DECLARE_TRANSFORM(constant, CONSTANT, asdf_gwcs_constant_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_CONSTANT_H */
