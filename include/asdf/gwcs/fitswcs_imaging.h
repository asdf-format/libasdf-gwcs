/**
 * Representation of the http://stsci.edu/schemas/gwcs/fitswcs_imaging-1.0.0
 * schema--a GWCS transform encapuslating a FITS WCS
 *
 * The C type representing the FITS WCS data is called just `asdf_gwcs_fits_t`
 * for short.
 *
 * If libasdf is configured with the ``--with-fits-wcs`` flag (which requires
 * WCSLIB) then it is also possible to convert this to the associated
 * `wcsprm`.  But even without WCSLIB it is possible to read these objects.
 */
#ifndef ASDF_GWCS_FITSWCS_IMAGING_H
#define ASDF_GWCS_FITSWCS_IMAGING_H

#include <asdf/gwcs/transform.h>
#include <asdf/gwcs/wcs.h>

ASDF_BEGIN_DECLS

/**
 * Contains properties from an ``gwcs/fitswcs_imaging-1.0.0`` object
 */
typedef struct {
    asdf_gwcs_transform_t base;

    /** The FITS CRPIXn headers (0-indexed) */
    const double crpix[2];

    /** The FITS CRVALn headers (0-indexed) */
    const double crval[2];

    /** The FITS CDELTn headers (0-indexed) */
    const double cdelt[2];

    /** The FITS PCij headers (0-indexed) */
    const double pc[2][2];

    /**
     * The FITS CTYPEn headers (0-indexed)
     *
     * .. warning::
     *
     *   Extracting the correct CTYPEn headers requires the full GWCS object
     *   to be read (e.g. with `asdf_get_gwcs`).  In this case the ctype values
     *   will be filled in on this object.
     *
     *   Otherwise, if a ``fitswcs_imaging`` transform object is read directly,
     *   without the full context of its containing GWCS, these values will be
     *   ``NULL``!
     */
    const char *ctype[2];

    asdf_gwcs_transform_t projection;
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


ASDF_DECLARE_EXTENSION(gwcs_fits, asdf_gwcs_fits_t);

ASDF_END_DECLS
#endif /* ASDF_GWCS_FITSWCS_IMAGING_H */
