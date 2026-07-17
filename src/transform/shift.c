#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "shift.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_shift_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_shift_t *shift = *out;
    asdf_mapping_t *map = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        return ASDF_VALUE_ERR_PARSE_FAILURE;

    asdf_value_err_t err =
        asdf_get_required_property(map, "offset", ASDF_VALUE_DOUBLE, NULL, &shift->offset);

    if (ASDF_IS_ERR(err))
        return err;

    asdf_gwcs_transform_arity_set(&shift->base, asdf_value_file(value), 1, 1);
    return ASDF_VALUE_OK;
}


static asdf_value_t *asdf_gwcs_shift_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_shift_t *shift = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &shift->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_mapping_set_double(map, "offset", shift->offset);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static const asdf_extension_vtab_t asdf_gwcs_shift_vtab = {
    .serialize = asdf_gwcs_shift_serialize,
    .deserialize = asdf_gwcs_shift_deserialize,
    /* .copy and .deinit not needed as shift is shallow */
    .copy = NULL,
    .deinit = NULL,
};


/**
 * Register shift transform extensions
 *
 * NOTE: The only differences so far between shift schema versions is in the
 * base transform schema version; nominally all versions are supported.
 */
ASDF_GWCS_REGISTER_TRANSFORM(
    shift,
    SHIFT,
    asdf_gwcs_shift_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_shift_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift-1.0.0"
);
