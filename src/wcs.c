#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/error.h>
#include <asdf/extension.h>
#include <asdf/extension_util.h>
#include <asdf/file.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "fitswcs_imaging.h"
#include "gwcs.h"
#include "step.h"
#include "util.h"


asdf_version_t libasdf_gwcs_version = {0};


asdf_software_t libasdf_gwcs_software = {
    .name = PACKAGE_NAME,
    .version = &libasdf_gwcs_version,
    .homepage = PACKAGE_URL,
    .author = "The libasdf Developers"};


static asdf_gwcs_err_t asdf_gwcs_finalize_fitswcs_imaging(
    const asdf_file_t *file, asdf_gwcs_t *gwcs) {
    assert(gwcs->n_steps == 2);
    const asdf_gwcs_step_t *step0 = &gwcs->steps[0];
    assert(step0->transform && step0->transform->type == ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING);
    asdf_gwcs_fits_t *fits = (asdf_gwcs_fits_t *)step0->transform;
    return asdf_gwcs_fits_get_ctype(file, gwcs, &fits->ctype[0], &fits->ctype[1]);
}


static asdf_value_t *asdf_gwcs_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_t *gwcs = obj;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *steps_seq = NULL;
    asdf_value_t *value = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_EMIT_FAILURE;

    map = asdf_mapping_create(file);

    if (!map)
        goto cleanup;

    err = asdf_mapping_set_string0(map, "name", gwcs->name ? gwcs->name : "");

    if (ASDF_IS_ERR(err))
        goto cleanup;

    /* pixel_shape is optional; write it only when the WCS actually carries one. */
    if (gwcs->pixel_shape && gwcs->pixel_ndim > 0) {
        asdf_sequence_t *shape_seq =
            asdf_sequence_of_uint64(file, gwcs->pixel_shape, (int)gwcs->pixel_ndim);

        if (!shape_seq) {
            err = ASDF_VALUE_ERR_OOM;
            goto cleanup;
        }

        err = asdf_mapping_set_sequence(map, "pixel_shape", shape_seq);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(shape_seq);
            goto cleanup;
        }
        /* shape_seq is owned by map now */
    }

    steps_seq = asdf_sequence_create(file);

    if (!steps_seq)
        goto cleanup;

    for (uint32_t idx = 0; idx < gwcs->n_steps; idx++) {
        asdf_value_t *step_val = asdf_value_of_gwcs_step(file, &gwcs->steps[idx]);

        if (!step_val)
            goto cleanup;

        err = asdf_sequence_append(steps_seq, step_val);

        if (ASDF_IS_ERR(err)) {
            asdf_value_destroy(step_val);
            goto cleanup;
        }
    }

    err = asdf_mapping_set_sequence(map, "steps", steps_seq);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    steps_seq = NULL; // owned by map

    value = asdf_value_of_mapping(map);
    map = NULL; // owned by value

cleanup:
    asdf_sequence_destroy(steps_seq);
    asdf_mapping_destroy(map);
    return value;
}


static asdf_value_err_t asdf_gwcs_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_t *gwcs = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *gwcs_map = NULL;
    asdf_sequence_t *steps_seq = NULL;
    char *name = NULL;
    const asdf_file_t *file = asdf_value_file(value);

    if (asdf_value_as_mapping(value, &gwcs_map) != ASDF_VALUE_OK)
        goto cleanup;

    gwcs = calloc(1, sizeof(asdf_gwcs_t));

    if (!gwcs) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    // The name property is required
    err = asdf_get_required_property(gwcs_map, "name", ASDF_VALUE_STRING, NULL, (void *)&name);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    gwcs->name = strdup(name);

    if (!gwcs->name) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    /* pixel_shape is optional (added in wcs-1.2.0) and explicitly nullable, so
     * fetch the raw value and check for null rather than asking for a sequence
     * outright, which would warn on a legitimate "pixel_shape: null".
     *
     * asdf_value_as_sequence yields a *view* of pixel_shape_val, so only the
     * value itself is released here. */
    asdf_value_t *pixel_shape_val = asdf_mapping_get(gwcs_map, "pixel_shape");

    if (pixel_shape_val && !asdf_value_is_null(pixel_shape_val)) {
        asdf_sequence_t *pixel_shape_seq = NULL;
        int ndim = 0;

        if (asdf_value_as_sequence(pixel_shape_val, &pixel_shape_seq) != ASDF_VALUE_OK)
            err = ASDF_VALUE_ERR_TYPE_MISMATCH;
        else if ((ndim = asdf_sequence_size(pixel_shape_seq)) < 0)
            err = ASDF_VALUE_ERR_PARSE_FAILURE;
        else {
            uint64_t *pixel_shape = calloc((size_t)ndim, sizeof(uint64_t));

            if (!pixel_shape) {
                err = ASDF_VALUE_ERR_OOM;
            } else {
                gwcs->pixel_shape = pixel_shape;
                gwcs->pixel_ndim = (uint32_t)ndim;

                asdf_sequence_iter_t *ps_iter = asdf_sequence_iter_init(pixel_shape_seq);

                while (asdf_sequence_iter_next(&ps_iter)) {
                    uint64_t dim = 0;

                    if (asdf_value_as_uint64(ps_iter->value, &dim) != ASDF_VALUE_OK) {
                        asdf_sequence_iter_destroy(ps_iter);
                        err = ASDF_VALUE_ERR_TYPE_MISMATCH;
                        break;
                    }

                    pixel_shape[ps_iter->index] = dim;
                }
            }
        }

        if (ASDF_IS_ERR(err)) {
            asdf_value_destroy(pixel_shape_val);
            goto cleanup;
        }
    }

    asdf_value_destroy(pixel_shape_val);

    // Parse steps
    err = asdf_get_required_property(
        gwcs_map, "steps", ASDF_VALUE_SEQUENCE, NULL, (void *)&steps_seq);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    int n_steps = asdf_sequence_size(steps_seq);

    if (n_steps < 0)
        goto cleanup;

    gwcs->n_steps = (uint32_t)n_steps;

    asdf_gwcs_step_t *steps = calloc(n_steps, sizeof(asdf_gwcs_step_t));

    if (!steps) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    gwcs->steps = steps;

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(steps_seq);
    asdf_gwcs_step_t *step_tmp = steps;
    while (asdf_sequence_iter_next(&iter)) {
        if (ASDF_VALUE_OK != asdf_value_as_gwcs_step(iter->value, &step_tmp)) {
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }

        step_tmp++;
    }

    // Special finalization step in the case of a FITS WCS, to fill in the
    // ctype parameters.  Maybe useful to have such hooks for other GWCS
    // instances, but the fitswcs_imaging is the only case I can think of
    // at the moment
    if (asdf_gwcs_is_fits(file, gwcs)) {
        asdf_gwcs_err_t gwcs_err = asdf_gwcs_finalize_fitswcs_imaging(file, gwcs);
        // Just log a warning for now
        if (ASDF_GWCS_OK != gwcs_err) {
#ifdef ASDF_LOG_ENABLED
            const char *path = asdf_value_path(value);
            ASDF_LOG(
                file,
                ASDF_LOG_WARN,
                "failure to finalize fitswcs_imaging transform in gwcs at %s",
                path);
#endif
        }
    }

    *out = gwcs;
    err = ASDF_VALUE_OK;
