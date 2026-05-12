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
#include "affine.h"


static asdf_value_err_t asdf_gwcs_affine_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_affine_t *affine = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_ndarray_t *mat_arr = NULL;
    asdf_ndarray_t *tr_arr = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    affine = calloc(1, sizeof(asdf_gwcs_affine_t));

    if (!affine) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &affine->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(
        map, "matrix", ASDF_VALUE_EXTENSION, ASDF_CORE_NDARRAY_TAG, (void *)&mat_arr);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (mat_arr->ndim != 2 || mat_arr->shape[0] != mat_arr->shape[1]) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    uint32_t n_dims = (uint32_t)mat_arr->shape[0];

    asdf_ndarray_err_t nd_err = asdf_ndarray_read_all(
        mat_arr, ASDF_DATATYPE_FLOAT64, (void **)&affine->matrix);

    if (nd_err != ASDF_NDARRAY_OK) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    err = asdf_get_required_property(
        map, "translation", ASDF_VALUE_EXTENSION, ASDF_CORE_NDARRAY_TAG, (void *)&tr_arr);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (tr_arr->ndim != 1 || tr_arr->shape[0] != n_dims) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    nd_err = asdf_ndarray_read_all(
        tr_arr, ASDF_DATATYPE_FLOAT64, (void **)&affine->translation);

    if (nd_err != ASDF_NDARRAY_OK) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    asdf_gwcs_transform_arity_set(&affine->base, asdf_value_file(value), n_dims, n_dims);

    *out = affine;
    err = ASDF_VALUE_OK;
cleanup:
    asdf_ndarray_destroy(mat_arr);
    asdf_ndarray_destroy(tr_arr);

    if (ASDF_IS_ERR(err))
        asdf_gwcs_affine_destroy(affine);

    return err;
}


static asdf_value_t *asdf_gwcs_affine_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_affine_t *affine = obj;

    if (!affine->matrix || !affine->translation || affine->base.n_inputs == 0)
        return NULL;

    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &affine->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    uint64_t mat_shape[2] = {affine->base.n_inputs, affine->base.n_inputs};
    asdf_ndarray_t mat_arr = {
        .ndim = 2,
        .shape = mat_shape,
        .datatype = {.type = ASDF_DATATYPE_FLOAT64},
        .byteorder = ASDF_BYTEORDER_LITTLE,
    };

    asdf_ndarray_storage_set(&mat_arr, ASDF_ARRAY_STORAGE_INLINE);
    void *mat_data = asdf_ndarray_data_alloc_temp(file, &mat_arr);

    if (!mat_data)
        goto cleanup;

    memcpy(mat_data, affine->matrix, (size_t)affine->base.n_inputs * affine->base.n_inputs * sizeof(double));

    asdf_value_t *mat_val = asdf_value_of_ndarray(file, &mat_arr);

    if (!mat_val)
        goto cleanup;

    err = asdf_mapping_set(map, "matrix", mat_val);

    if (ASDF_IS_ERR(err)) {
        asdf_value_destroy(mat_val);
        goto cleanup;
    }

    uint64_t tr_shape[1] = {affine->base.n_inputs};
    asdf_ndarray_t tr_arr = {
        .ndim = 1,
        .shape = tr_shape,
        .datatype = {.type = ASDF_DATATYPE_FLOAT64},
        .byteorder = ASDF_BYTEORDER_LITTLE,
    };

    asdf_ndarray_storage_set(&tr_arr, ASDF_ARRAY_STORAGE_INLINE);
    void *tr_data = asdf_ndarray_data_alloc_temp(file, &tr_arr);

    if (!tr_data)
        goto cleanup;

    memcpy(tr_data, affine->translation, (size_t)affine->base.n_inputs * sizeof(double));

    asdf_value_t *tr_val = asdf_value_of_ndarray(file, &tr_arr);

    if (!tr_val)
        goto cleanup;

    err = asdf_mapping_set(map, "translation", tr_val);

    if (ASDF_IS_ERR(err)) {
        asdf_value_destroy(tr_val);
        goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_affine_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_affine_t *affine = (asdf_gwcs_affine_t *)value;
    asdf_gwcs_transform_clean(&affine->base);
    free(affine->matrix);
    free(affine->translation);
    free(affine);
}


ASDF_REGISTER_EXTENSION(
    gwcs_affine,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "affine-1.4.0",
    asdf_gwcs_affine_t,
    &libasdf_software,
    asdf_gwcs_affine_serialize,
    asdf_gwcs_affine_deserialize,
    NULL,
    asdf_gwcs_affine_dealloc,
    NULL);
