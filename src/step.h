#pragma once

#include <stdbool.h>

#include "frame.h"
#include "transform.h"


/** Internal definition of asdf_gwcs_step_t */
typedef struct _asdf_gwcs_step {
    asdf_gwcs_frame_t *frame;
    const asdf_gwcs_transform_t *transform;
    bool free;
} asdf_gwcs_step_t;
