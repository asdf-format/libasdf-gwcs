#ifndef ASDF_GWCS_TRANSFORM_ROTATE3D_H
#define ASDF_GWCS_TRANSFORM_ROTATE3D_H

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * A rotation in 3-D space given by three Euler angles
 *
 * Implements up to version 1.5.0 of the
 * :transform-schema:`transform/rotate3d <rotate3d-1.5.0>` schema.  The three
 * angles are in degrees, and ``direction`` gives either the order of the
 * axes they are applied about (``"zxz"``, ``"xyz"``, ...) or one of the two
 * spherical conventions, ``"native2celestial"`` and ``"celestial2native"``.
 *
 * Under those two conventions the angles have a specific meaning: ``phi``
 * and ``theta`` are the longitude and latitude of the native pole in the
 * celestial system, and ``psi`` is the longitude of the celestial pole in
 * the native system.
 *
 * The transform acts on longitude/latitude pairs in degrees, so it has two
 * inputs and two outputs.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    /** First Euler angle, in degrees. */
    double phi;
    /** Second Euler angle, in degrees. */
    double theta;
    /** Third Euler angle, in degrees. */
    double psi;
    /**
     * Heap-allocated rotation direction: an axis order such as ``"zxz"``, or
     * ``"native2celestial"`` / ``"celestial2native"``.
     */
    const char *direction;
} asdf_gwcs_rotate3d_t;

ASDF_GWCS_DECLARE_TRANSFORM(rotate3d, ROTATE3D, asdf_gwcs_rotate3d_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_ROTATE3D_H */
