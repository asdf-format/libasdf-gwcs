/**
 * Partial implementation of the gwcs/step-1.3.0 schema
 */

//

#ifndef ASDF_GWCS_STEP_H
#define ASDF_GWCS_STEP_H

#include <stdalign.h>
#include <stdint.h>

#include <asdf/extension.h>
#include <asdf/gwcs/frame.h>
#include <asdf/gwcs/transform/transform.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS

/**
 * One step of a WCS pipeline
 *
 * A step pairs a coordinate frame with the transform that maps *out of* that
 * frame and into the frame of the following step.
 */
typedef struct  {
    /** The coordinate frame this step's inputs are expressed in */
    const asdf_gwcs_frame_t *frame;

    /**
     * The transform mapping out of `frame` into the next step's frame
     *
     * This is ``NULL`` for the **last** step of a pipeline, which exists only
     * to name the frame the pipeline ends in.  Code iterating over the steps
     * of an `asdf_gwcs_t` must account for this.
     */
    const asdf_gwcs_transform_t *transform;
} asdf_gwcs_step_t;


ASDF_DECLARE_EXTENSION(gwcs_step, asdf_gwcs_step_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_STEP_H */
