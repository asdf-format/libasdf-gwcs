#ifndef ASDF_GWCS_TRANSFORM_COMPOSE_H
#define ASDF_GWCS_TRANSFORM_COMPOSE_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of the ``transform/compose-1.3.0`` schema.
 *
 * Serial composition of two transforms applied right-to-left:
 * ``out = forward[0](forward[1](in))``.
 *
 * ``forward`` always has exactly two elements.
 */
typedef struct {
    asdf_gwcs_transform_t base;
    /** Always 2. */
    uint32_t n_forward;
    /** Heap array of ``n_forward`` owned pointers. */
    asdf_gwcs_transform_t **forward;
} asdf_gwcs_compose_t;

ASDF_DECLARE_EXTENSION(gwcs_compose, asdf_gwcs_compose_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_COMPOSE_H */
