#ifndef ASDF_GWCS_TRANSFORM_SCALE_H
#define ASDF_GWCS_TRANSFORM_SCALE_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * A multiplicative scaling
 *
 * Implements up to the :transform-schema:`transform/scale <scale-1.2.0>`
 * schema.
 *
 * ``out = in * factor``
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;

    /** The factor each input is multiplied by */
    double factor;
} asdf_gwcs_scale_t;

ASDF_GWCS_DECLARE_TRANSFORM(scale, SCALE, asdf_gwcs_scale_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_SCALE_H */
