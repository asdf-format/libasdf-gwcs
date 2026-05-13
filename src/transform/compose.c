#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../transform.h"
#include "../util.h"
#include "compose.h"


static asdf_value_err_t asdf_gwcs_compose_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_compose_t *compose = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *forward_seq = NULL;
    asdf_gwcs_transform_t **forward = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    compose = calloc(1, sizeof(asdf_gwcs_compose_t));

    if (!compose) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &compose->base);

    if (ASDF_IS_ERR(err))
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

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(forward_seq);

    while (asdf_sequence_iter_next(&iter)) {
        err = asdf_value_as_gwcs_transform(iter->value, &forward[iter->index]);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }
    }

    compose->n_forward = (uint32_t)n;
    compose->forward = forward;
    forward = NULL;

    /* n_inputs from last sub-transform, n_outputs from first */
    asdf_gwcs_transform_arity_set(
        &compose->base,
        asdf_value_file(value),
        compose->forward[n - 1]->n_inputs,
        compose->forward[0]->n_outputs);

    *out = compose;
    err = ASDF_VALUE_OK;
cleanup:
    if (forward) {
        for (int idx = 0; idx < (int)(compose ? compose->n_forward : 0); idx++)
            asdf_gwcs_transform_destroy(forward[idx]);

        free(forward);
    }

    asdf_sequence_destroy(forward_seq);

    if (ASDF_IS_ERR(err))
        asdf_gwcs_compose_destroy(compose);

    return err;
}


static asdf_value_t *asdf_gwcs_compose_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_compose_t *compose = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &compose->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    asdf_sequence_t *seq = asdf_sequence_create(file);

    if (!seq)
        goto cleanup;

    for (uint32_t idx = 0; idx < compose->n_forward; idx++) {
        asdf_value_t *t_val = asdf_value_of_gwcs_transform(file, compose->forward[idx]);

        if (!t_val) {
            asdf_sequence_destroy(seq);
            goto cleanup;
        }

        err = asdf_sequence_append(seq, t_val);

        if (ASDF_IS_ERR(err)) {
            asdf_value_destroy(t_val);
            asdf_sequence_destroy(seq);
            goto cleanup;
        }
    }

    err = asdf_mapping_set_sequence(map, "forward", seq);

    if (ASDF_IS_ERR(err)) {
        asdf_sequence_destroy(seq);
        goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_compose_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_compose_t *compose = (asdf_gwcs_compose_t *)value;
    asdf_gwcs_transform_clean(&compose->base);

    if (compose->forward) {
        for (uint32_t idx = 0; idx < compose->n_forward; idx++)
            asdf_gwcs_transform_destroy(compose->forward[idx]);

        free(compose->forward);
    }

    free(compose);
}


ASDF_REGISTER_EXTENSION(
    gwcs_compose,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "compose-1.3.0",
    asdf_gwcs_compose_t,
    &libasdf_software,
    asdf_gwcs_compose_serialize,
    asdf_gwcs_compose_deserialize,
    NULL,
    asdf_gwcs_compose_dealloc,
    NULL);
