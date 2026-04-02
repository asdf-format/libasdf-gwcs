#include <stdbool.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "gwcs.h"
#include "step.h"
#include "transform.h"
#include "util.h"


static asdf_value_err_t asdf_gwcs_step_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_step_t *step = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_value_t *prop = NULL;
    asdf_mapping_t *step_map = NULL;
    asdf_gwcs_frame_t *frame = NULL;
    asdf_gwcs_transform_t *transform = NULL;
    const asdf_file_t *file = asdf_value_file(value);

    if (asdf_value_as_mapping(value, &step_map) != ASDF_VALUE_OK)
        goto failure;

    // gwcs_step is usually not found on its own, but as a sequence of steps
    // in a gwcs, which is normally pre-allocated.
    // If *out is non-null then don't allocate
    if (*out) {
        step = (asdf_gwcs_step_t *)*out;
        step->free = false;
    } else {
        step = calloc(1, sizeof(asdf_gwcs_step_t));
        step->free = true;
    }

    if (!step) {
        err = ASDF_VALUE_ERR_OOM;
        goto failure;
    }

    err = asdf_get_required_property(step_map, "frame", ASDF_VALUE_UNKNOWN, NULL, (void *)&prop);

    if (ASDF_IS_OK(err)) {
        err = asdf_value_as_gwcs_frame(prop, &frame);
        asdf_value_destroy(prop);
    }

    if (ASDF_IS_ERR(err) || !frame) {
#ifdef ASDF_LOG_ENABLED
        const char *path = asdf_value_path(value);
        ASDF_LOG(file, ASDF_LOG_WARN, "invalid frame value for step at %s", path);
#endif
        goto failure;
    }

    step->frame = frame;

    err = asdf_get_optional_property(
        step_map, "transform", ASDF_VALUE_UNKNOWN, NULL, (void *)&prop);

    if (ASDF_IS_OK(err)) {
        // transform may be null
        if (!asdf_value_is_null(prop))
            err = asdf_value_as_gwcs_transform(prop, &transform);

        asdf_value_destroy(prop);
    }

    if (ASDF_IS_ERR(err)) {
#ifdef ASDF_LOG_ENABLED
        const char *path = asdf_value_path(value);
        ASDF_LOG(file, ASDF_LOG_WARN, "invalid transform value for step at %s", path);
#endif
        goto failure;
    }

    step->transform = transform;

    if (!*out)
        *out = step;

    return ASDF_VALUE_OK;
failure:
    asdf_gwcs_transform_destroy(transform);
    if (step && step->free)
        free(step);

    return err;
}


static asdf_value_t *asdf_gwcs_step_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_step_t *step = obj;
    asdf_mapping_t *map = NULL;
    asdf_value_t *value = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_EMIT_FAILURE;

    map = asdf_mapping_create(file);

    if (!map)
        goto cleanup;

    // frame is required
    asdf_value_t *frame_val = asdf_gwcs_frame_value_of(file, step->frame);

    if (!frame_val)
        goto cleanup;

    err = asdf_mapping_set(map, "frame", frame_val);

    if (ASDF_IS_ERR(err)) {
        asdf_value_destroy(frame_val);
        goto cleanup;
    }

    // transform is optional; null means no transform (last step in wcs)
    if (step->transform) {
        asdf_value_t *transform_val = asdf_gwcs_transform_value_of(file, step->transform);

        if (!transform_val)
            goto cleanup;

        err = asdf_mapping_set(map, "transform", transform_val);

        if (ASDF_IS_ERR(err)) {
            asdf_value_destroy(transform_val);
            goto cleanup;
        }
    } else {
        err = asdf_mapping_set_null(map, "transform");

        if (ASDF_IS_ERR(err))
            goto cleanup;
    }

    value = asdf_value_of_mapping(map);
    map = NULL; // owned by value

cleanup:
    asdf_mapping_destroy(map);
    return value;
}


static void asdf_gwcs_step_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_step_t *step = (asdf_gwcs_step_t *)value;

    if (step->frame)
        asdf_gwcs_frame_destroy(step->frame);

    if (step->transform)
        asdf_gwcs_transform_destroy((asdf_gwcs_transform_t *)step->transform);

    if (step->free)
        free(step);
}


ASDF_REGISTER_EXTENSION(
    gwcs_step,
    ASDF_GWCS_TAG_PREFIX "step-1.3.0",
    asdf_gwcs_step_t,
    &libasdf_software,
    asdf_gwcs_step_serialize,
    asdf_gwcs_step_deserialize,
    NULL, /* TODO: copy */
    asdf_gwcs_step_dealloc,
    NULL);
