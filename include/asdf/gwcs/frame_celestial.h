/**
 * Partial implementation of version 1.2.0 of the
 * :gwcs-schema:`gwcs/celestial_frame <celestial_frame-1.0.0>` schema
 */

//

#ifndef ASDF_GWCS_FRAME_CELESTIAL_H
#define ASDF_GWCS_FRAME_CELESTIAL_H

#include <stdint.h>

#include <asdf/extension.h>
#include <asdf/gwcs/coordinates/baseframe.h>
#include <asdf/gwcs/frame.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS

/**
 * A celestial coordinate frame (``gwcs/celestial_frame``)
 *
 * The per-axis arrays are sized for three axes so that the same layout covers
 * frames carrying a distance or radial-velocity axis alongside the two sky
 * axes; entries beyond those actually present are ``NULL``.
 */
typedef struct {
    /** Common frame fields; `asdf_gwcs_frame_t.type` is ``ASDF_GWCS_FRAME_CELESTIAL`` */
    asdf_gwcs_frame_t base;
    const char *axes_names[3];
    uint32_t axes_order[3];
    const char *unit[3];
    const char *axis_physical_types[3];
    /**
     * Astropy coordinate reference frame, or NULL if absent/unrecognized.
     *
     * Ownership: the celestial frame owns this pointer; it is freed by
     * ``asdf_gwcs_frame_celestial_destroy``.
     */
    asdf_gwcs_baseframe_t *reference_frame;
} asdf_gwcs_frame_celestial_t;

ASDF_DECLARE_EXTENSION(gwcs_frame_celestial, asdf_gwcs_frame_celestial_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_FRAME_CELESTIAL_H */
