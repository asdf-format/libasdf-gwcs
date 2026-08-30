#ifndef ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D_H
#define ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Whether a rotation sequence acts on Cartesian or spherical coordinates
 */
typedef enum {
    /** Rotate a Cartesian vector (the default) */
    ASDF_GWCS_ROTATION_TYPE_CARTESIAN,
    /** Rotate spherical (longitude/latitude) coordinates */
    ASDF_GWCS_ROTATION_TYPE_SPHERICAL,
} asdf_gwcs_rotation_type_t;

/**
 * Representation of version 1.1.0 of the
 * :external+asdf-transform-schemas:doc:`transform/rotate_sequence_3d <generated/schemas/rotate_sequence_3d-1.0.0>`
 * schema.
 *
 * A sequence of 3-D rotations about named axes.  Each rotation is specified
 * by an angle in degrees; the axes are given by a string like ``"zyx"`` or
 * ``"xyz"``.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    /** Number of rotation angles. */
    uint32_t n_angles;
    /** Heap-allocated array of rotation angles, in degrees. */
    const double *angles;
    /** Heap-allocated string naming the rotation axes (e.g. ``"zyx"``). */
    const char *axes_order;
    /** Rotation type: cartesian (default) or spherical. */
    asdf_gwcs_rotation_type_t rotation_type;
} asdf_gwcs_rotate_sequence_3d_t;

ASDF_GWCS_DECLARE_TRANSFORM(rotate_sequence_3d, ROTATE_SEQUENCE_3D, asdf_gwcs_rotate_sequence_3d_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D_H */
