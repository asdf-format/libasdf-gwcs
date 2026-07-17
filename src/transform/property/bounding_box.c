#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/error.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../../gwcs.h"
#include "../../util.h"
#include "asdf/gwcs/transform/property/bounding_box.h"


/** Helper to parse bounding box intervals from mapping items */
static asdf_value_err_t asdf_gwcs_interval_parse(
    const char *key, asdf_value_t *bounds, asdf_gwcs_interval_t *out) {
    asdf_sequence_t *bounds_seq = NULL;
    asdf_value_t *bound_val = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    out->input_name = strdup(key);

    if (asdf_value_as_sequence(bounds, &bounds_seq) != ASDF_VALUE_OK)
        goto cleanup;


    if (asdf_sequence_size(bounds_seq) != 2)
        goto cleanup;

    for (int idx = 0; idx < 2; idx++) {
        double bound = 0.0;
        bound_val = asdf_sequence_get(bounds_seq, idx);

        if (!bound_val)
            goto cleanup;

        if (asdf_value_as_double(bound_val, &bound))
            goto cleanup;

        asdf_value_destroy(bound_val);
        bound_val = NULL;
        out->bounds[idx] = bound;
    }

    err = ASDF_VALUE_OK;
cleanup:
    asdf_value_destroy(bound_val);
    return err;
}


static void asdf_gwcs_interval_cleanup(asdf_gwcs_interval_t *interval) {
    if (!interval)
        return;

    free((void *)interval->input_name);
    ZERO_MEMORY(interval, sizeof(asdf_gwcs_interval_t));
}


static asdf_value_err_t asdf_gwcs_bounding_box_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_bounding_box_t *bounding_box = NULL;
    asdf_mapping_t *intervals_map = NULL;
    asdf_gwcs_interval_t *intervals = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *bbox_map = NULL;
    asdf_sequence_t *bounds_seq = NULL;

    if (asdf_value_as_mapping(value, &bbox_map) != ASDF_VALUE_OK)
        goto cleanup;

    bounding_box = calloc(1, sizeof(asdf_gwcs_bounding_box_t));

    if (!bounding_box) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_get_required_property(
        bbox_map, "intervals", ASDF_VALUE_MAPPING, NULL, &intervals_map);

    if (ASDF_VALUE_OK != err)
        goto cleanup;

    int n_intervals = asdf_mapping_size(intervals_map);

    if (n_intervals < 1) {
#ifdef ASDF_LOG_ENABLED
        const asdf_file_t *file = asdf_value_file(value);
        const char *path = asdf_value_path(value);
        ASDF_LOG(file, ASDF_LOG_WARN, "insufficient intervals in bounding_box at %s", path);
#endif
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    intervals = calloc(n_intervals, sizeof(asdf_gwcs_interval_t));

    if (!intervals) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    asdf_mapping_iter_t *iter = asdf_mapping_iter_init(intervals_map);
    asdf_gwcs_interval_t *interval_tmp = intervals;

    while (asdf_mapping_iter_next(&iter)) {
        err = asdf_gwcs_interval_parse(iter->key, iter->value, interval_tmp);

        if (err != ASDF_VALUE_OK) {
            asdf_mapping_iter_destroy(iter);
            goto cleanup;
        }

        interval_tmp++;
    }

    bounding_box->n_intervals = n_intervals;
    bounding_box->intervals = intervals;

    // TODO: Parse order and ignore
    *out = bounding_box;
    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(bounds_seq);
    asdf_mapping_destroy(intervals_map);
    if (err != ASDF_VALUE_OK) {
        free(intervals);
        free(bounding_box);
    }
    return err;
}


