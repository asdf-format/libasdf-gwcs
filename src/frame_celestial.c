#include <stdlib.h>

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


static void asdf_gwcs_frame_celestial_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_frame_celestial_t *frame = value;
    asdf_gwcs_coordinate_frame_destroy(frame->reference_frame);
    frame->reference_frame = NULL;
    asdf_gwcs_base_frame_destroy((asdf_gwcs_frame_t *)frame);
}


static const asdf_extension_vtab_t asdf_gwcs_frame_celestial_vtab = {
    .serialize = asdf_gwcs_frame_celestial_serialize,
    .deserialize = asdf_gwcs_frame_celestial_deserialize,
    .copy = NULL, /* TODO */
    .dealloc = asdf_gwcs_frame_celestial_dealloc,
};


ASDF_REGISTER_EXTENSION(
    gwcs_frame_celestial,
    asdf_gwcs_frame_celestial_t,
    &libasdf_software,
    &asdf_gwcs_frame_celestial_vtab,
    NULL,
    ASDF_GWCS_TAG_PREFIX "celestial_frame-1.2.0"
);
