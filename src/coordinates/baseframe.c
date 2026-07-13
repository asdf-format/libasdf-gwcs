#include <stdatomic.h>
#include <stdlib.h>

#include <asdf/extension.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "../gwcs.h"
#include "../types/asdf_gwcs_coordinate_frame_map.h"
#include "../util.h"


static asdf_gwcs_coordinate_frame_map_t g_coordinate_frame_map = {0};
static atomic_bool g_coordinate_frame_map_initialized = false;


static void coordinate_frame_map_ensure_init(void) {
    if (!atomic_load_explicit(&g_coordinate_frame_map_initialized, memory_order_acquire)) {
        g_coordinate_frame_map = asdf_gwcs_coordinate_frame_map_init();
        atomic_store_explicit(&g_coordinate_frame_map_initialized, true, memory_order_release);
    }
}


ASDF_DESTRUCTOR static void coordinate_frame_map_destroy(void) {
    if (atomic_load_explicit(&g_coordinate_frame_map_initialized, memory_order_acquire)) {
        asdf_gwcs_coordinate_frame_map_drop(&g_coordinate_frame_map);
        atomic_store_explicit(&g_coordinate_frame_map_initialized, false, memory_order_release);
    }
}


void asdf_gwcs_coordinate_frame_register(asdf_gwcs_coordinate_frame_type_t type) {
    coordinate_frame_map_ensure_init();
    const char *const *tags = ((asdf_extension_t *)type)->tags;

    for (const char *const *tag = tags; *tag; tag++) {
        char *full_tag = tag_canonicalize(*tag);
        if (!full_tag) {
            ASDF_LOG(NULL, ASDF_LOG_FATAL,
                     "failed to allocate memory for coordinate frame tag %s", *tag);
            return;
        }
        asdf_gwcs_coordinate_frame_map_result res =
            asdf_gwcs_coordinate_frame_map_emplace(&g_coordinate_frame_map, full_tag, type);
        (void)res;
        free(full_tag);
    }
}


asdf_value_err_t asdf_value_as_gwcs_coordinate_frame(
    asdf_value_t *value, asdf_gwcs_baseframe_t **out) {

    const char *tag_str = asdf_value_tag(value);
    if (UNLIKELY(!tag_str))
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    char *full_tag = tag_canonicalize(tag_str);
    if (!full_tag)
        return ASDF_VALUE_ERR_OOM;

    const asdf_gwcs_coordinate_frame_map_value *item =
        asdf_gwcs_coordinate_frame_map_get(&g_coordinate_frame_map, full_tag);
    free(full_tag);

    if (!item)
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    const asdf_extension_t *ext = (const asdf_extension_t *)item->second;
    asdf_value_err_t err = asdf_value_as_extension_type(value, ext, (void **)out);

    /* The deserializer uses calloc so type is zero; stamp it from the registry. */
    if (err == ASDF_VALUE_OK && *out)
        (*out)->type = item->second;

    return err;
}


asdf_value_t *asdf_gwcs_coordinate_frame_value_of(
    asdf_file_t *file, const asdf_gwcs_baseframe_t *frame) {

    if (!frame)
        return NULL;

    const asdf_extension_t *ext = (const asdf_extension_t *)frame->type;
    return asdf_value_of_extension_type(file, frame, ext);
}


void asdf_gwcs_coordinate_frame_destroy(asdf_gwcs_baseframe_t *frame) {
    if (!frame)
        return;

    const asdf_extension_t *ext = (const asdf_extension_t *)frame->type;
    if (ext && ext->vtab && ext->vtab->dealloc)
        ext->vtab->dealloc(frame);
    else
        free(frame);
}


/* Generic serialize/deserialize for frames with empty frame_attributes.
 *
 * Applies to: ICRS, Galactic, Supergalactic, BarycentricMeanEcliptic */

static asdf_value_t *empty_frame_serialize(
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


static asdf_value_err_t empty_frame_deserialize(
    UNUSED(asdf_value_t *value), UNUSED(const void *userdata), void **out) {

    *out = calloc(1, sizeof(asdf_gwcs_baseframe_t));
    return *out ? ASDF_VALUE_OK : ASDF_VALUE_ERR_OOM;
}


static void baseframe_dealloc(void *value) {
    free(value);
}


static const asdf_extension_vtab_t empty_frame_vtab = {
    .serialize = empty_frame_serialize,
    .deserialize = empty_frame_deserialize,
    .copy = NULL, /* TODO */
    .dealloc = baseframe_dealloc,
};


/**
 * Register icrs frame extensions
 *
 * NOTE: The only differences between versions 1.1.0 and 1.3.0 are the
 * baseframe schema versions.  icrs-1.0.0, however, is a substantively
 * different schema (appears to predate the baseframe schema) and is not
 * yet implemented.
 */
ASDF_GWCS_REGISTER_COORDINATE_FRAME(
    icrs,
    ICRS,
    asdf_gwcs_baseframe_t,
    &libasdf_gwcs_software,
    &empty_frame_vtab,
    NULL,
    ASDF_COORDINATES_TAG_PREFIX "icrs-1.3.0",
    ASDF_COORDINATES_TAG_PREFIX "icrs-1.2.0",
    ASDF_COORDINATES_TAG_PREFIX "icrs-1.1.0"
)

/**
 * Register galactic frame extensions
 *
 * NOTE: The only differences so far between galactic schema versions are
 * in the baseframe schema versions.
 */
ASDF_GWCS_REGISTER_COORDINATE_FRAME(
    galactic,
    GALACTIC,
    asdf_gwcs_baseframe_t,
    &libasdf_gwcs_software,
    &empty_frame_vtab,
    NULL,
    ASDF_COORDINATES_TAG_PREFIX "galactic-1.2.0",
    ASDF_COORDINATES_TAG_PREFIX "galactic-1.1.0",
    ASDF_COORDINATES_TAG_PREFIX "galactic-1.0.0"
)
