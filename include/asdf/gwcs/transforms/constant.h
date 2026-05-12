#ifndef ASDF_GWCS_TRANSFORMS_CONSTANT_H
#define ASDF_GWCS_TRANSFORMS_CONSTANT_H

#include <stdint.h>

#include <asdf/gwcs/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    asdf_gwcs_transform_t base;
    double value;
    uint32_t dimensions;
} asdf_gwcs_constant_t;

ASDF_DECLARE_EXTENSION(gwcs_constant, asdf_gwcs_constant_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORMS_CONSTANT_H */
