#ifndef ASDF_GWCS_TRANSFORM_CONSTANT_H
#define ASDF_GWCS_TRANSFORM_CONSTANT_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    asdf_gwcs_transform_t base;
    double value;
} asdf_gwcs_constant_t;

ASDF_GWCS_DECLARE_TRANSFORM(constant, CONSTANT, asdf_gwcs_constant_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_CONSTANT_H */
