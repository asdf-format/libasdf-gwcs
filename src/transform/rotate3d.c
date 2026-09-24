#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/file.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "rotate3d.h"
#include "transform.h"


#define ASDF_GWCS_ROTATE3D_DEFAULT_DIRECTION "native2celestial"


/* The `direction` enum from the rotate3d schema: the twelve Euler axis
 * orders, plus the two spherical conventions. */
static const char *const asdf_gwcs_rotate3d_directions[] = {
    "zxz", "xyx", "yzy", "zyz", "xzx", "yxy", "xyz", "yzx", "zxy", "xzy", "zyx", "yxz",
    "native2celestial", "celestial2native", NULL};


static bool asdf_gwcs_rotate3d_direction_valid(const char *direction) {
    for (const char *const *dir = asdf_gwcs_rotate3d_directions; *dir; dir++) {
        if (strcmp(*dir, direction) == 0)
            return true;
    }

    return false;
}


static asdf_value_err_t asdf_gwcs_rotate3d_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_rotate3d_t *rot = *out;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    const char *direction = NULL;
    char *direction_copy = NULL;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    err = asdf_get_required_property(map, "phi", ASDF_VALUE_DOUBLE, NULL, &rot->phi);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(map, "theta", ASDF_VALUE_DOUBLE, NULL, &rot->theta);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(map, "psi", ASDF_VALUE_DOUBLE, NULL, &rot->psi);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_optional_property(
        map, "direction", ASDF_VALUE_STRING, NULL, (void *)&direction);

    if (!ASDF_IS_OPTIONAL_OK(err))
        goto cleanup;

    if (!direction)
        direction = ASDF_GWCS_ROTATE3D_DEFAULT_DIRECTION;

    if (!asdf_gwcs_rotate3d_direction_valid(direction)) {
        ASDF_LOG(
            asdf_value_file(value),
            ASDF_LOG_WARN,
            "invalid rotate3d direction \"%s\"",
            direction);
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    direction_copy = strdup(direction);

    if (!direction_copy) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    rot->direction = direction_copy;
    direction_copy = NULL;

    asdf_gwcs_transform_arity_set(&rot->base, asdf_value_file(value), 2, 2);

    err = ASDF_VALUE_OK;
cleanup:
    free(direction_copy);
    return err;
}


static asdf_value_t *asdf_gwcs_rotate3d_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_rotate3d_t *rot = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    if (ASDF_IS_ERR(asdf_mapping_set_double(map, "phi", rot->phi)))
        goto cleanup;

    if (ASDF_IS_ERR(asdf_mapping_set_double(map, "theta", rot->theta)))
        goto cleanup;

    if (ASDF_IS_ERR(asdf_mapping_set_double(map, "psi", rot->psi)))
        goto cleanup;

    const char *direction =
        rot->direction ? rot->direction : ASDF_GWCS_ROTATE3D_DEFAULT_DIRECTION;

    if (ASDF_IS_ERR(asdf_mapping_set_string0(map, "direction", direction)))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static bool asdf_gwcs_rotate3d_copy_impl(UNUSED(asdf_file_t *file), const void *src, void *dst) {
    const asdf_gwcs_rotate3d_t *rot = src;
    asdf_gwcs_rotate3d_t *copy = dst;

    copy->phi = rot->phi;
    copy->theta = rot->theta;
    copy->psi = rot->psi;
    copy->direction = NULL;

    if (rot->direction) {
        copy->direction = strdup(rot->direction);

        if (UNLIKELY(!copy->direction))
            return false;
    }

    return true;
}


static void asdf_gwcs_rotate3d_deinit_impl(void *value) {
    if (!value)
        return;

    asdf_gwcs_rotate3d_t *rot = (asdf_gwcs_rotate3d_t *)value;
    free((char *)rot->direction);
    rot->direction = NULL;
}


static const asdf_extension_vtab_t asdf_gwcs_rotate3d_vtab = {
    .serialize = asdf_gwcs_rotate3d_serialize,
    .deserialize = asdf_gwcs_rotate3d_deserialize,
    .copy = asdf_gwcs_rotate3d_copy_impl,
    .deinit = asdf_gwcs_rotate3d_deinit_impl,
};


/**
 * Register rotate3d transform extensions
 *
 * NOTE: The only differences between rotate3d schema versions are in the
 * base transform schema version.
 */
ASDF_GWCS_REGISTER_TRANSFORM(
    rotate3d,
    ROTATE3D,
    asdf_gwcs_rotate3d_t,
    &libasdf_gwcs_software,
    &asdf_gwcs_rotate3d_vtab,
    NULL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate3d-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate3d-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate3d-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate3d-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate3d-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate3d-1.0.0"
);
