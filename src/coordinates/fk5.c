/*
 * FK5 coordinate frame (astropy/coordinates/frames/fk5-1.x).
 *
 * The schema defines
 *
 * ``frame_attributes: { equinox: <time> }``,
 *
 * with equinox required.  The equinox is a time/time value of any 1.x version,
 * which libasdf's time extension recognizes across all of them.
 */

#include <stdlib.h>

#include <asdf/core/time.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../gwcs.h"
#include "../util.h"
#include "frame_attributes.h"


static asdf_value_t *fk5_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_fk5_t *fk5 = obj;

    return asdf_gwcs_frame_attributes_serialize(file, fk5->equinox, NULL);
}


static asdf_value_err_t fk5_deserialize(
    asdf_value_t *value, const void *userdata, void **out) {
    const asdf_gwcs_coordinate_frame_data_t *data = userdata;
    asdf_gwcs_fk5_t *fk5 = calloc(1, sizeof(asdf_gwcs_fk5_t));

    if (!fk5)
        return ASDF_VALUE_ERR_OOM;

    if (data)
        fk5->type = data->type;

    asdf_value_err_t err = asdf_gwcs_frame_attributes_parse(value, &fk5->equinox, NULL);

    if (ASDF_IS_ERR(err)) {
        asdf_time_destroy(fk5->equinox);
        free(fk5);
        return err;
    }

    *out = fk5;
    return ASDF_VALUE_OK;
}


static bool fk5_copy(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_fk5_t *fk5 = src;
    asdf_gwcs_fk5_t *copy = dst;

    copy->equinox = NULL;

    if (fk5->equinox) {
        copy->equinox = asdf_time_copy(file, fk5->equinox);

        if (UNLIKELY(!copy->equinox))
            return false;
    }

    return true;
}


static void fk5_deinit(void *obj) {
    asdf_gwcs_fk5_t *fk5 = obj;

    if (!fk5)
        return;

    asdf_time_destroy(fk5->equinox);
    fk5->equinox = NULL;
}


static const asdf_extension_vtab_t fk5_vtab = {
    .serialize = fk5_serialize,
    .deserialize = fk5_deserialize,
    .copy = fk5_copy,
    .deinit = fk5_deinit,
};


/**
 * Register fk5 frame extensions
 *
 * NOTE: The only differences so far between fk5 schema versions are
 * in the baseframe schema versions.
 */
ASDF_GWCS_REGISTER_COORDINATE_FRAME(
    fk5,
    FK5,
    asdf_gwcs_fk5_t,
    &libasdf_gwcs_software,
    &fk5_vtab,
    NULL,
    ASDF_COORDINATES_TAG_PREFIX "fk5-1.2.0",
    ASDF_COORDINATES_TAG_PREFIX "fk5-1.1.0",
    ASDF_COORDINATES_TAG_PREFIX "fk5-1.0.0"
)
