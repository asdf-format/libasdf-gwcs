#include <stdlib.h>

#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "fitswcs_imaging.h"
#include "gwcs.h"
#include "step.h"
#include "util.h"


static asdf_gwcs_err_t asdf_gwcs_finalize_fitswcs_imaging(const asdf_file_t *file, asdf_gwcs_t *gwcs) {
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
    const asdf_file_t *file = asdf_value_file(value);

    if (asdf_value_as_mapping(value, &gwcs_map) != ASDF_VALUE_OK)
        goto cleanup;

    gwcs = calloc(1, sizeof(asdf_gwcs_t));

    if (!gwcs) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    // The name property is required
    err = asdf_get_required_property(
        gwcs_map, "name", ASDF_VALUE_STRING, NULL, (void *)&gwcs->name);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    // TODO: Implement pixel_shape parsing

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

    asdf_sequence_iter_t iter = asdf_sequence_iter_init();
    asdf_value_t *item = NULL;
    asdf_gwcs_step_t *step_tmp = steps;
    while ((item = asdf_sequence_iter(steps_seq, &iter)) != NULL) {
        if (ASDF_VALUE_OK != asdf_value_as_gwcs_step(item, &step_tmp))
            goto cleanup;

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


static void asdf_gwcs_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_t *gwcs = (asdf_gwcs_t *)value;

    if (gwcs->steps) {
        for (uint32_t idx = 0; idx < gwcs->n_steps; idx++) {
            asdf_gwcs_step_t *step = (asdf_gwcs_step_t *)&gwcs->steps[idx];
            asdf_gwcs_step_destroy(step);
        }
        free((asdf_gwcs_step_t *)gwcs->steps);
    }

    free(gwcs);
}


ASDF_REGISTER_EXTENSION(
    gwcs,
    ASDF_GWCS_TAG_PREFIX "wcs-1.4.0",
    asdf_gwcs_t,
    &libasdf_software,
    asdf_gwcs_serialize,
    asdf_gwcs_deserialize,
    NULL, /* TODO: copy */
    asdf_gwcs_dealloc,
    NULL);
