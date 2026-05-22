#include <stdlib.h>

#include "asdf/gwcs/eval.h"
#include "asdf/gwcs/backend.h"
#include "asdf/gwcs/core.h"
#include "asdf/gwcs/grid.h"
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


asdf_gwcs_err_t asdf_gwcs_eval_grid2d(
        asdf_gwcs_eval_t *eval,
        const asdf_gwcs_grid2d_t *grid,
        double **xout, double **yout) {
    size_t npts = (size_t)grid->nx * grid->ny;
    bool alloc_x = (*xout == NULL);
    bool alloc_y = (*yout == NULL);
    double *xrow = NULL;
    double *yrow = NULL;
    asdf_gwcs_err_t err = ASDF_GWCS_OK;

    if (alloc_x) {
        *xout = malloc(npts * sizeof(double));

        if (!*xout) {
            err = ASDF_GWCS_ERR_OOM;
            goto cleanup; 
        }
    }
    if (alloc_y) {
        *yout = malloc(npts * sizeof(double));

        if (!*yout) {
            err = ASDF_GWCS_ERR_OOM; goto cleanup;
        }
    }

    double xstep = grid->nx > 1 ? (grid->x1 - grid->x0) / (grid->nx - 1) : 0.0;
    double ystep = grid->ny > 1 ? (grid->y1 - grid->y0) / (grid->ny - 1) : 0.0;

    xrow = malloc(grid->nx * sizeof(double));
    yrow = malloc(grid->nx * sizeof(double));

    if (!xrow || !yrow) {
        err = ASDF_GWCS_ERR_OOM; goto cleanup;
    }

    for (uint32_t ix = 0; ix < grid->nx; ix++)
        xrow[ix] = grid->x0 + xstep * ix;

    for (uint32_t iy = 0; iy < grid->ny && err == ASDF_GWCS_OK; iy++) {
        double y = grid->y0 + ystep * iy;

        for (uint32_t ix = 0; ix < grid->nx; ix++)
            yrow[ix] = y;

        err = asdf_gwcs_eval_2d(eval, xrow, yrow,
                                *xout + iy * grid->nx,
                                *yout + iy * grid->nx,
                                grid->nx);
    }

cleanup:
    free(xrow);
    free(yrow);

    if (err != ASDF_GWCS_OK) {

        if (alloc_x) {
            free(*xout);
            *xout = NULL;
        }

        if (alloc_y) {
            free(*yout);
            *yout = NULL;
        }
    }

    return err;
}
