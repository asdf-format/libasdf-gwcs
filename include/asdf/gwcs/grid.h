/**
 * 2-D pixel sampling grid utilities
 */
#ifndef ASDF_GWCS_GRID_H
#define ASDF_GWCS_GRID_H

#include <stdint.h>

#include <asdf/util.h>
#include <asdf/gwcs/eval.h>

ASDF_BEGIN_DECLS


/**
 * Parameters for a 2-D rectangular pixel sampling grid.
 *
 * ``x0``/``y0`` are the first sample coordinates (inclusive);
 * ``x1``/``y1`` are the last (inclusive).  ``nx`` and ``ny`` are the
 * number of evenly-spaced sample points along each axis.  For a
 * dense pixel grid (unit spacing) set ``x1 = x0 + nx - 1`` and
 * ``y1 = y0 + ny - 1``.
 */
typedef struct {
    double   x0, y0;
    double   x1, y1;
    uint32_t nx, ny;
} asdf_gwcs_grid2d_t;


/**
 * Fill coordinate arrays for a 2-D grid.
 *
 * If ``*xout`` is NULL the function allocates an array of ``nx * ny``
 * doubles and stores the pointer in ``*xout``; otherwise it writes into
 * the existing buffer.  The same rule applies to ``*yout`` independently.
 * The caller is responsible for freeing any array allocated here.
 *
 * Layout is row-major: index ``iy * nx + ix``.  If ``nx == 1`` the
 * x-step is zero (single column); likewise for ``ny == 1``.
 *
 * :param grid: Grid descriptor.
 * :param xout: Address of x-coordinate array pointer (allocated if ``*xout`` is NULL).
 * :param yout: Address of y-coordinate array pointer (allocated if ``*yout`` is NULL).
 * :return: ``ASDF_GWCS_OK`` on success, ``ASDF_GWCS_ERR_OOM`` on allocation failure.
 */
ASDF_EXPORT asdf_gwcs_err_t asdf_gwcs_grid2d_fill(
    const asdf_gwcs_grid2d_t *grid, double **xout, double **yout);


/**
 * Evaluate a 2-D WCS transform over a rectangular pixel grid.
 *
 * Processes one row at a time so peak working memory is ``O(nx)`` rather
 * than ``O(nx * ny)``.  The NULL-means-allocate convention of
 * `asdf_gwcs_grid2d_fill` applies to ``*xout`` and ``*yout``.
 *
 * :param eval: Evaluation context from `asdf_gwcs_eval_create`.
 * :param grid: Grid descriptor.
 * :param xout: Address of output world x-coordinate array pointer.
 * :param yout: Address of output world y-coordinate array pointer.
 * :return: ``ASDF_GWCS_OK`` on success, or an error code.
 */
ASDF_EXPORT asdf_gwcs_err_t asdf_gwcs_eval_grid2d(
    asdf_gwcs_eval_t *eval,
    const asdf_gwcs_grid2d_t *grid,
    double **xout, double **yout);


ASDF_END_DECLS

#endif /* ASDF_GWCS_GRID_H */
