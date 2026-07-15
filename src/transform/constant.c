#include <stdint.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "constant.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_constant_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_constant_t *constant = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    uint64_t dimensions = 1;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    constant = calloc(1, sizeof(asdf_gwcs_constant_t));

    if (!constant) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &constant->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(map, "value", ASDF_VALUE_DOUBLE, NULL, &constant->value);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    /* NOTE: In constant "dimensions" is a required property and is said to be "one or two" in
     * the description, though the schema seems to allow for any value (nor practically speaking
     * is there any reason for it to be limited to that -- seems like a bit of shoehorning
     * of Astropy's Const1D and Const2D...*/
    err = asdf_get_optional_property(map, "dimensions", ASDF_VALUE_UINT64, NULL, &dimensions);

    if (err == ASDF_VALUE_ERR_NOT_FOUND) {
        const char *tag_str = asdf_value_tag(value);
        asdf_tag_t *tag = asdf_tag_parse(tag_str);
        if (UNLIKELY(!tag)) {
            // We would not even be on this code path if there was a valid tag
            // unless OOM occurred
            err = ASDF_VALUE_ERR_OOM;
            goto cleanup;
        }

        // TODO: Might be good to have an explicit helper for this type of
        // logic, where a certain properties is required but only at or above
        // a specific tag version...
        if (tag->version->major >= 1 && tag->version->minor >= 4) {
            const char *path = asdf_value_path(value);
            ASDF_LOG(asdf_value_file(value), ASDF_LOG_WARN,
                     "required property 'dimensions' missing "
                     "from %s at %s; assuming dimensions=1", tag_str, path);
        }
        err = ASDF_VALUE_OK;
        asdf_tag_destroy(tag);
    } else if (ASDF_IS_OK(err)) {
        // Maybe should warn if missing since at least in the current version
        // of the schema dimensions is required
        err = ASDF_VALUE_OK;
    } else {
        goto cleanup;
    }

    asdf_gwcs_transform_arity_set(&constant->base, asdf_value_file(value), dimensions, 1);
    *out = constant;
cleanup:
    if (ASDF_IS_ERR(err))
        asdf_gwcs_constant_destroy(constant);

    return err;
}


static asdf_value_t *asdf_gwcs_constant_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_constant_t *constant = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &constant->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_mapping_set_double(map, "value", constant->value);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_mapping_set_uint64(map, "dimensions", constant->n_inputs);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_constant_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_constant_t *constant = (asdf_gwcs_constant_t *)value;
    asdf_gwcs_transform_clean(&constant->base);
    free(constant);
}


static const asdf_extension_vtab_t asdf_gwcs_constant_vtab = {
    .serialize = asdf_gwcs_constant_serialize,
    .deserialize = asdf_gwcs_constant_deserialize,
    .copy = NULL, /* TODO */
    .dealloc = asdf_gwcs_constant_dealloc,
};


/**
 * Register constant transform extensions
 *
 * NOTE: The main substantive difference between versions is the 'dimensions'
 * property (required) was only added in version >=1.4.0; this is accounted
 * for in the deserialize method.
 */

ASDF_GWCS_REGISTER_TRANSFORM(
    constant,
    CONSTANT,
    asdf_gwcs_constant_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_constant_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.6.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "constant-1.0.0"
);
