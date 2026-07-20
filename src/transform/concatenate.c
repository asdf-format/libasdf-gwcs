#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "concatenate.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_concatenate_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_concatenate_t *concat = *out;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *forward_seq = NULL;
    asdf_gwcs_transform_t **forward = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    err = asdf_get_required_property(
        map, "forward", ASDF_VALUE_SEQUENCE, NULL, (void *)&forward_seq);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    int n = asdf_sequence_size(forward_seq);

    if (n < 0) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    forward = calloc((size_t)n, sizeof(asdf_gwcs_transform_t *));

    if (!forward) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    concat->n_forward = (uint32_t)n;
    concat->forward = forward;

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(forward_seq);

    while (asdf_sequence_iter_next(&iter)) {
        err = asdf_value_as_gwcs_transform(iter->value, &forward[iter->index]);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }
    }

    uint32_t sum_inputs = 0, sum_outputs = 0;
    for (int idx = 0; idx < n; idx++) {
        sum_inputs += concat->forward[idx]->n_inputs;
        sum_outputs += concat->forward[idx]->n_outputs;
    }
    asdf_gwcs_transform_arity_set(&concat->base, asdf_value_file(value), sum_inputs, sum_outputs);

    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(forward_seq);
    return err;
}


static asdf_value_t *asdf_gwcs_concatenate_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_concatenate_t *concat = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_sequence_t *seq = asdf_sequence_create(file);

    if (!seq)
        goto cleanup;

    for (uint32_t idx = 0; idx < concat->n_forward; idx++) {
        asdf_value_t *t_val = asdf_value_of_gwcs_transform(file, concat->forward[idx]);

        if (!t_val) {
            asdf_sequence_destroy(seq);
            goto cleanup;
        }

        if (ASDF_IS_ERR(asdf_sequence_append(seq, t_val))) {
            asdf_value_destroy(t_val);
            asdf_sequence_destroy(seq);
            goto cleanup;
        }
    }

    if (ASDF_IS_ERR(asdf_mapping_set_sequence(map, "forward", seq))) {
        asdf_sequence_destroy(seq);
        goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static bool asdf_gwcs_concatenate_copy_impl(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_concatenate_t *concatenate = src;
    asdf_gwcs_concatenate_t *copy = dst;

    if (concatenate->n_forward) {
        copy->forward = calloc(concatenate->n_forward, sizeof(*(concatenate->forward)));

        if (UNLIKELY(!copy->forward))
            return false;

        copy->n_forward = concatenate->n_forward;
    }

    for (uint32_t idx = 0; idx < concatenate->n_forward; idx++) {
        asdf_gwcs_transform_t *forward = concatenate->forward[idx];

        if (UNLIKELY(!forward))
            continue;

        copy->forward[idx] = asdf_gwcs_transform_copy(file, forward);

        if (UNLIKELY(!copy->forward[idx]))
            return false;
    }

    return true;
}


static void asdf_gwcs_concatenate_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_concatenate_t *concat = (asdf_gwcs_concatenate_t *)value;

    if (concat->forward) {
        for (uint32_t idx = 0; idx < concat->n_forward; idx++)
            asdf_gwcs_transform_destroy(concat->forward[idx]);

        free(concat->forward);
        concat->forward = NULL;
    }
}


static const asdf_extension_vtab_t asdf_gwcs_concatenate_vtab = {
    .serialize = asdf_gwcs_concatenate_serialize,
    .deserialize = asdf_gwcs_concatenate_deserialize,
    .copy = asdf_gwcs_concatenate_copy_impl,
    .deinit = asdf_gwcs_concatenate_deinit_impl,
};


/**
 * Register concatenate transform extensions
 *
 * NOTE: The only differences so far between concatenate schema versions is in
 * the base transform schema version; nominally all versions are supported.
 */
ASDF_GWCS_REGISTER_TRANSFORM(
    concatenate,
    CONCATENATE,
    asdf_gwcs_concatenate_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_concatenate_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate-1.0.0"
);
