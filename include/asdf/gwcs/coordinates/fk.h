/**
 * The FK4 and FK5 astropy coordinate frames
 *
 * These are the equinox-based reference frames, and unlike ICRS and Galactic
 * they carry ``frame_attributes``: an ``equinox`` that every one of them
 * requires, and for the FK4 frames an optional ``obstime``.  Both are
 * ``time/time`` values, so reading them yields libasdf's `asdf_time_t`.
 */

//

#ifndef ASDF_GWCS_COORDINATES_FK_H
#define ASDF_GWCS_COORDINATES_FK_H

#include <asdf/core/time.h>
#include <asdf/gwcs/coordinates/baseframe.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS


/**
 * An FK4 or FK4NoETerms coordinate frame
 *
 * The two schemas are identical; the distinct tags exist only to name
 * different frame identities.  Which one a frame is can be told from its
 * ``type``, or read as a string with `asdf_gwcs_coordinate_frame_type_name`.
 */
typedef struct {
    ASDF_GWCS_COORDINATE_FRAME_BASE;

    /**
     * The frame's equinox, conventionally B1950 (never ``NULL``)
     *
     * Required by the schema, so a frame lacking one fails to parse.
     */
    asdf_time_t *equinox;

    /** The observation time, or ``NULL`` when the file omits it */
    asdf_time_t *obstime;
} asdf_gwcs_fk4_t;


/**
 * An FK5 coordinate frame
 *
 * Unlike FK4, the FK5 schema defines no ``obstime``.
 */
typedef struct {
    ASDF_GWCS_COORDINATE_FRAME_BASE;

    /**
     * The frame's equinox, conventionally J2000 (never ``NULL``)
     *
     * Required by the schema, so a frame lacking one fails to parse.
     */
    asdf_time_t *equinox;
} asdf_gwcs_fk5_t;


ASDF_GWCS_DECLARE_COORDINATE_FRAME(fk5, FK5, asdf_gwcs_fk5_t);
ASDF_GWCS_DECLARE_COORDINATE_FRAME(fk4, FK4, asdf_gwcs_fk4_t);
ASDF_GWCS_DECLARE_COORDINATE_FRAME(fk4noeterms, FK4_NO_E, asdf_gwcs_fk4_t);


ASDF_END_DECLS

#endif /* ASDF_GWCS_COORDINATES_FK_H */
