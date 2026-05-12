#ifndef ASDF_GWCS_TRANSFORMS_SPHERICAL_CARTESIAN_H
#define ASDF_GWCS_TRANSFORMS_SPHERICAL_CARTESIAN_H

#include <stdint.h>

#include <asdf/gwcs/transform.h>

ASDF_BEGIN_DECLS

typedef enum {
    ASDF_GWCS_SPHERICAL_TO_CARTESIAN,
    ASDF_GWCS_CARTESIAN_TO_SPHERICAL,
} asdf_gwcs_spherical_cartesian_direction_t;

typedef struct {
    asdf_gwcs_transform_t base;
    asdf_gwcs_spherical_cartesian_direction_t direction;
    double wrap_lon_at;
} asdf_gwcs_spherical_cartesian_t;

ASDF_DECLARE_EXTENSION(gwcs_spherical_cartesian, asdf_gwcs_spherical_cartesian_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORMS_SPHERICAL_CARTESIAN_H */
