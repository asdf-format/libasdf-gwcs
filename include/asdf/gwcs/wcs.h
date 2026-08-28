/**
 * Partial implementation of the gwcs/wcs-1.4.0 schema
 */

//

#ifndef ASDF_GWCS_WCS_H
#define ASDF_GWCS_WCS_H

#include <stdint.h>

#include <asdf/extension.h>
#include <asdf/gwcs/step.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS


/**
 * Represents an instance of the ``gwcs/wcs-1.4.0`` schema
 *
 * .. note::
 *
 *   Most GWCS objects have corresponding structures starting with
 *   ``asdf_gwcs_``; e.g. `asdf_gwcs_step_t` to avoid ambiguity.  However this
 *   top-level object is named just ``asdf_gwcs_t`` as there is little
 *   ambiguity and avoids the repetitious "asdf_gwcs_wcs_t".
 */
typedef struct {
    /** A human-readable name for the WCS (may be ``NULL``) */
    const char *name;

    /**
     * Number of entries in `pixel_shape`, or ``0`` when it is absent
     */
    uint32_t pixel_ndim;

    /**
     * The shape of the pixel array the WCS applies to, or ``NULL``
     *
     * This corresponds to the optional ``pixel_shape`` property, which was
     * added in version 1.2.0 of the ``gwcs/wcs`` schema and is also explicitly
     * nullable.  It is ``NULL`` (with `pixel_ndim` ``0``) when the file omits
     * it, records it as ``null``, or predates the property; it is only written
     * back out when non-``NULL``.
     *
     * When present it has `pixel_ndim` elements, ordered as in the file.
     */
    const uint64_t *pixel_shape;

    /** Number of entries in `steps` */
    uint32_t n_steps;

    /**
     * The pipeline, as an array of `n_steps` steps
     *
     * The **last** step's `asdf_gwcs_step_t.transform` is ``NULL``: it exists
     * only to name the frame the pipeline ends in.
     */
    const asdf_gwcs_step_t *steps;
} asdf_gwcs_t;


ASDF_DECLARE_EXTENSION(gwcs, asdf_gwcs_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_WCS_H */
