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
    asdf_gwcs_identity_t *identity = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    identity = calloc(1, sizeof(asdf_gwcs_identity_t));

    if (!identity) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &identity->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    uint64_t n_dims = 0;
    err = asdf_get_optional_property(map, "n_dims", ASDF_VALUE_UINT64, NULL, &n_dims);

    if (ASDF_IS_OPTIONAL_OK(err))
        asdf_gwcs_transform_arity_set(
            &identity->base, asdf_value_file(value), (uint32_t)n_dims, (uint32_t)n_dims);

    *out = identity;
    err = ASDF_VALUE_OK;
cleanup:
    if (ASDF_IS_ERR(err))
        asdf_gwcs_identity_destroy(identity);

    return err;
}


static asdf_value_t *asdf_gwcs_identity_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_identity_t *identity = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &identity->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (identity->n_inputs > 0) {
        err = asdf_mapping_set_uint64(map, "n_dims", identity->n_inputs);

        if (ASDF_IS_ERR(err))
            goto cleanup;
    }

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_identity_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_identity_t *identity = (asdf_gwcs_identity_t *)value;
    asdf_gwcs_transform_clean(&identity->base);
    free(identity);
}


ASDF_GWCS_REGISTER_TRANSFORM(
    identity,
    IDENTITY,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "identity-1.3.0",
    asdf_gwcs_identity_t,
    &libasdf_gwcs_software,
    asdf_gwcs_identity_serialize,
    asdf_gwcs_identity_deserialize,
    NULL,
    asdf_gwcs_identity_dealloc,
    NULL);
