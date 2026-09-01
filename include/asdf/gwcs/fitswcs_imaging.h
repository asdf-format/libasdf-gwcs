/**
 * Representation of the ``gwcs/fitswcs_imaging-1.0.0`` schema--a GWCS
 * transform encapsulating a FITS WCS
 *
 * The C type representing the FITS WCS data is called just `asdf_gwcs_fits_t`
 * for short, and the generated accessors are named accordingly
 * (``asdf_get_gwcs_fits``, ``asdf_gwcs_fits_destroy``, and so on) even though
 * the schema and this header are named ``fitswcs_imaging``.
 */

//

#ifndef ASDF_GWCS_FITSWCS_IMAGING_H
#define ASDF_GWCS_FITSWCS_IMAGING_H

#include <asdf/gwcs/transform/transform.h>
#include <asdf/gwcs/wcs.h>

ASDF_BEGIN_DECLS

/**
 * A FITS imaging WCS: CRPIX, CRVAL, CDELT and PC plus a projection
 *
 * Contains the properties of a ``gwcs/fitswcs_imaging-1.0.0`` object.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;

    /** The FITS CRPIXn headers (0-indexed) */
    const double crpix[2];

    /** The FITS CRVALn headers (0-indexed) */
    const double crval[2];

    /** The FITS CDELTn headers (0-indexed) */
    const double cdelt[2];

    /** The FITS PCij headers (0-indexed) */
    const double pc[2][2];

    /**
     * The FITS CTYPEn headers (0-indexed), e.g. ``RA---TAN``
     *
     * Unlike every other member here, these are *derived* rather than read
     * from the ``fitswcs_imaging`` object, because a CTYPE is made of two
     * halves that come from two different places:
     *
     * - The **coordinate type** (``RA``, ``DEC``, ``GLON``, ...) comes from
     *   the ``axis_physical_types`` of the WCS's *output* frame, whose UCD1+
     *   terms map onto it: ``pos.eq.ra`` gives ``RA``, ``pos.galactic.lon``
     *   gives ``GLON``, and so on.
     * - The **projection code** (``TAN``, ``SIN``, ...) comes from
     *   `projection`, which is part of this object.
     *
     * Only the second half is knowable from a ``fitswcs_imaging`` object on
     * its own.  The output frame is a sibling step of the containing
     * `asdf_gwcs_t`, not part of this transform, so the first half is simply
     * not present in the data unless the whole WCS is at hand.
     *
     * .. warning::
     *
     *   Reading the full GWCS (with ``asdf_get_gwcs``, say) fills these in.
     *   Reading a ``fitswcs_imaging`` transform on its own leaves them
     *   ``NULL``, and no amount of inspecting the transform can recover them.
     */
    const char *ctype[2];

    /** The projection transform (e.g. gnomonic/``TAN``), owned by this object. */
    asdf_gwcs_transform_t *projection;
} asdf_gwcs_fits_t;


/**
 * Return whether the given `asdf_gwcs_t` represents a FITS-compatible WCS
 *
 * What we mean in this case by "FITS-compatible WCS" is simply a GWCS with:
 *
 * - Two steps
 * - The transform from the first to the second step of type
 *   ``fitswcs_imaging-*``, specifically
 * - The last step must have a coordinate frame with ``axis_physical_types``
 *   specified.  Currently only celestial coordinates are supported though
 *   more will be added.
 *
 * This does not otherwise make any presumptions about whether an arbitrary
 * GWCS could be converted losslessly to a FITS WCS, or approximated as a FITS
 * WCS, though those could be interesting extensions for the future.
 *
 * This is, in other words just checking if this is a FITS WCS is embeded in a
 * GWCS object.
 *
 * :param file: The `asdf_file_t *` handle for the file from which the GWCS was
 *   read
 * :param gwcs: An `asdf_gwcs_t *`
 * :return: `true` if the GWCS matches the conditions described above
 */
ASDF_EXPORT bool asdf_gwcs_is_fits(const asdf_file_t *file, asdf_gwcs_t *gwcs);



/* This declares the ASDF_GWCS_TRANSFORM_FITWCS_IMAGING constant as well
 * as the libasdf extension declarations. */
ASDF_GWCS_DECLARE_TRANSFORM(fits, FITSWCS_IMAGING, asdf_gwcs_fits_t);


ASDF_END_DECLS
#endif /* ASDF_GWCS_FITSWCS_IMAGING_H */
