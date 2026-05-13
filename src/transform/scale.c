#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../util.h"
#include "scale.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_scale_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_scale_t *scale = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    scale = calloc(1, sizeof(asdf_gwcs_scale_t));

    if (!scale) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &scale->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(map, "factor", ASDF_VALUE_DOUBLE, NULL, &scale->factor);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    asdf_gwcs_transform_arity_set(&scale->base, asdf_value_file(value), 1, 1);

    *out = scale;
    err = ASDF_VALUE_OK;
cleanup:
    if (ASDF_IS_ERR(err))
        asdf_gwcs_scale_destroy(scale);

    return err;
}


static asdf_value_t *asdf_gwcs_scale_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_scale_t *scale = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &scale->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_mapping_set_double(map, "factor", scale->factor);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_scale_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_scale_t *scale = (asdf_gwcs_scale_t *)value;
    asdf_gwcs_transform_clean(&scale->base);
    free(scale);
}


ASDF_REGISTER_EXTENSION(
    gwcs_scale,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale-1.3.0",
    asdf_gwcs_scale_t,
    &libasdf_software,
    asdf_gwcs_scale_serialize,
    asdf_gwcs_scale_deserialize,
    NULL,
    asdf_gwcs_scale_dealloc,
    NULL);
