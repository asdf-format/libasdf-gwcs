#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/file.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "remap_axes.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_remap_axes_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_remap_axes_t *remap = *out;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *mapping_seq = NULL;
    uint32_t *mapping = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
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

    /* n_inputs: explicit value takes precedence; fall back to max(mapping)+1. */
    uint32_t max_input = 0;
    for (int idx = 0; idx < n; idx++) {
        if (remap->mapping[idx] > max_input)
            max_input = remap->mapping[idx];
    }
    uint32_t n_inputs = max_input + 1;
    uint64_t explicit_n_inputs = 0;
    if (asdf_get_optional_property(
            map, "n_inputs", ASDF_VALUE_UINT64, NULL, (void *)&explicit_n_inputs) ==
            ASDF_VALUE_OK &&
        explicit_n_inputs > 0)
        n_inputs = (uint32_t)explicit_n_inputs;
    asdf_gwcs_transform_arity_set(&remap->base, asdf_value_file(value), n_inputs, (uint32_t)n);

    err = ASDF_VALUE_OK;
cleanup:
    free(mapping);
    asdf_sequence_destroy(mapping_seq);
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

    for (uint32_t idx = 0; idx < remap->n_outputs; idx++) {
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

    /* Emit n_inputs only when it exceeds max(mapping)+1; the schema defines
     * n_inputs=max(mapping)+1 as the default, so omitting it is unambiguous
     * otherwise.  Mappings with unused inputs (e.g. mapping=[0,0,0], n_inputs=3)
     * must be explicit or any reader would infer the wrong arity. */
    uint32_t max_mapping = 0;
    for (uint32_t idx = 0; idx < remap->n_outputs; idx++) {
        if (remap->mapping[idx] > max_mapping)
            max_mapping = remap->mapping[idx];
    }

    if (remap->base.n_inputs > max_mapping + 1) {
        err = asdf_mapping_set_uint64(map, "n_inputs", remap->n_inputs);

        if (ASDF_IS_ERR(err))
            goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static bool asdf_gwcs_remap_axes_copy_impl(UNUSED(asdf_file_t *file), const void *src, void *dst) {
    const asdf_gwcs_remap_axes_t *remap_axes = src;
    asdf_gwcs_remap_axes_t *copy = dst;

    if (remap_axes->mapping && remap_axes->n_outputs) {
        size_t n = remap_axes->n_outputs;
        uint32_t *mapping = malloc(n * sizeof(*mapping));

        if (UNLIKELY(!mapping))
            return false;

        memcpy(mapping, remap_axes->mapping, n * sizeof(*mapping));
        copy->mapping = mapping;
    }

    return true;
}


static void asdf_gwcs_remap_axes_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_remap_axes_t *remap = (asdf_gwcs_remap_axes_t *)value;
    free((uint32_t *)remap->mapping);
    remap->mapping = NULL;
}


static const asdf_extension_vtab_t asdf_gwcs_remap_axes_vtab = {
    .serialize = asdf_gwcs_remap_axes_serialize,
    .deserialize = asdf_gwcs_remap_axes_deserialize,
    .copy = asdf_gwcs_remap_axes_copy_impl,
    .deinit = asdf_gwcs_remap_axes_deinit_impl,
};


/**
 * Register remap_axes transform extensions
 *
 * NOTE: Substantively there is a slight difference in the 1.1.0 and 1.2.0
 * versions of this schema that allowed constant-tagged items in the mapping
 * sequence; this was again removed in 1.3.0.  This case is not yet handled
 * but we nominally support the 1.1.0 and 1.2.0 tag versions anyways; not
 * clear if any files exist affected by this.
 */
ASDF_GWCS_REGISTER_TRANSFORM(
    remap_axes,
    REMAP_AXES,
    asdf_gwcs_remap_axes_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_remap_axes_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.0.0"
);
