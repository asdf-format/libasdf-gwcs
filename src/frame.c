#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <asdf/extension_util.h>
#include <asdf/value.h>

#include "frame.h"
#include "gwcs.h"
#include "util.h"

#ifdef ASDF_LOGGING_ENABLED
static inline void warn_invalid_frame_axes_param(
    asdf_value_t *value,
    const char *propname,
    asdf_value_type_t expected_type,
    uint32_t min_axes,
    uint32_t max_axes) {
    const char *path = asdf_value_path(value);

    if (min_axes == max_axes)
        ASDF_LOG(
            value->file,
            ASDF_LOG_WARN,
            "property %s in %s must be a %d element sequence of %s",
            propname,
            path,
            max_axes,
            asdf_value_type_string(expected_type));
    else
        ASDF_LOG(
            value->file,
            ASDF_LOG_WARN,
            "property %s in %s must be a %d to %d element sequence of %s",
            propname,
            path,
            min_axes max_axes,
            asdf_value_type_string(expected_type));
}
#else
static inline void warn_invalid_frame_axes_param(
    UNUSED(asdf_value_t *value),
    UNUSED(const char *propname),
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    UNUSED(asdf_value_type_t expected_type),
    UNUSED(uint32_t min_axes),
    UNUSED(uint32_t max_axes)) {
}
#endif


static asdf_value_err_t get_frame_axes_string_param(
    asdf_mapping_t *value,
    const char *propname,
    char **strings,
    uint32_t min_axes,
    uint32_t max_axes) {
    asdf_sequence_t *frames_seq = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    err = asdf_get_optional_property(
        value, propname, ASDF_VALUE_SEQUENCE, NULL, (void *)&frames_seq);

    if (!ASDF_IS_OPTIONAL_OK(err))
        goto cleanup;

    if (!frames_seq) {
        // Property is absent (optional), nothing to parse
        err = ASDF_VALUE_OK;
        goto cleanup;
    }

    uint32_t size = (uint32_t)asdf_sequence_size(frames_seq);
    asdf_value_t *mapping_val = asdf_value_of_mapping(value);

    if (size < min_axes || size > max_axes) {
        warn_invalid_frame_axes_param(mapping_val, propname, ASDF_VALUE_STRING, min_axes, max_axes);
        goto cleanup;
    }

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(frames_seq);
    char **str_tmp = strings;
    while (asdf_sequence_iter_next(&iter)) {
        const char *s = NULL;
        if (!ASDF_IS_OK(asdf_value_as_string0(iter->value, &s))) {
            warn_invalid_frame_axes_param(
                mapping_val, propname, ASDF_VALUE_STRING, min_axes, max_axes);
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }
        *str_tmp = strdup(s);
        if (!*str_tmp) {
            err = ASDF_VALUE_ERR_OOM;
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }
        str_tmp++;
    }

    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(frames_seq);
    return err;
}


static asdf_value_err_t get_frame_axes_order_param(
    asdf_mapping_t *value, uint32_t *ints, uint32_t min_axes, uint32_t max_axes) {
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_sequence_t *axes_seq = NULL;
    assert(ints);

    err = asdf_get_optional_property(
        value, "axes_order", ASDF_VALUE_SEQUENCE, NULL, (void *)&axes_seq);

    if (!ASDF_IS_OPTIONAL_OK(err))
        goto cleanup;

    if (!axes_seq) {
        // Property is absent (optional), nothing to parse
        err = ASDF_VALUE_OK;
        goto cleanup;
    }

    uint32_t size = (uint32_t)asdf_sequence_size(axes_seq);
    asdf_value_t *mapping_val = asdf_value_of_mapping(value);

    if (size < min_axes || size > max_axes) {
        warn_invalid_frame_axes_param(
            mapping_val, "axes_order", ASDF_VALUE_UINT32, min_axes, max_axes);
        goto cleanup;
    }

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(axes_seq);
    uint32_t *int_tmp = ints;
    while (asdf_sequence_iter_next(&iter)) {
        if (ASDF_VALUE_OK != asdf_value_as_uint32(iter->value, int_tmp)) {
            warn_invalid_frame_axes_param(
                mapping_val, "axes_order", ASDF_VALUE_UINT32, min_axes, max_axes);
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }

        if (*int_tmp >= max_axes) {
            warn_invalid_frame_axes_param(
                mapping_val, "axes_order", ASDF_VALUE_UINT32, min_axes, max_axes);
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }

        int_tmp++;
    }

    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(axes_seq);
    return err;
}


