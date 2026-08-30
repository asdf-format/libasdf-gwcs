#include <stdlib.h>

#include <asdf/core/time.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../util.h"
#include "frame_attributes.h"


/* Read one time-valued frame attribute.  The property is fetched as a raw
 * value and converted with asdf_value_as_time rather than asked for by tag:
 * the schema allows any time-1.* version, and the generated accessor matches
 * every version the time extension registers.
 *
 * Returns ASDF_VALUE_OK with *out left NULL when the property is absent. */
static asdf_value_err_t get_time_attribute(
    asdf_mapping_t *attrs, const char *name, asdf_time_t **out) {
    asdf_value_t *value = asdf_mapping_get(attrs, name);

    if (!value)
        return ASDF_VALUE_OK;

    asdf_value_err_t err = ASDF_VALUE_OK;

    if (!asdf_value_is_null(value))
        err = asdf_value_as_time(value, out);

    asdf_value_destroy(value);
    return err;
}


asdf_value_err_t asdf_gwcs_frame_attributes_parse(
    asdf_value_t *value, asdf_time_t **equinox, asdf_time_t **obstime) {
    asdf_mapping_t *map = NULL;
    asdf_mapping_t *attrs = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    err = asdf_get_required_property(
        map, "frame_attributes", ASDF_VALUE_MAPPING, NULL, (void *)&attrs);

    if (ASDF_IS_ERR(err))
        return err;

    err = get_time_attribute(attrs, "equinox", equinox);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    if (!*equinox) {
        ASDF_LOG(
            asdf_value_file(value),
            ASDF_LOG_WARN,
            "coordinate frame at %s is missing the required frame_attributes "
            "property equinox",
            asdf_value_path(value));
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    if (obstime)
        err = get_time_attribute(attrs, "obstime", obstime);

cleanup:
    asdf_mapping_destroy(attrs);
    return err;
}


asdf_value_t *asdf_gwcs_frame_attributes_serialize(
    asdf_file_t *file, const asdf_time_t *equinox, const asdf_time_t *obstime) {
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_mapping_t *attrs = asdf_mapping_create(file);

    if (!attrs)
        goto failure;

    const char *names[2] = {"equinox", "obstime"};
    const asdf_time_t *times[2] = {equinox, obstime};

    for (int idx = 0; idx < 2; idx++) {
        if (!times[idx])
            continue;

        asdf_value_t *time_val = asdf_value_of_time(file, times[idx]);

        if (!time_val)
            goto failure_attrs;

        if (ASDF_IS_ERR(asdf_mapping_set(attrs, names[idx], time_val))) {
            asdf_value_destroy(time_val);
            goto failure_attrs;
        }
    }

    if (ASDF_IS_ERR(asdf_mapping_set_mapping(map, "frame_attributes", attrs)))
        goto failure_attrs;

    return asdf_value_of_mapping(map);

failure_attrs:
    asdf_mapping_destroy(attrs);
failure:
    asdf_mapping_destroy(map);
    return NULL;
}
