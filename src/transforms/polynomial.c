#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/core/ndarray.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../transform.h"
#include "../util.h"
#include "polynomial.h"


static asdf_value_err_t asdf_gwcs_polynomial_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_polynomial_t *poly = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_ndarray_t *ndarray = NULL;
    double *coefficients = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    poly = calloc(1, sizeof(asdf_gwcs_polynomial_t));

    if (!poly) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &poly->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(
        map, "coefficients", ASDF_VALUE_EXTENSION, ASDF_CORE_NDARRAY_TAG, (void *)&ndarray);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (ndarray->datatype.type != ASDF_DATATYPE_FLOAT64) {
#ifdef ASDF_LOG_ENABLED
        const asdf_file_t *file = asdf_value_file(value);
        const char *path = asdf_value_path(value);
        ASDF_LOG(file, ASDF_LOG_ERROR, "polynomial coefficients must be float64 at %s", path);
#endif
        goto cleanup;
    }

    uint32_t ndim = ndarray->ndim;
    uint32_t degree = (uint32_t)(ndarray->shape[0] - 1);
    uint64_t n_coeffs = asdf_ndarray_size(ndarray);

    void *dst = NULL;
    asdf_ndarray_err_t nd_err = asdf_ndarray_read_all(ndarray, ASDF_DATATYPE_FLOAT64, &dst);

    if (nd_err != ASDF_NDARRAY_OK) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    coefficients = dst;

    poly->ndim = ndim;
    poly->degree = degree;
    poly->n_coeffs = (uint32_t)n_coeffs;
    poly->coefficients = coefficients;
    coefficients = NULL;

    *out = poly;
    err = ASDF_VALUE_OK;
cleanup:
    free(coefficients);
    asdf_ndarray_destroy(ndarray);

    if (ASDF_IS_ERR(err))
        asdf_gwcs_polynomial_destroy(poly);

    return err;
}


static asdf_value_t *asdf_gwcs_polynomial_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_polynomial_t *poly = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &poly->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    /* Build ndarray for coefficients */
    if (poly->ndim == 0 || poly->n_coeffs == 0)
        goto cleanup;

    uint64_t *shape = calloc(poly->ndim, sizeof(uint64_t));

    if (!shape)
        goto cleanup;

    for (uint32_t idx = 0; idx < poly->ndim; idx++)
        shape[idx] = poly->degree + 1;

    asdf_ndarray_t ndarray = {
        .ndim = poly->ndim,
        .shape = shape,
        .datatype = {.type = ASDF_DATATYPE_FLOAT64},
        .byteorder = ASDF_BYTEORDER_LITTLE,
    };

    void *data = asdf_ndarray_data_alloc_temp(file, &ndarray);

    if (!data) {
        free(shape);
        goto cleanup;
    }

    memcpy(data, poly->coefficients, (size_t)poly->n_coeffs * sizeof(double));

    asdf_value_t *coeff_val = asdf_value_of_ndarray(file, &ndarray);
    free(shape);
    ndarray.shape = NULL;

    if (!coeff_val)
        goto cleanup;

    err = asdf_mapping_set(map, "coefficients", coeff_val);

    if (ASDF_IS_ERR(err)) {
        asdf_value_destroy(coeff_val);
        goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_polynomial_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_polynomial_t *poly = (asdf_gwcs_polynomial_t *)value;
    asdf_gwcs_transform_clean(&poly->base);
    free((double *)poly->coefficients);
    free(poly);
}


ASDF_REGISTER_EXTENSION(
    gwcs_polynomial,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "polynomial-1.2.0",
    asdf_gwcs_polynomial_t,
    &libasdf_software,
    asdf_gwcs_polynomial_serialize,
    asdf_gwcs_polynomial_deserialize,
    NULL,
    asdf_gwcs_polynomial_dealloc,
    NULL);
