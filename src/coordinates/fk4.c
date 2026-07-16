/*
 * FK4 and FK4NoETerms coordinate frames.
 *
 * Both schemas define frame_attributes: { equinox: <time>, obstime: <time> }
 * with equinox required.  Full round-trip is deferred pending libasdf support
 * for multi-version tag resolution (time-1.1.0 vs time-1.2.0).  For now we
 * serialize frame_attributes: {} and ignore attributes on read.
 */

#include <stdlib.h>

#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../gwcs.h"
#include "../util.h"


static asdf_value_t *fk4_serialize(
    asdf_file_t *file, UNUSED(const void *obj), UNUSED(const void *userdata)) {

    asdf_mapping_t *map = asdf_mapping_create(file);
    if (!map)
        return NULL;

    asdf_mapping_t *attrs = asdf_mapping_create(file);
    if (!attrs) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    asdf_value_err_t err = asdf_mapping_set_mapping(map, "frame_attributes", attrs);
    if (ASDF_IS_ERR(err)) {
        asdf_mapping_destroy(attrs);
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


static asdf_value_err_t fk4_deserialize(
    UNUSED(asdf_value_t *value), UNUSED(const void *userdata), void **out) {

    *out = calloc(1, sizeof(asdf_gwcs_baseframe_t));
    return *out ? ASDF_VALUE_OK : ASDF_VALUE_ERR_OOM;
}


static const asdf_extension_vtab_t fk4_vtab = {
    .serialize = fk4_serialize,
    .deserialize = fk4_deserialize,
    /* .copy and .deinit not needed as fk4 is shallow */
    .copy = NULL,
    .deinit = NULL,
};


/**
 * Register fk4 frame extensions
 *
 * NOTE: The only differences so far between fk4 schema versions are in the
 * baseframe schema versions.
 */
ASDF_GWCS_REGISTER_COORDINATE_FRAME(
    fk4,
    FK4,
    asdf_gwcs_baseframe_t,
    &libasdf_gwcs_software,
    &fk4_vtab,
    NULL,
    ASDF_COORDINATES_TAG_PREFIX "fk4-1.2.0",
    ASDF_COORDINATES_TAG_PREFIX "fk4-1.1.0",
    ASDF_COORDINATES_TAG_PREFIX "fk4-1.0.0"
)

/**
 * Register fk4noeterms frame extensions
 *
 * NOTE: The only differences so far between fk4noeterms schema versions are
 * in the baseframe schema versions.  There is also no differences in the
 * schemas between fk4 and fk4noterms; the different tags are merely identify
 * different frame identities.
 */
ASDF_GWCS_REGISTER_COORDINATE_FRAME(
    fk4noeterms,
    FK4_NO_E,
    asdf_gwcs_baseframe_t,
    &libasdf_gwcs_software,
    &fk4_vtab,
    NULL,
    ASDF_COORDINATES_TAG_PREFIX "fk4noeterms-1.2.0",
    ASDF_COORDINATES_TAG_PREFIX "fk4noeterms-1.1.0",
    ASDF_COORDINATES_TAG_PREFIX "fk4noeterms-1.0.0"
)
