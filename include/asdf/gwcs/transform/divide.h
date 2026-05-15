#ifndef ASDF_GWCS_TRANSFORM_DIVIDE_H
#define ASDF_GWCS_TRANSFORM_DIVIDE_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    asdf_gwcs_transform_t *numerator;
    asdf_gwcs_transform_t *denominator;
} asdf_gwcs_divide_t;

ASDF_GWCS_DECLARE_TRANSFORM(divide, DIVIDE, asdf_gwcs_divide_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_DIVIDE_H */
