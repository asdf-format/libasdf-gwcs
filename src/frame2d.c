#include <stdlib.h>

#include <asdf/extension_util.h>
#include <asdf/value.h>

#include "frame.h"
#include "gwcs.h"
#include "util.h"


static asdf_value_err_t asdf_gwcs_frame2d_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_frame2d_t *frame2d = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    frame2d = calloc(1, sizeof(asdf_gwcs_frame2d_t));

    if (!frame2d) {
        err = ASDF_VALUE_ERR_OOM;
        goto failure;
    }

    asdf_gwcs_frame_common_params_t params = {
        .min_axes = 2,
        .max_axes = 2,
        .axes_names = (char **)frame2d->axes_names,
        .axes_order = frame2d->axes_order,
        .unit = (char **)frame2d->unit,
        .axis_physical_types = (char **)frame2d->axis_physical_types};

    if (ASDF_VALUE_OK != asdf_gwcs_frame_parse(value, (asdf_gwcs_frame_t *)frame2d, &params))
        goto failure;

    frame2d->base.type = ASDF_GWCS_FRAME_2D;
    *out = frame2d;
    return ASDF_VALUE_OK;
failure:
    asdf_gwcs_frame2d_destroy(frame2d);
    return err;
}


static asdf_value_t *asdf_gwcs_frame2d_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_frame2d_t *frame2d = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_frame_serialize_common(
        file,
        frame2d->base.name,
        2,
        frame2d->axes_names,
        frame2d->axes_order,
        frame2d->unit,
        frame2d->axis_physical_types,
        NULL,
        map);

    if (ASDF_IS_ERR(err)) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


static bool asdf_gwcs_frame2d_copy_impl(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_frame2d_t *frame2d = src;
    asdf_gwcs_frame2d_t *copy = dst;

    if (!asdf_gwcs_base_frame_copy_impl(file, src, dst))
        return false;

    for (int idx = 0; idx < 2; idx++) {
        if (frame2d->axes_names[idx]) {
            copy->axes_names[idx] = strdup(frame2d->axes_names[idx]);

            if (UNLIKELY(!copy->axes_names[idx]))
                return false;
        }

        if (frame2d->unit[idx]) {
            copy->unit[idx] = strdup(frame2d->unit[idx]);

            if (UNLIKELY(!copy->unit[idx]))
                return false;
        }

        if (frame2d->axis_physical_types[idx]) {
            copy->axis_physical_types[idx] = strdup(frame2d->axis_physical_types[idx]);

            if (UNLIKELY(!copy->axis_physical_types[idx]))
                return false;
        }

        copy->axes_order[idx] = frame2d->axes_order[idx];
    }

    return true;
}


static void asdf_gwcs_frame2d_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_frame2d_t *frame2d = (asdf_gwcs_frame2d_t *)value;
    asdf_gwcs_frame_cleanup_axes(
        2,
        (char **)frame2d->axes_names,
        (char **)frame2d->unit,
        (char **)frame2d->axis_physical_types);
    asdf_gwcs_base_frame_deinit_impl(value);
}


static const asdf_extension_vtab_t asdf_gwcs_frame2d_vtab = {
    .serialize = asdf_gwcs_frame2d_serialize,
    .deserialize = asdf_gwcs_frame2d_deserialize,
    .copy = asdf_gwcs_frame2d_copy_impl,
    .deinit = asdf_gwcs_frame2d_deinit_impl,
};


/**
 * Register frame2d extensions
 *
 * NOTE: The only differences so far between frame2d schema versions is in the
 * base frame schema version; nominally all versions are supported.
 */
ASDF_REGISTER_EXTENSION(
    gwcs_frame2d,
    asdf_gwcs_frame2d_t,
    &libasdf_software,
    &asdf_gwcs_frame2d_vtab,
    NULL,
    ASDF_GWCS_TAG_PREFIX "frame2d-1.2.0",
    ASDF_GWCS_TAG_PREFIX "frame2d-1.1.0",
    ASDF_GWCS_TAG_PREFIX "frame2d-1.0.0"
);