asdf_value_err_t asdf_gwcs_frame_parse(
    asdf_value_t *value, asdf_gwcs_frame_t *frame, asdf_gwcs_frame_common_params_t *params) {
    assert(value);
    assert(frame);
    assert(params);
    asdf_mapping_t *frame_map = NULL;
    const char *name = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    if (asdf_value_as_mapping(value, &frame_map) != ASDF_VALUE_OK)
        goto failure;

    err = asdf_get_required_property(frame_map, "name", ASDF_VALUE_STRING, NULL, (void *)&name);

    if (ASDF_IS_ERR(err))
        goto failure;

    if (name) {
        frame->name = strdup(name);

        if (UNLIKELY(!frame->name)) {
            err = ASDF_VALUE_ERR_OOM;
            goto failure;
        }
    }

    if (!ASDF_IS_OPTIONAL_OK(get_frame_axes_string_param(
            frame_map, "axes_names", params->axes_names, params->min_axes, params->max_axes)))
        goto failure;

    if (!ASDF_IS_OPTIONAL_OK(get_frame_axes_string_param(
            frame_map, "unit", params->unit, params->min_axes, params->max_axes)))
        goto failure;

    if (!ASDF_IS_OPTIONAL_OK(get_frame_axes_string_param(
            frame_map,
            "axis_physical_types",
            params->axis_physical_types,
            params->min_axes,
            params->max_axes)))
        goto failure;

    if (!(ASDF_IS_OPTIONAL_OK(get_frame_axes_order_param(
            frame_map, params->axes_order, params->min_axes, params->max_axes))))
        goto failure;

    if (params->reference_frame) {
        asdf_value_t *rf_val = asdf_mapping_get(frame_map, "reference_frame");
        if (rf_val) {
            asdf_gwcs_baseframe_t *baseframe = NULL;
            if (asdf_value_as_gwcs_coordinate_frame(rf_val, &baseframe) == ASDF_VALUE_OK)
                *params->reference_frame = baseframe;
            asdf_value_destroy(rf_val);
        }
    }

    return ASDF_VALUE_OK;
failure:
    return err;
}


asdf_value_err_t asdf_gwcs_frame_serialize_common(
    asdf_file_t *file,
    const char *name,
    uint32_t naxes,
    const char *const *axes_names,
    const uint32_t *axes_order,
    const char *const *unit,
    const char *const *axis_physical_types,
    const asdf_gwcs_baseframe_t *reference_frame,
    asdf_mapping_t *map) {
    asdf_value_err_t err;

    err = asdf_mapping_set_string0(map, "name", name ? name : "");

    if (ASDF_IS_ERR(err))
        return err;

    if (naxes > 0 && axes_names && axes_names[0]) {
        asdf_sequence_t *seq = asdf_sequence_of_string0(file, axes_names, (int)naxes);

        if (!seq)
            return ASDF_VALUE_ERR_OOM;

        asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);
        err = asdf_mapping_set_sequence(map, "axes_names", seq);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(seq);
            return err;
        }
    }

    if (naxes > 0 && axes_order) {
        asdf_sequence_t *seq = asdf_sequence_of_uint32(file, axes_order, (int)naxes);

        if (!seq)
            return ASDF_VALUE_ERR_OOM;

        asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);
        err = asdf_mapping_set_sequence(map, "axes_order", seq);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(seq);
            return err;
        }
    }

    if (naxes > 0 && unit && unit[0]) {
        asdf_sequence_t *seq = asdf_sequence_of_string0(file, unit, (int)naxes);

        if (!seq)
            return ASDF_VALUE_ERR_OOM;

        asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);
        err = asdf_mapping_set_sequence(map, "unit", seq);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(seq);
            return err;
        }
    }

    if (naxes > 0 && axis_physical_types && axis_physical_types[0]) {
        asdf_sequence_t *seq = asdf_sequence_of_string0(file, axis_physical_types, (int)naxes);

        if (!seq)
            return ASDF_VALUE_ERR_OOM;

        asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);
        err = asdf_mapping_set_sequence(map, "axis_physical_types", seq);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(seq);
            return err;
        }
    }

    if (reference_frame) {
        asdf_value_t *rf = asdf_gwcs_coordinate_frame_value_of(file, reference_frame);
        if (!rf)
            return ASDF_VALUE_ERR_OOM;
        err = asdf_mapping_set(map, "reference_frame", rf);
        if (ASDF_IS_ERR(err)) {
            asdf_value_destroy(rf);
            return err;
        }
    }

    return ASDF_VALUE_OK;
}


