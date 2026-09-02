/**
 * 2-D pixel sampling grid utilities
 */

//

#ifndef ASDF_GWCS_GRID_H
#define ASDF_GWCS_GRID_H

#include <stdint.h>

#include <asdf/util.h>
#include <asdf/gwcs/eval.h>

ASDF_BEGIN_DECLS


/**
 * Parameters for a 2-D rectangular pixel sampling grid
 *
 * The grid is the product of two evenly spaced sequences of sample
 * coordinates, one per axis, with both endpoints included.  For a dense pixel
 * grid---one sample per pixel, unit spacing---set ``x1 = x0 + nx - 1`` and
 * ``y1 = y0 + ny - 1``.
 *
 * A grid2d can be initialized directly like::
 *
 *   // 32x32 samples spanning a 4088x4088 detector, corners included.
 *   asdf_gwcs_grid2d_t grid = {
 *       .x0 = 0.0, .y0 = 0.0,
 *       .x1 = 4087.0, .y1 = 4087.0,
 *       .nx = 32, .ny = 32
 *   };
 *
 * Samples are ordered row-major with x varying fastest, so the sample at
 * ``(ix, iy)`` is at index ``iy * nx + ix``.
 */
typedef struct {
    /** First sample coordinate along x, inclusive */
    double x0;

    /** First sample coordinate along y, inclusive */
    double y0;

    /** Last sample coordinate along x, inclusive */
    double x1;

    /** Last sample coordinate along y, inclusive */
    double y1;

    /**
     * Number of samples along x
     *
     * ``1`` gives a single column at ``x0``, and ``x1`` is then unused.
     */
    uint32_t nx;

    /**
     * Number of samples along y
     *
     * ``1`` gives a single row at ``y0``, and ``y1`` is then unused.
     */
    uint32_t ny;
} asdf_gwcs_grid2d_t;


/**
 * Fill coordinate arrays for a 2-D grid
 *
 * If ``*xout`` is NULL the function allocates an array of ``nx * ny``
 * doubles and stores the pointer in ``*xout``; otherwise it writes into
 * the existing buffer.  The same rule applies to ``*yout`` independently.
 * The caller is responsible for freeing any array allocated here.
 *
 * The arrays follow the grid's own sample order, described on
 * `asdf_gwcs_grid2d_t`.
 *
 * :param grid: Grid descriptor.
 * :param xout: Address of x-coordinate array pointer (allocated if ``*xout`` is NULL).
 * :param yout: Address of y-coordinate array pointer (allocated if ``*yout`` is NULL).
 * :return: ``ASDF_GWCS_OK`` on success, ``ASDF_GWCS_ERR_OOM`` on allocation failure.
 */
ASDF_EXPORT asdf_gwcs_err_t asdf_gwcs_grid2d_fill(
    const asdf_gwcs_grid2d_t *grid, double **xout, double **yout);


/**
 * Evaluate a 2-D WCS transform over a rectangular pixel grid
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
