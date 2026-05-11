#pragma once

#include <stddef.h>

#include <asdf/gwcs/core.h>
#include <asdf/gwcs/eval.h>

/* Internal base struct -- concrete backends embed this as first member */
struct asdf_gwcs_eval {
    asdf_gwcs_err_t (*eval_2d)(asdf_gwcs_eval_t *self,
                               const double *xin, const double *yin,
                               double *xout, double *yout, size_t n);
    void (*destroy)(asdf_gwcs_eval_t *self);
};
