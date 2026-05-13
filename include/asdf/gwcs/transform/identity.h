#ifndef ASDF_GWCS_TRANSFORM_IDENTITY_H
#define ASDF_GWCS_TRANSFORM_IDENTITY_H

#include <asdf/gwcs/transform.h>

ASDF_BEGIN_DECLS

typedef struct {
    asdf_gwcs_transform_t base;
} asdf_gwcs_identity_t;

ASDF_DECLARE_EXTENSION(gwcs_identity, asdf_gwcs_identity_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_IDENTITY_H */
