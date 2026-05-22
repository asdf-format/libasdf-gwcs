#include <stdlib.h>

#include "asdf/gwcs/grid.h"


asdf_gwcs_err_t asdf_gwcs_grid2d_fill(
        const asdf_gwcs_grid2d_t *grid, double **xout, double **yout) {
    size_t npts = (size_t)grid->nx * grid->ny;
    bool alloc_x = (*xout == NULL);
    bool alloc_y = (*yout == NULL);

    if (alloc_x) {
        *xout = malloc(npts * sizeof(double));
        if (!*xout)
            return ASDF_GWCS_ERR_OOM;
    }
    if (alloc_y) {
        *yout = malloc(npts * sizeof(double));
        if (!*yout) {
            if (alloc_x) {
                free(*xout);
                *xout = NULL;
            }
            return ASDF_GWCS_ERR_OOM;
        }
    }

    double xstep = grid->nx > 1 ? (grid->x1 - grid->x0) / (grid->nx - 1) : 0.0;
    double ystep = grid->ny > 1 ? (grid->y1 - grid->y0) / (grid->ny - 1) : 0.0;

    for (uint32_t iy = 0; iy < grid->ny; iy++) {
        for (uint32_t ix = 0; ix < grid->nx; ix++) {
            (*xout)[iy * grid->nx + ix] = grid->x0 + xstep * ix;
            (*yout)[iy * grid->nx + ix] = grid->y0 + ystep * iy;
        }
    }
    return ASDF_GWCS_OK;
}


