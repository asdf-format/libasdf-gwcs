#ifndef ASDF_GWCS_TRANSFORM_SCALE_H
#define ASDF_GWCS_TRANSFORM_SCALE_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    double factor;
} asdf_gwcs_scale_t;

ASDF_GWCS_DECLARE_TRANSFORM(scale, SCALE, asdf_gwcs_scale_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_SCALE_H */