static asdf_value_t *asdf_gwcs_bounding_box_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_bounding_box_t *bbox = obj;
    asdf_mapping_t *bbox_map = NULL;
    asdf_mapping_t *intervals_map = NULL;
    asdf_value_t *value = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_EMIT_FAILURE;

    bbox_map = asdf_mapping_create(file);
    if (UNLIKELY(!bbox_map))
        goto cleanup;

    intervals_map = asdf_mapping_create(file);
    if (UNLIKELY(!intervals_map))
        goto cleanup;

    for (uint32_t idx = 0; idx < bbox->n_intervals; idx++) {
        const asdf_gwcs_interval_t *interval = &bbox->intervals[idx];
        asdf_sequence_t *bounds_seq = asdf_sequence_of_double(file, interval->bounds, 2);

        if (!bounds_seq) {
            err = ASDF_VALUE_ERR_OOM;
            goto cleanup;
        }

        asdf_sequence_set_style(bounds_seq, ASDF_YAML_NODE_STYLE_FLOW);

        err = asdf_mapping_set_sequence(intervals_map, interval->input_name, bounds_seq);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(bounds_seq);
            goto cleanup;
        }
    }

    err = asdf_mapping_set_mapping(bbox_map, "intervals", intervals_map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    intervals_map = NULL; // owned by bbox_map now

    value = asdf_value_of_mapping(bbox_map);
    bbox_map = NULL; // owned by value

cleanup:
    asdf_mapping_destroy(intervals_map);
    asdf_mapping_destroy(bbox_map);
    return value;
}


static bool asdf_gwcs_bounding_box_copy_impl(
    UNUSED(asdf_file_t *file), const void *src, void *dst) {

    const asdf_gwcs_bounding_box_t *bounding_box = src;
    asdf_gwcs_bounding_box_t *copy = dst;

    copy->order = bounding_box->order;

    if (bounding_box->n_intervals && bounding_box->intervals) {
        asdf_gwcs_interval_t *intervals = calloc(
            bounding_box->n_intervals, sizeof(asdf_gwcs_interval_t));

        if (UNLIKELY(!intervals))
            return false;

        copy->intervals = intervals;
        copy->n_intervals = bounding_box->n_intervals;

        for (uint32_t idx = 0; idx < bounding_box->n_intervals; idx++) {
            intervals[idx].bounds[0] = bounding_box->intervals[idx].bounds[0];
            intervals[idx].bounds[1] = bounding_box->intervals[idx].bounds[1];

            if (bounding_box->intervals[idx].input_name) {
                intervals[idx].input_name = strdup(bounding_box->intervals[idx].input_name);

                if (UNLIKELY(!intervals[idx].input_name))
                    return false;
            }
        }
    }

    if (bounding_box->ignore) {
        size_t n = 0;
        while (bounding_box->ignore[n])
            n++;

        const char **ignore = calloc(n + 1, sizeof(char *));

        if (UNLIKELY(!ignore))
            return false;

        copy->ignore = ignore;

        for (size_t idx = 0; idx < n; idx++) {
            ignore[idx] = strdup(bounding_box->ignore[idx]);

            if (UNLIKELY(!ignore[idx]))
                return false;
        }
    }

    return true;
}


static void asdf_gwcs_bounding_box_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_bounding_box_t *bounding_box = (asdf_gwcs_bounding_box_t *)value;

    if (bounding_box->ignore) {
        for (uint32_t idx = 0; bounding_box->ignore[idx]; idx++)
            free((void *)bounding_box->ignore[idx]);

        free((void *)bounding_box->ignore);
        bounding_box->ignore = NULL;
    }

    if (bounding_box->intervals) {
        for (uint32_t idx = 0; idx < bounding_box->n_intervals; idx++)
            asdf_gwcs_interval_cleanup((asdf_gwcs_interval_t *)&bounding_box->intervals[idx]);

        free((void *)bounding_box->intervals);
        bounding_box->intervals = NULL;
    }
}


static const asdf_extension_vtab_t asdf_gwcs_bounding_box_vtab = {
    .serialize = asdf_gwcs_bounding_box_serialize,
    .deserialize = asdf_gwcs_bounding_box_deserialize,
    .copy = asdf_gwcs_bounding_box_copy_impl,
    .deinit = asdf_gwcs_bounding_box_deinit_impl,
};


/**
 * Register the bounding_box property tag extension
 *
 * NOTE: This tag is supported nominally, but not all properties are supported
 * yet.  The only major difference between versions of this tag is the versions
 * of referenced quantity schemas.
 */
ASDF_REGISTER_EXTENSION(
    gwcs_bounding_box,
    asdf_gwcs_bounding_box_t,
    &libasdf_software,
    &asdf_gwcs_bounding_box_vtab,
    NULL,
    ASDF_GWCS_BOUNDING_BOX_TAG,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "property/bounding_box-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "property/bounding_box-1.0.0"
);
