#pragma once

#include <stddef.h>

#include "asdf/gwcs/core.h"
#include "asdf/gwcs/eval.h"  // IWYU pragma: export

/* Internal base struct -- concrete backends embed this as first member */
struct asdf_gwcs_eval {
    asdf_gwcs_err_t (*eval_2d)(
        asdf_gwcs_eval_t *self,
        const double *xin,
        const double *yin,
        double *xout,
        double *yout,
        size_t n);
    void (*destroy)(asdf_gwcs_eval_t *self);
    /** Deep-copy for use by another thread; NULL if the backend cannot */
    asdf_gwcs_eval_t *(*copy)(asdf_gwcs_eval_t *self, asdf_gwcs_err_t *err_out);
    /** Copy evaluating in the inverse direction; NULL if the backend cannot */
    asdf_gwcs_eval_t *(*invert)(asdf_gwcs_eval_t *self, asdf_gwcs_err_t *err_out);
};