static asdf_value_err_t asdf_gwcs_base_frame_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_frame_t *frame = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    frame = calloc(1, sizeof(asdf_gwcs_frame_t));

    if (!frame) {
        err = ASDF_VALUE_ERR_OOM;
        goto failure;
    }

    asdf_gwcs_frame_common_params_t params = {0};

    if (ASDF_VALUE_OK != asdf_gwcs_frame_parse(value, frame, &params))
        goto failure;

    *out = frame;
    return ASDF_VALUE_OK;
failure:
    asdf_gwcs_base_frame_destroy(frame);
    return err;
}


static asdf_value_t *asdf_gwcs_base_frame_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_frame_t *frame = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_frame_serialize_common(
        file, frame->name, 0, NULL, NULL, NULL, NULL, NULL, map);

    if (ASDF_IS_ERR(err)) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


void asdf_gwcs_base_frame_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_frame_t *frame = (asdf_gwcs_frame_t *)value;
    free((char *)frame->name);
    frame->name = NULL;
}


bool asdf_gwcs_base_frame_copy_impl(UNUSED(asdf_file_t *file), const void *src, void *dst) {
    const asdf_gwcs_frame_t *frame = (const asdf_gwcs_frame_t *)src;
    asdf_gwcs_frame_t *copy = dst;

    if (frame->name) {
        copy->name = strdup(frame->name);

        if (UNLIKELY(!copy->name))
            return false;
    }

    copy->type = frame->type;
    return true;
}


void asdf_gwcs_frame_cleanup_axes(
    uint32_t naxes, char **axes_names, char **unit, char **axis_physical_types) {
    for (uint32_t idx = 0; idx < naxes; idx++) {
        if (axes_names)
            free(axes_names[idx]);
        if (unit)
            free(unit[idx]);
        if (axis_physical_types)
            free(axis_physical_types[idx]);
    }
}


/**
 * Generic constructor for frames of different types from a value, depending
 * on the tag
 *
 * .. todo::
 *
 *   This should get a frame "subclass" registration system as done with
 *   transforms and astropy coordinates frames.  At the moment we only really
 *   use celestial frame so that is deferred for later.
 */
asdf_value_err_t asdf_value_as_gwcs_frame(asdf_value_t *value, asdf_gwcs_frame_t **out) {
    // TODO: It will be useful in the future to have some registry of known frame
    // extension types.  Because there are only two currently it's hard-coded for now,
    // but this is a bit ugly...
    asdf_gwcs_frame_t *frame = NULL;
    asdf_gwcs_frame2d_t *frame2d = NULL;
    asdf_gwcs_frame_celestial_t *frame_celestial = NULL;

    if (ASDF_VALUE_OK == asdf_value_as_gwcs_frame2d(value, &frame2d)) {
        frame = (asdf_gwcs_frame_t *)frame2d;
        assert(frame);
    } else if (ASDF_VALUE_OK == asdf_value_as_gwcs_frame_celestial(value, &frame_celestial)) {
        frame = (asdf_gwcs_frame_t *)frame_celestial;
        assert(frame);
    } else if (ASDF_VALUE_OK != asdf_value_as_gwcs_base_frame(value, &frame)) {
        frame = NULL;
    }

    if (frame) {
        *out = frame;
        return ASDF_VALUE_OK;
    }

    return ASDF_VALUE_ERR_TYPE_MISMATCH;
}


/*
 * Frames are dispatched on an enum rather than an extension token, so unlike
 * transforms and coordinate frames there is nowhere to hang a tag-derived name;
 * the schema names are spelled out here until frames get the registration
 * system asdf_value_as_gwcs_frame's TODO describes.
 */
const char *asdf_gwcs_frame_type_name(const asdf_gwcs_frame_t *frame) {
    if (!frame)
        return NULL;

    switch (frame->type) {
    case ASDF_GWCS_FRAME_2D:
        return "frame2d";
    case ASDF_GWCS_FRAME_CELESTIAL:
        return "celestial_frame";
    case ASDF_GWCS_FRAME_GENERIC:
    default:
        return "frame";
    }
}


