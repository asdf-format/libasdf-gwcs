#include "asdf/gwcs/eval.h"
#include "asdf/gwcs/backend.h"
#include "asdf/gwcs/core.h"
#include "asdf/gwcs/wcs.h"

#include "backend.h"
#include "eval.h"


asdf_gwcs_eval_t *asdf_gwcs_eval_create(
    asdf_file_t *file,
    const asdf_gwcs_t *wcs,
    const asdf_gwcs_backend_t *backend,
    asdf_gwcs_err_t *err_out) {

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_eval_t *eval = NULL;

    if (!backend)
        backend = asdf_gwcs_backend_get_default();

    if (!backend) {
        err = ASDF_GWCS_ERR_BACKEND_NOT_AVAILABLE;
        goto done;
    }

    if (!backend->pipeline || !backend->pipeline->create) {
        err = ASDF_GWCS_ERR_BACKEND_NOT_AVAILABLE;
        goto done;
    }

    eval = backend->pipeline->create(file, wcs, &err);

done:
    if (err_out)
        *err_out = err;

    if (err != ASDF_GWCS_OK)
        return NULL;

    return eval;
}


asdf_gwcs_err_t asdf_gwcs_eval_2d(
    asdf_gwcs_eval_t *eval,
    const double *xin,
    const double *yin,
    double *xout,
    double *yout,
    size_t n) {
    return eval->eval_2d(eval, xin, yin, xout, yout, n);
}


void asdf_gwcs_eval_destroy(asdf_gwcs_eval_t *eval) {
    eval->destroy(eval);
}
