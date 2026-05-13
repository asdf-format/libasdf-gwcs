#ifndef ASDF_GWCS_TRANSFORM_SCALE_H
#define ASDF_GWCS_TRANSFORM_SCALE_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    asdf_gwcs_transform_t base;
    double factor;
} asdf_gwcs_scale_t;

ASDF_DECLARE_EXTENSION(gwcs_scale, asdf_gwcs_scale_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_SCALE_H */
