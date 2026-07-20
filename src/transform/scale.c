#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "scale.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_scale_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_scale_t *scale = *out;
    asdf_mapping_t *map = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        return ASDF_VALUE_ERR_PARSE_FAILURE;

    asdf_value_err_t err =
        asdf_get_required_property(map, "factor", ASDF_VALUE_DOUBLE, NULL, &scale->factor);

    if (ASDF_IS_ERR(err))
        return err;

    asdf_gwcs_transform_arity_set(&scale->base, asdf_value_file(value), 1, 1);
    return ASDF_VALUE_OK;
}


static asdf_value_t *asdf_gwcs_scale_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_scale_t *scale = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    if (ASDF_IS_ERR(asdf_mapping_set_double(map, "factor", scale->factor))) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


static const asdf_extension_vtab_t asdf_gwcs_scale_vtab = {
    .serialize = asdf_gwcs_scale_serialize,
    .deserialize = asdf_gwcs_scale_deserialize,
    /* copy and deinit are not needed; scale is a shallow object */
    .copy = NULL,
    .deinit = NULL,
};


/**
 * Register scale transform extensions
 *
 * NOTE: The only differences so far between scale schema versions is in the
 * base transform schema version; nominally all versions are supported.
 */
ASDF_GWCS_REGISTER_TRANSFORM(
    scale,
    SCALE,
    asdf_gwcs_scale_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_scale_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale-1.0.0"
);
