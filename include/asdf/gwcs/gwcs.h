/**
 * Umbrella header for libasdf-gwcs
 *
 * Including this pulls in the entire public API: the WCS object and its steps,
 * every coordinate frame and transform type, and the evaluation interface.
 * Most programs need only this header alongside libasdf's own ``<asdf.h>``.
 *
 * Individual headers can be included instead where only part of the API is
 * wanted.  ``<asdf/gwcs/transform.h>`` is a second convenience header of the
 * same kind, covering every transform type and nothing else.
 */

//

#ifndef ASDF_GWCS_GWCS_H
#define ASDF_GWCS_GWCS_H

#include <asdf/gwcs/backend.h>
#include <asdf/gwcs/coordinates/fk.h>
#include <asdf/gwcs/core.h>
#include <asdf/gwcs/eval.h>
#include <asdf/gwcs/grid.h>
#include <asdf/gwcs/fitswcs_imaging.h>
#include <asdf/gwcs/frame2d.h>
#include <asdf/gwcs/frame_celestial.h>
#include <asdf/gwcs/step.h>
#include <asdf/gwcs/transform.h>
#include <asdf/gwcs/transform/compose.h>
#include <asdf/gwcs/transform/concatenate.h>
#include <asdf/gwcs/transform/polynomial.h>
#include <asdf/gwcs/transform/remap_axes.h>
#include <asdf/gwcs/transform/rotate_sequence_3d.h>
#include <asdf/gwcs/transform/shift.h>
#include <asdf/gwcs/wcs.h>

#endif /* ASDF_GWCS_GWCS_H */
