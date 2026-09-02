#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "divide.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_divide_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_divide_t *divide = *out;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *forward_seq = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
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

    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(forward_seq);
    return err;
}


static asdf_value_t *asdf_gwcs_divide_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_divide_t *divide = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

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


static bool asdf_gwcs_divide_copy_impl(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_divide_t *divide = src;
    asdf_gwcs_divide_t *copy = dst;

    if (divide->numerator) {
        copy->numerator = asdf_gwcs_transform_copy(file, divide->numerator);

        if (!copy->numerator)
            return false;
    }

    if (divide->denominator) {
        copy->denominator = asdf_gwcs_transform_copy(file, divide->denominator);

        if (!copy->denominator)
            return false;
    }

    return true;
}


static void asdf_gwcs_divide_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_divide_t *divide = (asdf_gwcs_divide_t *)value;
    asdf_gwcs_transform_destroy(divide->numerator);
    divide->numerator = NULL;
    asdf_gwcs_transform_destroy(divide->denominator);
    divide->denominator = NULL;
}


/* A divide keeps its two operands in named members rather than a list, so its
 * sub-transforms are reported by role. */
static uint32_t asdf_gwcs_divide_children(
    const asdf_gwcs_transform_t *transform, uint32_t index, asdf_gwcs_transform_iter_t *out) {
    const asdf_gwcs_divide_t *divide = (const asdf_gwcs_divide_t *)transform;

    if (out) {
        switch (index) {
        case 0:
            out->value = divide->numerator;
            out->role = "numerator";
            break;
        case 1:
            out->value = divide->denominator;
            out->role = "denominator";
            break;
        default:
            break;
        }
    }

    return 2;
}


static const asdf_extension_vtab_t asdf_gwcs_divide_vtab = {
    .serialize = asdf_gwcs_divide_serialize,
    .deserialize = asdf_gwcs_divide_deserialize,
    .copy = asdf_gwcs_divide_copy_impl,
    .deinit = asdf_gwcs_divide_deinit_impl,
};


/**
 * Register divide transform extensions
 *
 * NOTE: The only differences so far between divide schema versions is in the
 * base transform schema version; nominally all versions are supported.
 */
ASDF_GWCS_REGISTER_TRANSFORM_WITH_CHILDREN(
    divide,
    DIVIDE,
    asdf_gwcs_divide_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_divide_vtab,
    asdf_gwcs_divide_children,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "divide-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "divide-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "divide-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "divide-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "divide-1.0.0"
);
