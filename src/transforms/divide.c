#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../transform.h"
#include "../util.h"
#include "divide.h"


static asdf_value_err_t asdf_gwcs_divide_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_divide_t *divide = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *forward_seq = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    divide = calloc(1, sizeof(asdf_gwcs_divide_t));

    if (!divide) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &divide->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(
        map, "forward", ASDF_VALUE_SEQUENCE, NULL, (void *)&forward_seq);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (asdf_sequence_size(forward_seq) != 2) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(forward_seq);

    while (asdf_sequence_iter_next(&iter)) {
        asdf_gwcs_transform_t **target = iter->index == 0 ? &divide->numerator
                                                          : &divide->denominator;
        err = asdf_value_as_gwcs_transform(iter->value, target);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }
    }

    *out = divide;
    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(forward_seq);

    if (ASDF_IS_ERR(err))
        asdf_gwcs_divide_destroy(divide);

    return err;
}


static asdf_value_t *asdf_gwcs_divide_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_divide_t *divide = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &divide->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    asdf_sequence_t *seq = asdf_sequence_create(file);

    if (!seq)
        goto cleanup;

    const asdf_gwcs_transform_t *parts[2] = {divide->numerator, divide->denominator};

    for (int idx = 0; idx < 2; idx++) {
        asdf_value_t *t_val = asdf_value_of_gwcs_transform(file, parts[idx]);

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


static void asdf_gwcs_divide_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_divide_t *divide = (asdf_gwcs_divide_t *)value;
    asdf_gwcs_transform_clean(&divide->base);
    asdf_gwcs_transform_destroy(divide->numerator);
    asdf_gwcs_transform_destroy(divide->denominator);
    free(divide);
}


ASDF_REGISTER_EXTENSION(
    gwcs_divide,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "divide-1.3.0",
    asdf_gwcs_divide_t,
    &libasdf_software,
    asdf_gwcs_divide_serialize,
    asdf_gwcs_divide_deserialize,
    NULL,
    asdf_gwcs_divide_dealloc,
    NULL);