cleanup:
    asdf_sequence_destroy(steps_seq);

    if (err != ASDF_VALUE_OK)
        asdf_gwcs_destroy(gwcs);

    return err;
}


static bool asdf_gwcs_copy_impl(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_t *gwcs = src;
    asdf_gwcs_t *copy = dst;

    if (gwcs->name) {
        copy->name = strdup(gwcs->name);
        if (!copy->name)
            return false;
    }

    if (gwcs->pixel_shape && gwcs->pixel_ndim > 0) {
        uint64_t *pixel_shape = calloc(gwcs->pixel_ndim, sizeof(uint64_t));

        if (UNLIKELY(!pixel_shape))
            return false;

        memcpy(pixel_shape, gwcs->pixel_shape, gwcs->pixel_ndim * sizeof(uint64_t));
        copy->pixel_shape = pixel_shape;
        copy->pixel_ndim = gwcs->pixel_ndim;
    }

    if (gwcs->steps) {
        asdf_gwcs_step_t *steps = calloc(gwcs->n_steps, sizeof(asdf_gwcs_step_t));

        if (UNLIKELY(!steps))
            return false;

        copy->steps = steps;
        copy->n_steps = gwcs->n_steps;

        for (uint32_t idx = 0; idx < gwcs->n_steps; idx++) {
            if (!asdf_gwcs_step_copy_into(file, &gwcs->steps[idx], &steps[idx]))
                return false;
        }
    }

    return copy;
}


static void asdf_gwcs_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_t *gwcs = (asdf_gwcs_t *)value;

    if (gwcs->name)
        free((char *)gwcs->name);

    if (gwcs->pixel_shape)
        free((uint64_t *)gwcs->pixel_shape);

    if (gwcs->steps) {
        /* steps is a contiguous array: deinit each element, then free the
         * array once (asdf_gwcs_step_destroy would free each interior pointer). */
        for (uint32_t idx = 0; idx < gwcs->n_steps; idx++) {
            asdf_gwcs_step_t *step = (asdf_gwcs_step_t *)&gwcs->steps[idx];
            asdf_gwcs_step_deinit(step);
        }
        free((asdf_gwcs_step_t *)gwcs->steps);
    }

    ZERO_MEMORY(gwcs, sizeof(asdf_gwcs_t));
}


static const asdf_extension_vtab_t asdf_gwcs_vtab = {
    .serialize = asdf_gwcs_serialize,
    .deserialize = asdf_gwcs_deserialize,
    .copy = asdf_gwcs_copy_impl,
    .deinit = asdf_gwcs_deinit_impl,
};


/**
 * Register the wcs tag extension
 *
 * NOTE: The main difference between versions for now is the expected version
 * of step property tag; this is not yet fully accounted for.  The optional
 * pixel_shape property was only added in version 1.2.0 of the schema; it is
 * read and written when present, and omitted otherwise, so it is harmless for
 * the earlier versions that do not define it.
 */
ASDF_REGISTER_EXTENSION(
    gwcs,
    asdf_gwcs_t,
    &libasdf_software,
    &asdf_gwcs_vtab,
    NULL,
    ASDF_GWCS_TAG_PREFIX "wcs-1.4.0",
    ASDF_GWCS_TAG_PREFIX "wcs-1.3.0",
    ASDF_GWCS_TAG_PREFIX "wcs-1.2.0",
    ASDF_GWCS_TAG_PREFIX "wcs-1.1.0",
    ASDF_GWCS_TAG_PREFIX "wcs-1.0.0"
);


ASDF_CONSTRUCTOR static void asdf_gwcs_version_init() {
    asdf_version_t *version = asdf_version_parse(PACKAGE_VERSION);
    libasdf_gwcs_version.version = version->version;
    libasdf_gwcs_version.major = version->major;
    libasdf_gwcs_version.minor = version->minor;
    libasdf_gwcs_version.patch = version->patch;
    libasdf_gwcs_version.extra = version->extra;
    free(version);
}


ASDF_DESTRUCTOR static void asdf_gwcs_version_destroy() {
    free((void *)libasdf_gwcs_version.version);
    free((void *)libasdf_gwcs_version.extra);
}
