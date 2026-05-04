#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../transform.h"
#include "../util.h"
#include "remap_axes.h"


static asdf_value_err_t asdf_gwcs_remap_axes_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_remap_axes_t *remap = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *mapping_seq = NULL;
    uint32_t *mapping = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    remap = calloc(1, sizeof(asdf_gwcs_remap_axes_t));

    if (!remap) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &remap->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(
        map, "mapping", ASDF_VALUE_SEQUENCE, NULL, (void *)&mapping_seq);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    int n = asdf_sequence_size(mapping_seq);

    if (n < 0) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    mapping = calloc((size_t)n, sizeof(uint32_t));

    if (!mapping) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(mapping_seq);

    while (asdf_sequence_iter_next(&iter)) {
        uint32_t v = 0;
        err = asdf_value_as_uint32(iter->value, &v);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }

        mapping[iter->index] = v;
    }

    remap->mapping = mapping;
    mapping = NULL;

    /* Compute implicit n_inputs = max(mapping) + 1, n_outputs = n */
    uint32_t max_input = 0;
    for (int idx = 0; idx < n; idx++) {
        if (remap->mapping[idx] > max_input)
            max_input = remap->mapping[idx];
    }
    asdf_gwcs_transform_arity_set(
        &remap->base, asdf_value_file(value), max_input + 1, (uint32_t)n);

    *out = remap;
    err = ASDF_VALUE_OK;
cleanup:
    free(mapping);
    asdf_sequence_destroy(mapping_seq);

    if (ASDF_IS_ERR(err))
        asdf_gwcs_remap_axes_destroy(remap);

    return err;
}


static asdf_value_t *asdf_gwcs_remap_axes_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_remap_axes_t *remap = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &remap->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    asdf_sequence_t *seq = asdf_sequence_create(file);

    if (!seq)
        goto cleanup;

    asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);

    for (uint32_t idx = 0; idx < remap->base.n_outputs; idx++) {
        err = asdf_sequence_append_uint32(seq, remap->mapping[idx]);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(seq);
            goto cleanup;
        }
    }

    err = asdf_mapping_set_sequence(map, "mapping", seq);

    if (ASDF_IS_ERR(err)) {
        asdf_sequence_destroy(seq);
        goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_remap_axes_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_remap_axes_t *remap = (asdf_gwcs_remap_axes_t *)value;
    asdf_gwcs_transform_clean(&remap->base);
    free((uint32_t *)remap->mapping);
    free(remap);
}


ASDF_REGISTER_EXTENSION(
    gwcs_remap_axes,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.4.0",
    asdf_gwcs_remap_axes_t,
    &libasdf_software,
    asdf_gwcs_remap_axes_serialize,
    asdf_gwcs_remap_axes_deserialize,
    NULL,
    asdf_gwcs_remap_axes_dealloc,
    NULL);
