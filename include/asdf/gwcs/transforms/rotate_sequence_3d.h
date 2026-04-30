#ifndef ASDF_GWCS_TRANSFORMS_ROTATE_SEQUENCE_3D_H
#define ASDF_GWCS_TRANSFORMS_ROTATE_SEQUENCE_3D_H

#include <stdint.h>

#include <asdf/gwcs/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of the ``transform/rotate_sequence_3d-1.1.0`` schema.
 *
 * A sequence of 3-D rotations about named axes.  Each rotation is specified
 * by an angle in degrees; the axes are given by a string like ``"zyx"`` or
 * ``"xyz"``.
 */
typedef struct {
    asdf_gwcs_transform_t base;
    /** Number of rotation angles. */
    uint32_t n_angles;
    /** Heap-allocated array of rotation angles, in degrees. */
    const double *angles;
    /** Heap-allocated string naming the rotation axes (e.g. ``"zyx"``). */
    const char *axes_order;
} asdf_gwcs_rotate_sequence_3d_t;

ASDF_DECLARE_EXTENSION(gwcs_rotate_sequence_3d, asdf_gwcs_rotate_sequence_3d_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORMS_ROTATE_SEQUENCE_3D_H */
