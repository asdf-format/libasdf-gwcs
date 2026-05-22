#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../gwcs.h"
#include "../util.h"
#include "spherical_cartesian.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_spherical_cartesian_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_spherical_cartesian_t *sc = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    const char *transform_type = NULL;
    uint64_t wrap_lon_at = 180;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    sc = calloc(1, sizeof(asdf_gwcs_spherical_cartesian_t));

    if (!sc) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &sc->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(
        map, "transform_type", ASDF_VALUE_STRING, NULL, (void *)&transform_type);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (strcmp(transform_type, "spherical_to_cartesian") == 0) {
        sc->direction = ASDF_GWCS_SPHERICAL_TO_CARTESIAN;
        asdf_gwcs_transform_arity_set(&sc->base, asdf_value_file(value), 2, 3);
    } else if (strcmp(transform_type, "cartesian_to_spherical") == 0) {
        sc->direction = ASDF_GWCS_CARTESIAN_TO_SPHERICAL;
        asdf_gwcs_transform_arity_set(&sc->base, asdf_value_file(value), 3, 2);
    } else {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    err = asdf_get_optional_property(map, "wrap_lon_at", ASDF_VALUE_UINT64, NULL, &wrap_lon_at);

    if (ASDF_IS_OPTIONAL_OK(err))
        sc->wrap_lon_at = (double)wrap_lon_at;
    else
        sc->wrap_lon_at = 180.0;

    *out = sc;
    err = ASDF_VALUE_OK;
cleanup:
    if (ASDF_IS_ERR(err))
        asdf_gwcs_spherical_cartesian_destroy(sc);

    return err;
}


static asdf_value_t *asdf_gwcs_spherical_cartesian_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_spherical_cartesian_t *sc = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &sc->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    const char *type_str = sc->direction == ASDF_GWCS_SPHERICAL_TO_CARTESIAN
                               ? "spherical_to_cartesian"
                               : "cartesian_to_spherical";

    err = asdf_mapping_set_string0(map, "transform_type", type_str);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_mapping_set_uint64(map, "wrap_lon_at", (uint64_t)sc->wrap_lon_at);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_spherical_cartesian_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_spherical_cartesian_t *sc = (asdf_gwcs_spherical_cartesian_t *)value;
    asdf_gwcs_transform_clean(&sc->base);
    free(sc);
}


ASDF_GWCS_REGISTER_TRANSFORM(
    spherical_cartesian,
    SPHERICAL_CARTESIAN,
    ASDF_GWCS_TAG_PREFIX "spherical_cartesian-1.3.0",
    asdf_gwcs_spherical_cartesian_t,
    &libasdf_gwcs_software,
    asdf_gwcs_spherical_cartesian_serialize,
    asdf_gwcs_spherical_cartesian_deserialize,
    NULL,
    asdf_gwcs_spherical_cartesian_dealloc,
    NULL);
