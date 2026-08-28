#ifndef ASDF_GWCS_TRANSFORM_SPHERICAL_CARTESIAN_H
#define ASDF_GWCS_TRANSFORM_SPHERICAL_CARTESIAN_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Which way an `asdf_gwcs_spherical_cartesian_t` converts
 */
typedef enum {
    /** Longitude/latitude in, unit Cartesian vector out */
    ASDF_GWCS_SPHERICAL_TO_CARTESIAN,
    /** Cartesian vector in, longitude/latitude out */
    ASDF_GWCS_CARTESIAN_TO_SPHERICAL,
} asdf_gwcs_spherical_cartesian_direction_t;

/**
 * Conversion between spherical and Cartesian coordinates
 *
 * The direction of the conversion is given by `direction`; the two directions
 * correspond to the separate ``spherical_to_cartesian`` and
 * ``cartesian_to_spherical`` schema tags.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;

    /** Which direction this transform converts in */
    asdf_gwcs_spherical_cartesian_direction_t direction;

    /**
     * Longitude at which the branch cut is placed, in degrees
     *
     * Only meaningful for `ASDF_GWCS_CARTESIAN_TO_SPHERICAL`; conventionally
     * ``360`` or ``180``.
     */
    double wrap_lon_at;
} asdf_gwcs_spherical_cartesian_t;

ASDF_GWCS_DECLARE_TRANSFORM(
    spherical_cartesian, SPHERICAL_CARTESIAN, asdf_gwcs_spherical_cartesian_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_SPHERICAL_CARTESIAN_H */
