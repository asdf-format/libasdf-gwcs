#include <asdf/extension_util.h>
#include <asdf/value.h>
#include "util.h"

#include "frame.h"
#include "gwcs.h"


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
        map);

    if (ASDF_IS_ERR(err)) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


static void asdf_gwcs_frame2d_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_base_frame_t *frame = (asdf_gwcs_base_frame_t *)value;
    asdf_gwcs_base_frame_destroy(frame);
}


ASDF_REGISTER_EXTENSION(
    gwcs_frame2d,
    ASDF_GWCS_TAG_PREFIX "frame2d-1.2.0",
    asdf_gwcs_frame2d_t,
    &libasdf_software,
    asdf_gwcs_frame2d_serialize,
    asdf_gwcs_frame2d_deserialize,
    NULL, /* TODO: copy */
    asdf_gwcs_frame2d_dealloc,
    NULL);
