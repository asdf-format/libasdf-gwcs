/**
 * WCS evaluation context
 */

//

#ifndef ASDF_GWCS_EVAL_H
#define ASDF_GWCS_EVAL_H

#include <stddef.h>

#include <asdf/file.h>
#include <asdf/util.h>

#include "asdf/gwcs/core.h"
#include "asdf/gwcs/wcs.h"

ASDF_BEGIN_DECLS


/**
 * Opaque per-WCS evaluation context created by `asdf_gwcs_eval_create`
 *
 * Concrete backends embed this struct as their first member, enabling safe
 * casting between the base type and the backend-specific type.
 *
 * An evaluation context belongs to the thread that created it and must not
 * be used from any other thread.  To evaluate one WCS from several threads,
 * create the context once and give each thread its own
 * `asdf_gwcs_eval_copy`.
 */
typedef struct asdf_gwcs_eval asdf_gwcs_eval_t;

/**
 * Backend descriptor; see :doc:`asdf/gwcs/backend.h </api/backend.h>`
 */
typedef struct asdf_gwcs_backend asdf_gwcs_backend_t;


/**
 * Create an evaluation context for the given WCS
 *
 * :param file: The `asdf_file_t` from which *wcs* was read (may be NULL for
 *   synthetic WCS objects).
 * :param wcs: The WCS to evaluate.
 * :param backend: Backend to use, or NULL to select the first registered
 *   backend automatically.
 * :param err_out: If non-NULL, receives the error code on failure.
 * :return: A new `asdf_gwcs_eval_t`, or NULL on error.
 */
ASDF_EXPORT asdf_gwcs_eval_t *asdf_gwcs_eval_create(
    asdf_file_t *file, const asdf_gwcs_t *wcs,
    const asdf_gwcs_backend_t *backend, asdf_gwcs_err_t *err_out);

/**
 * Evaluate a 2-D WCS transform at *n* positions
 *
 * The input arrays *xin* and *yin* must each contain *n* elements, given in
 * the context's input frame.  On success *xout* and *yout* are filled with
 * the corresponding coordinates in its output frame.  For a context from
 * `asdf_gwcs_eval_create` these are the WCS's first and last frames
 * (typically pixel and world coordinates); a context from
 * `asdf_gwcs_eval_invert` runs the other way.  Sky coordinates are in degrees
 * in either direction.
 *
 * Evaluating many points in one call is far cheaper than calling this once
 * per point, since each call carries the backend's own per-call overhead.
 * To sample a regular grid, prefer `asdf_gwcs_eval_grid2d`, which does that
 * batching for you and does not require materializing the full set of input
 * coordinates in memory.
 *
 * :param eval: Evaluation context from `asdf_gwcs_eval_create`.
 * :param xin: Input x-coordinates (length *n*).
 * :param yin: Input y-coordinates (length *n*).
 * :param xout: Output x-coordinates (length *n*).
 * :param yout: Output y-coordinates (length *n*).
 * :param n: Number of coordinate pairs to evaluate.
 * :return: `ASDF_GWCS_OK` on success, or an error code.
 */
ASDF_EXPORT asdf_gwcs_err_t asdf_gwcs_eval_2d(
    asdf_gwcs_eval_t *eval,
    const double *xin, const double *yin,
    double *xout, double *yout, size_t n);

/**
 * Create an independent copy of an evaluation context for use by another
 * thread
 *
 * An `asdf_gwcs_eval_t` may only be used by the thread that created it.  To
 * evaluate the same WCS from several threads, create it once and give each
 * thread its own copy.  The returned context is independent of the original
 * and requires no further synchronization: evaluate on it at full speed,
 * and destroy it with `asdf_gwcs_eval_destroy` like any other context.
 *
 * May be called from any thread, on the original or on another copy, and
 * concurrently from several threads at once.  The original and the copy may
 * be destroyed in either order.
 *
 * :param eval: The context to copy.
 * :param err_out: If non-NULL, receives the error code on failure.  Backends
 *   that cannot copy a context yield `ASDF_GWCS_ERR_NOT_IMPLEMENTED`.
 * :return: A new `asdf_gwcs_eval_t`, or NULL on error.
 */
ASDF_EXPORT asdf_gwcs_eval_t *asdf_gwcs_eval_copy(
    asdf_gwcs_eval_t *eval, asdf_gwcs_err_t *err_out);

/**
 * Create a new evaluation context for the inverse of an existing one
 *
 * The returned context's forward direction is the inverse of *eval*'s: it
 * takes coordinates in *eval*'s output frame and returns them in its input
 * frame.  Evaluate it with `asdf_gwcs_eval_2d` like any other context.
 *
 * Where the WCS declares an explicit ``inverse`` for a transform, that
 * declared inverse is what gets evaluated, exactly as given in the file; no
 * check is made that it accurately inverts the forward transform.
 * Otherwise it is up to the backend whether, and how, it can invert the
 * transform.
 *
 * Inverting the returned context again gives a context evaluating in the
 * original direction, and `asdf_gwcs_eval_copy` of an inverted context is
 * also inverted.
 *
 * This has the same thread-safety guarantees as `asdf_gwcs_eval_copy`.
 * Destroy it with `asdf_gwcs_eval_destroy`.
 *
 * :param eval: The context to invert.
 * :param err_out: If non-NULL, receives the error code on failure.  This is
 *   `ASDF_GWCS_ERR_NO_INVERSE` if the backend cannot invert this particular
 *   WCS, or `ASDF_GWCS_ERR_NOT_IMPLEMENTED` if it cannot invert any.
 * :return: A new `asdf_gwcs_eval_t`, or NULL on error.
 */
ASDF_EXPORT asdf_gwcs_eval_t *asdf_gwcs_eval_invert(
    asdf_gwcs_eval_t *eval, asdf_gwcs_err_t *err_out);

/**
 * Release all resources held by an evaluation context
 *
 * Passing ``NULL`` is a no-op.
 *
 * :param eval: Context to destroy; must not be used afterwards.
 */
ASDF_EXPORT void asdf_gwcs_eval_destroy(asdf_gwcs_eval_t *eval);


ASDF_END_DECLS

#endif /* ASDF_GWCS_EVAL_H */