asdf_value_t *asdf_gwcs_frame_value_of(asdf_file_t *file, const asdf_gwcs_frame_t *frame) {
    if (!frame)
        return NULL;

    switch (frame->type) {
    case ASDF_GWCS_FRAME_2D:
        return asdf_value_of_gwcs_frame2d(file, (const asdf_gwcs_frame2d_t *)frame);
    case ASDF_GWCS_FRAME_CELESTIAL:
        return asdf_value_of_gwcs_frame_celestial(file, (const asdf_gwcs_frame_celestial_t *)frame);
    case ASDF_GWCS_FRAME_GENERIC:
    default:
        return asdf_value_of_gwcs_base_frame(file, (const asdf_gwcs_frame_t *)frame);
    }
}


bool asdf_gwcs_frame_copy_into(asdf_file_t *file, const asdf_gwcs_frame_t *frame,
                               asdf_gwcs_frame_t *copy) {
    if (UNLIKELY(!frame || !copy))
        return false;

    /* The per-type copy_into methods copy the base fields themselves (via
     * asdf_gwcs_base_frame_copy_impl), so dispatch straight to them. */
    switch (frame->type) {
    case ASDF_GWCS_FRAME_2D:
        return asdf_gwcs_frame2d_copy_into(
            file, (const asdf_gwcs_frame2d_t *)frame, (asdf_gwcs_frame2d_t *)copy);
    case ASDF_GWCS_FRAME_CELESTIAL:
        return asdf_gwcs_frame_celestial_copy_into(
            file, (const asdf_gwcs_frame_celestial_t *)frame, (asdf_gwcs_frame_celestial_t *)copy);
    case ASDF_GWCS_FRAME_GENERIC:
    default:
        return asdf_gwcs_base_frame_copy_into(file, frame, copy);
    }
}


/**
 * Generic copy for frames of different types from a value, depending on the tag
 */
asdf_gwcs_frame_t *asdf_gwcs_frame_copy(asdf_file_t *file, const asdf_gwcs_frame_t *frame) {
    if (!frame)
        return NULL;

    switch (frame->type) {
    case ASDF_GWCS_FRAME_2D:
        return (asdf_gwcs_frame_t *)asdf_gwcs_frame2d_copy(
            file, (const asdf_gwcs_frame2d_t *)frame);
    case ASDF_GWCS_FRAME_CELESTIAL:
        return (asdf_gwcs_frame_t *)asdf_gwcs_frame_celestial_copy(
            file, (const asdf_gwcs_frame_celestial_t *)frame);
    case ASDF_GWCS_FRAME_GENERIC:
    default:
        return asdf_gwcs_base_frame_copy(file, frame);
    }
}




/**
 * Generic destructor for frames of different types from a value,
 * depending on the tag
 */
void asdf_gwcs_frame_deinit(asdf_gwcs_frame_t *frame) {
    if (!frame)
        return;

    switch (frame->type) {
    case ASDF_GWCS_FRAME_2D:
        asdf_gwcs_frame2d_deinit((asdf_gwcs_frame2d_t *)frame);
        break;
    case ASDF_GWCS_FRAME_CELESTIAL:
        asdf_gwcs_frame_celestial_deinit((asdf_gwcs_frame_celestial_t *)frame);
        break;
    case ASDF_GWCS_FRAME_GENERIC:
        asdf_gwcs_base_frame_deinit(frame);
        break;
    }
}


void asdf_gwcs_frame_destroy(asdf_gwcs_frame_t *frame) {
    asdf_gwcs_frame_deinit(frame);
    free(frame);
}


static const asdf_extension_vtab_t asdf_gwcs_base_frame_vtab = {
    .serialize = asdf_gwcs_base_frame_serialize,
    .deserialize = asdf_gwcs_base_frame_deserialize,
    .copy = asdf_gwcs_base_frame_copy_impl,
    .deinit = asdf_gwcs_base_frame_deinit_impl,
};


/**
 * Register the GWCS frame tag extension
 *
 * NOTE: The only differences between the schema versions are the underlying
 * baseframe schema version.
 */
ASDF_REGISTER_EXTENSION(
    gwcs_base_frame,
    asdf_gwcs_frame_t,
    &libasdf_software,
    &asdf_gwcs_base_frame_vtab,
    NULL,
    ASDF_GWCS_TAG_PREFIX "frame-1.2.0",
    ASDF_GWCS_TAG_PREFIX "frame-1.1.0",
    ASDF_GWCS_TAG_PREFIX "frame-1.0.0"
);
