/*
 * FK4 and FK4NoETerms coordinate frames.
 *
 * Both schemas define
 *
 * ``frame_attributes: { equinox: <time>, obstime: <time> }``
 *
 * with equinox required, and the two are otherwise identical. the distinct
 * tags merely identify different frame identities.  The times are time/time
 * values of any 1.x version, which libasdf's time extension recognizes across
 * all of them.
 */

#include <stdlib.h>

#include <asdf/core/time.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../gwcs.h"
#include "../util.h"
#include "frame_attributes.h"


static asdf_value_t *fk4_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    const asdf_gwcs_fk4_t *fk4 = obj;

    return asdf_gwcs_frame_attributes_serialize(file, fk4->equinox, fk4->obstime);
}


static asdf_value_err_t fk4_deserialize(
    asdf_value_t *value, const void *userdata, void **out) {
    const asdf_gwcs_coordinate_frame_data_t *data = userdata;
    asdf_gwcs_fk4_t *fk4 = calloc(1, sizeof(asdf_gwcs_fk4_t));

    if (!fk4)
        return ASDF_VALUE_ERR_OOM;

    if (data)
        fk4->type = data->type;

    asdf_value_err_t err = asdf_gwcs_frame_attributes_parse(
        value, &fk4->equinox, &fk4->obstime);

    if (ASDF_IS_ERR(err)) {
        asdf_time_destroy(fk4->equinox);
        asdf_time_destroy(fk4->obstime);
        free(fk4);
        return err;
    }

    *out = fk4;
    return ASDF_VALUE_OK;
}


static bool fk4_copy(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_fk4_t *fk4 = src;
    asdf_gwcs_fk4_t *copy = dst;

    copy->equinox = NULL;
    copy->obstime = NULL;

    if (fk4->equinox) {
        copy->equinox = asdf_time_copy(file, fk4->equinox);

        if (UNLIKELY(!copy->equinox))
            return false;
    }

    if (fk4->obstime) {
        copy->obstime = asdf_time_copy(file, fk4->obstime);

        if (UNLIKELY(!copy->obstime))
            return false;
    }

    return true;
}


static void fk4_deinit(void *obj) {
    asdf_gwcs_fk4_t *fk4 = obj;

    if (!fk4)
        return;

    asdf_time_destroy(fk4->equinox);
    asdf_time_destroy(fk4->obstime);
    fk4->equinox = NULL;
    fk4->obstime = NULL;
}


static const asdf_extension_vtab_t fk4_vtab = {
    .serialize = fk4_serialize,
    .deserialize = fk4_deserialize,
    .copy = fk4_copy,
    .deinit = fk4_deinit,
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
    asdf_gwcs_fk4_t,
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
    asdf_gwcs_fk4_t,
    &libasdf_gwcs_software,
    &fk4_vtab,
    NULL,
    ASDF_COORDINATES_TAG_PREFIX "fk4noeterms-1.2.0",
    ASDF_COORDINATES_TAG_PREFIX "fk4noeterms-1.1.0",
    ASDF_COORDINATES_TAG_PREFIX "fk4noeterms-1.0.0"
)
