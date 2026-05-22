/**
 * WCS evaluation context -- pixel-to-world coordinate transforms
 */
#ifndef ASDF_GWCS_EVAL_H
#define ASDF_GWCS_EVAL_H

#include <stddef.h>

#include <asdf/file.h>
#include <asdf/util.h>

#include "asdf/gwcs/core.h"
#include "asdf/gwcs/wcs.h"

ASDF_BEGIN_DECLS


/**
 * Opaque per-WCS evaluation context created by `asdf_gwcs_eval_create`.
 *
 * Concrete backends embed this struct as their first member, enabling safe
 * casting between the base type and the backend-specific type.
 */
typedef struct asdf_gwcs_eval asdf_gwcs_eval_t;

/**
 * Backend descriptor; see `asdf/gwcs/backend.h`.
 */
typedef struct asdf_gwcs_backend asdf_gwcs_backend_t;


/**
 * Create an evaluation context for the given WCS.
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
 * Evaluate a 2-D WCS transform at *n* pixel positions.
 *
 * The input arrays *xin* and *yin* must each contain *n* elements.  On
 * success *xout* and *yout* are filled with the corresponding world
 * coordinates.
 *
 * :param eval: Evaluation context from `asdf_gwcs_eval_create`.
 * :param xin: Pixel x-coordinates (length *n*).
 * :param yin: Pixel y-coordinates (length *n*).
 * :param xout: Output world x-coordinates (length *n*).
 * :param yout: Output world y-coordinates (length *n*).
 * :param n: Number of coordinate pairs to evaluate.
 * :return: `ASDF_GWCS_OK` on success, or an error code.
 */
ASDF_EXPORT asdf_gwcs_err_t asdf_gwcs_eval_2d(
    asdf_gwcs_eval_t *eval,
    const double *xin, const double *yin,
    double *xout, double *yout, size_t n);

/**
 * Release all resources held by an evaluation context.
 *
 * :param eval: Context to destroy; must not be used afterwards.
 */
ASDF_EXPORT void asdf_gwcs_eval_destroy(asdf_gwcs_eval_t *eval);


ASDF_END_DECLS

#endif /* ASDF_GWCS_EVAL_H */
