/**
 * Partial implementation of version 1.2.0 of the :gwcs-schema:`gwcs/frame2d
 * <frame2d-1.0.0>` schema
 */

//

#ifndef ASDF_GWCS_FRAME2D_H
#define ASDF_GWCS_FRAME2D_H

#include <stdint.h>

#include <asdf/extension.h>
#include <asdf/gwcs/frame.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS

/**
 * A two-dimensional Cartesian coordinate frame (``gwcs/frame2d``)
 *
 * The per-axis arrays are indexed by axis, and any of their entries may be
 * ``NULL`` when the corresponding property is absent from the file.
 */
typedef struct {
    /** Common frame fields; `asdf_gwcs_frame_t.type` is ``ASDF_GWCS_FRAME_2D`` */
    ASDF_GWCS_FRAME_BASE;
    const char *axes_names[2];
    uint32_t axes_order[2];
    // TODO: Should be an asdf_unit_t but right now that is just a string
    const char *unit[2];
    const char *axis_physical_types[2];
} asdf_gwcs_frame2d_t;

ASDF_DECLARE_EXTENSION(gwcs_frame2d, asdf_gwcs_frame2d_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_FRAME2D_H */
