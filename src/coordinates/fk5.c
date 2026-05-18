/*
 * FK5 coordinate frame (astropy/coordinates/frames/fk5-1.0.0).
 *
 * The schema defines frame_attributes: { equinox: <time> } (required).
 * Full round-trip is deferred pending libasdf support for multi-version tag
 * resolution (time-1.1.0 vs time-1.2.0).  For now we serialize
 * frame_attributes: {} and ignore equinox on read.
 */

#include <stdlib.h>

#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../gwcs.h"
#include "../util.h"


static asdf_value_t *fk5_serialize(
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


static asdf_value_err_t fk5_deserialize(
    UNUSED(asdf_value_t *value), UNUSED(const void *userdata), void **out) {

    *out = calloc(1, sizeof(asdf_gwcs_baseframe_t));
    return *out ? ASDF_VALUE_OK : ASDF_VALUE_ERR_OOM;
}

static void fk5_dealloc(void *value) {
    free(value);
}


ASDF_GWCS_REGISTER_COORDINATE_FRAME(
    fk5,
    FK5,
    ASDF_COORDINATES_TAG_PREFIX "fk5-1.0.0",
    asdf_gwcs_baseframe_t,
    &libasdf_gwcs_software,
    fk5_serialize,
    fk5_deserialize,
    NULL,
    fk5_dealloc,
    NULL)
