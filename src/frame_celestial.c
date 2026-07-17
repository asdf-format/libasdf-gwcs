#include <stdlib.h>
#include <string.h>

#include <asdf/error.h>
#include <asdf/extension.h>
#include <asdf/extension_util.h>
#include <asdf/value.h>

#include "frame.h"
#include "gwcs.h"
#include "util.h"


static asdf_value_err_t asdf_gwcs_frame_celestial_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_frame_celestial_t *frame_celestial = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    frame_celestial = calloc(1, sizeof(asdf_gwcs_frame_celestial_t));

    if (!frame_celestial) {
        err = ASDF_VALUE_ERR_OOM;
        goto failure;
    }

    asdf_gwcs_frame_common_params_t params = {
        .min_axes = 2,
        .max_axes = 3,
        .axes_names = (char **)frame_celestial->axes_names,
        .axes_order = frame_celestial->axes_order,
        .unit = (char **)frame_celestial->unit,
        .axis_physical_types = (char **)frame_celestial->axis_physical_types,
        .reference_frame = &frame_celestial->reference_frame};

    if (ASDF_VALUE_OK !=
        asdf_gwcs_frame_parse(value, (asdf_gwcs_frame_t *)frame_celestial, &params))
        goto failure;

    frame_celestial->base.type = ASDF_GWCS_FRAME_CELESTIAL;
    *out = frame_celestial;
    return ASDF_VALUE_OK;
failure:
    asdf_gwcs_frame_celestial_destroy(frame_celestial);
    return err;
}


static asdf_value_t *asdf_gwcs_frame_celestial_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_frame_celestial_t *frame_celestial = obj;

    uint32_t naxes = 0;

    while (naxes < 3 && frame_celestial->axes_names[naxes])
        naxes++;

    if (naxes < 2)
        naxes = 2;

    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_frame_serialize_common(
        file,
        frame_celestial->base.name,
        naxes,
        frame_celestial->axes_names,
        frame_celestial->axes_order,
        frame_celestial->unit,
        frame_celestial->axis_physical_types,
        frame_celestial->reference_frame,
        map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static bool asdf_gwcs_frame_celestial_copy_impl(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_frame_celestial_t *frame = src;
    asdf_gwcs_frame_celestial_t *copy = dst;

    if (!asdf_gwcs_base_frame_copy_impl(file, src, dst))
        return false;

    for (int idx = 0; idx < 3; idx++) {
        copy->axes_order[idx] = frame->axes_order[idx];

        if (frame->axes_names[idx]) {
            copy->axes_names[idx] = strdup(frame->axes_names[idx]);
            if (UNLIKELY(!copy->axes_names[idx]))
                return false;
        }

        if (frame->unit[idx]) {
            copy->unit[idx] = strdup(frame->unit[idx]);
            if (UNLIKELY(!copy->unit[idx]))
                return false;
        }

        if (frame->axis_physical_types[idx]) {
            copy->axis_physical_types[idx] = strdup(frame->axis_physical_types[idx]);
            if (UNLIKELY(!copy->axis_physical_types[idx]))
                return false;
        }
    }

    if (frame->reference_frame) {
        copy->reference_frame = asdf_gwcs_coordinate_frame_copy(file, frame->reference_frame);

        if (UNLIKELY(!copy->reference_frame))
            return false;
    }

    return true;
}


static void asdf_gwcs_frame_celestial_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_frame_celestial_t *frame = value;
    asdf_gwcs_coordinate_frame_destroy(frame->reference_frame);
    frame->reference_frame = NULL;
    asdf_gwcs_frame_cleanup_axes(
        3,
        (char **)frame->axes_names,
        (char **)frame->unit,
        (char **)frame->axis_physical_types);
    asdf_gwcs_base_frame_deinit_impl(value);
}


static const asdf_extension_vtab_t asdf_gwcs_frame_celestial_vtab = {
    .serialize = asdf_gwcs_frame_celestial_serialize,
    .deserialize = asdf_gwcs_frame_celestial_deserialize,
    .copy = asdf_gwcs_frame_celestial_copy_impl,
    .deinit = asdf_gwcs_frame_celestial_deinit_impl,
};


/**
 * Register celestial frame extensions
 *
 * NOTE: The only differences so far between celestial_frame schema versions
 * are in the base frame schema versions.
 */
ASDF_REGISTER_EXTENSION(
    gwcs_frame_celestial,
    asdf_gwcs_frame_celestial_t,
    &libasdf_software,
    &asdf_gwcs_frame_celestial_vtab,
    NULL,
    ASDF_GWCS_TAG_PREFIX "celestial_frame-1.2.0",
    ASDF_GWCS_TAG_PREFIX "celestial_frame-1.1.0",
    ASDF_GWCS_TAG_PREFIX "celestial_frame-1.0.0"
);
