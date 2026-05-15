#ifndef ASDF_GWCS_TRANSFORM_POLYNOMIAL_H
#define ASDF_GWCS_TRANSFORM_POLYNOMIAL_H

#include <stdint.h>

#include <asdf/gwcs/transform/transform.h>

ASDF_BEGIN_DECLS

/**
 * Representation of the ``transform/polynomial-1.2.0`` schema.
 *
 * An N-dimensional polynomial transform.  Coefficients are stored as a
 * ``(degree+1)^ndim`` row-major array of ``double``.
 *
 * ``ndim`` is inferred from the number of dimensions of the ``coefficients``
 * ndarray in the ASDF file, and ``degree`` from ``shape[0] - 1``.
 */
typedef struct {
    ASDF_GWCS_TRANSFORM_BASE;
    /** number of input dimensions (typically 2) */
    uint32_t ndim;
    /** polynomial degree (shape[0] - 1) */
    uint32_t degree;
    /** total elements = product of shape dimensions */
    uint32_t n_coeffs;
    /** heap-allocated, row-major */
    const double *coefficients;
} asdf_gwcs_polynomial_t;

ASDF_GWCS_DECLARE_TRANSFORM(polynomial, POLYNOMIAL, asdf_gwcs_polynomial_t);

ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_POLYNOMIAL_H */
