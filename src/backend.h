#pragma once

#include "asdf/gwcs/backend.h"

/* Returns the first registered backend, or NULL if no backends are registered */
const asdf_gwcs_backend_t *asdf_gwcs_backend_get_default(void);
