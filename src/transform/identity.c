#include <stdint.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "identity.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_identity_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_identity_t *identity = *out;
    asdf_mapping_t *map = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        return ASDF_VALUE_ERR_PARSE_FAILURE;

    uint64_t n_dims = 0;
    asdf_value_err_t err = asdf_get_optional_property(
        map, "n_dims", ASDF_VALUE_UINT64, NULL, &n_dims);

    if (ASDF_IS_OPTIONAL_OK(err) && n_dims)
        asdf_gwcs_transform_arity_set(
            &identity->base, asdf_value_file(value), (uint32_t)n_dims, (uint32_t)n_dims);

    return ASDF_VALUE_OK;
}


static asdf_value_t *asdf_gwcs_identity_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_identity_t *identity = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    if (identity->n_inputs > 0 &&
        ASDF_IS_ERR(asdf_mapping_set_uint64(map, "n_dims", identity->n_inputs))) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


static const asdf_extension_vtab_t asdf_gwcs_identity_vtab = {
    .serialize = asdf_gwcs_identity_serialize,
    .deserialize = asdf_gwcs_identity_deserialize,
    /* Deliberately left NULL--no internal fields to copy/deinit */
    .copy = NULL,
    .deinit = NULL,
};


/**
 * Register identity transform extensions
 *
 * NOTE: The only differences so far between identity schema versions is in
 * the base transform schema version; nominally all versions are supported.
 */
ASDF_GWCS_REGISTER_TRANSFORM(
    identity,
    IDENTITY,
    asdf_gwcs_identity_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_identity_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "identity-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "identity-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "identity-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "identity-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "identity-1.0.0"
);
