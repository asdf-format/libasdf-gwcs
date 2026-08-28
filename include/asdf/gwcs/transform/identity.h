#ifndef ASDF_GWCS_TRANSFORM_IDENTITY_H
#define ASDF_GWCS_TRANSFORM_IDENTITY_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * A transform that passes its inputs through unchanged
 *
 * ``out = in``.  It carries no parameters of its own beyond the number of
 * axes, which is given by the base :c:member:`asdf_gwcs_transform.n_inputs`.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
} asdf_gwcs_identity_t;

ASDF_GWCS_DECLARE_TRANSFORM(identity, IDENTITY, asdf_gwcs_identity_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_IDENTITY_H */
