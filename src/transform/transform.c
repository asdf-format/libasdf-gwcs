/** Generic handling for all transforms, base transform handling, and registration */

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/error.h>
#include <asdf/extension.h>
#include <asdf/extension_util.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "gwcs.h"
#include "types/asdf_gwcs_transform_map.h"
#include "util.h"


static const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_INVALID = NULL;
static asdf_gwcs_transform_map_t g_transform_map = {0};
static atomic_bool g_transform_map_initialized = false;


static asdf_gwcs_transform_type_t asdf_gwcs_transform_type_get(const char *tagstr) {
    const asdf_gwcs_transform_map_value *item = asdf_gwcs_transform_map_get(
        &g_transform_map, tagstr);

    if (!item)
        return ASDF_GWCS_TRANSFORM_INVALID;

    return item->second;
}


asdf_value_err_t asdf_gwcs_transform_parse(asdf_value_t *value, asdf_gwcs_transform_t *transform) {

    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *transform_map = NULL;

    if (asdf_value_as_mapping(value, &transform_map) != ASDF_VALUE_OK)
        goto failure;

    const char *tag = asdf_value_tag(value);

    if (!tag) {
        // Not a transform as far as we know
        err = ASDF_VALUE_ERR_TYPE_MISMATCH;
        goto failure;
    }

    asdf_gwcs_transform_type_t type = asdf_gwcs_transform_type_get(tag);

    /** TODO: Allow unknown tags to be handled? */
    if (ASDF_GWCS_TRANSFORM_INVALID == type) {
        err = ASDF_VALUE_ERR_TYPE_MISMATCH;
        goto failure;
    }

    transform->type = type;

    err = asdf_get_optional_property(
        transform_map, "name", ASDF_VALUE_STRING, NULL, (void *)&transform->name);

    if (!ASDF_IS_OPTIONAL_OK(err))
        goto failure;

    err = asdf_get_optional_property(
        transform_map,
        "bounding_box",
        ASDF_VALUE_EXTENSION,
        ASDF_GWCS_BOUNDING_BOX_TAG,
        (void *)&transform->bounding_box);

    if (!ASDF_IS_OPTIONAL_OK(err))
        goto failure;

    /* Parse optional inputs/outputs name sequences */
    const char *io_keys[2] = {"inputs", "outputs"};
    uint32_t *io_counts[2] = {&transform->n_inputs, &transform->n_outputs};
    const char ***io_arrays[2] = {&transform->inputs, &transform->outputs};

    for (int kdx = 0; kdx < 2; kdx++) {
        asdf_sequence_t *seq = NULL;
        err = asdf_get_optional_property(
            transform_map, io_keys[kdx], ASDF_VALUE_SEQUENCE, NULL, (void *)&seq);

        if (!ASDF_IS_OPTIONAL_OK(err))
            goto failure;

        if (!seq)
            continue;

        int n = asdf_sequence_size(seq);

        if (n > 0) {
            char **arr = calloc((size_t)n, sizeof(char *));

            if (!arr) {
                asdf_sequence_destroy(seq);
                err = ASDF_VALUE_ERR_OOM;
                goto failure;
            }

            *io_counts[kdx] = (uint32_t)n;
            *io_arrays[kdx] = (const char **)arr;

            asdf_sequence_iter_t *iter = asdf_sequence_iter_init(seq);

            while (asdf_sequence_iter_next(&iter)) {
                const char *s = NULL;
                err = asdf_value_as_string0(iter->value, &s);

                if (!ASDF_IS_OK(err)) {
                    asdf_sequence_iter_destroy(iter);
                    asdf_sequence_destroy(seq);
                    goto failure;
                }

                arr[iter->index] = strdup(s);

                if (!arr[iter->index]) {
                    err = ASDF_VALUE_ERR_OOM;
                    asdf_sequence_iter_destroy(iter);
                    asdf_sequence_destroy(seq);
                    goto failure;
                }
            }
        }

        asdf_sequence_destroy(seq);
    }

    err = ASDF_VALUE_OK;

failure:
    return err;
}



asdf_value_err_t asdf_gwcs_transform_generic_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_transform_t *transform = calloc(1, sizeof(asdf_gwcs_transform_t));

    if (!transform)
        return ASDF_VALUE_ERR_OOM;

    asdf_value_err_t err = asdf_gwcs_transform_parse(value, transform);

    if (ASDF_IS_OK(err))
        *out = transform;

    return err;
}


void asdf_gwcs_transform_clean(asdf_gwcs_transform_t *transform) {
    if (!transform)
        return;

    if (transform->inputs) {
        for (uint32_t idx = 0; idx < transform->n_inputs; idx++)
            free((char *)transform->inputs[idx]);
        free((void *)transform->inputs);
    }

    if (transform->outputs) {
        for (uint32_t idx = 0; idx < transform->n_outputs; idx++)
            free((char *)transform->outputs[idx]);
        free((void *)transform->outputs);
    }

    asdf_gwcs_bounding_box_destroy((asdf_gwcs_bounding_box_t *)transform->bounding_box);
    ZERO_MEMORY(transform, sizeof(asdf_gwcs_transform_t));
}


void asdf_gwcs_transform_arity_set(
    asdf_gwcs_transform_t *transform,
    UNUSED(const asdf_file_t *file),
    uint32_t implicit_n_inputs,
    uint32_t implicit_n_outputs) {
    if (implicit_n_inputs > 0) {
        if (transform->n_inputs == 0) {
            transform->n_inputs = implicit_n_inputs;
        } else if (transform->n_inputs != implicit_n_inputs) {
            ASDF_LOG(
                file,
                ASDF_LOG_WARN,
                "transform n_inputs %u does not match implicit count %u",
                transform->n_inputs,
                implicit_n_inputs);
        }
    }

    if (implicit_n_outputs > 0) {
        if (transform->n_outputs == 0) {
            transform->n_outputs = implicit_n_outputs;
        } else if (transform->n_outputs != implicit_n_outputs) {
            ASDF_LOG(
                file,
                ASDF_LOG_WARN,
                "transform n_outputs %u does not match implicit count %u",
                transform->n_outputs,
                implicit_n_outputs);
        }
    }
}


void asdf_gwcs_transform_destroy(asdf_gwcs_transform_t *transform) {
    if (!transform)
        return;

    const asdf_extension_t *ext = (const asdf_extension_t *)transform->type;

    if (ext && ext->dealloc) {
        ext->dealloc(transform);
        return;
    }

    asdf_gwcs_transform_clean(transform);
    free(transform);
}


void asdf_gwcs_transform_generic_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_transform_t *transform = (asdf_gwcs_transform_t *)value;
    asdf_gwcs_transform_clean(transform);
    free(transform);
}


asdf_value_err_t asdf_value_as_gwcs_transform(asdf_value_t *value, asdf_gwcs_transform_t **out) {
    const char *tag_str = asdf_value_tag(value);

    if (UNLIKELY(!tag_str))
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    const asdf_extension_t *ext = asdf_extension_get(asdf_value_file(value), tag_str);

    if (ext)
        return asdf_value_as_extension_type(value, ext, (void **)out);

    // Generic / unknown transform: parse only the base fields
    asdf_gwcs_transform_t *transform = calloc(1, sizeof(asdf_gwcs_transform_t));

    if (!transform)
        return ASDF_VALUE_ERR_OOM;

    asdf_value_err_t err = asdf_gwcs_transform_parse(value, transform);

    if (ASDF_IS_OK(err))
        *out = transform;
    else
        asdf_gwcs_transform_destroy(transform);

    return err;
}


/**
 * Prefix a tag string with tag: if not already prefixed
 *
 * Memory is always allocated for the new string even if unmodified
 *
 * TODO: This is copied out of libasdf; should maybe be added to its API
 */
static char *tag_canonicalize(const char *tag) {
    char *full_tag = NULL;
    if (0 != strncmp(tag, "tag:", 4)) {
        size_t taglen = strlen(tag);
        full_tag = malloc(4 + taglen + 1);

        if (!full_tag)
            return NULL;

        memcpy(full_tag, "tag:", 4);
        memcpy(full_tag + 4, tag, taglen + 1);
    } else {
        full_tag = strdup(tag);
    }

    return full_tag;
}


const char *asdf_gwcs_transform_type_to_tag(asdf_gwcs_transform_type_t type) {
    const char *full_tag = NULL;
    const asdf_extension_t *ext = (const asdf_extension_t *)type;

    if (!ext)
        return NULL;

    full_tag = tag_canonicalize(ext->tag);

    if (UNLIKELY(!full_tag)) {
        // TODO: libasdf's public headers don't expose its internal error-reporting primitives
        // but this would be useful to have for extension authors
        // ASDF_ERROR_OOM(NULL);
        return NULL;
    }

    return full_tag;
}


static asdf_value_err_t serialize_string_sequence(
    asdf_file_t *file, asdf_mapping_t *map, const char *key, const char **strings, uint32_t n) {
    asdf_sequence_t *seq = asdf_sequence_create(file);

    if (!seq)
        return ASDF_VALUE_ERR_EMIT_FAILURE;

    asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);

    for (uint32_t idx = 0; idx < n; idx++) {
        asdf_value_err_t err = asdf_sequence_append_string0(seq, strings[idx]);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_destroy(seq);
            return err;
        }
    }

    asdf_value_err_t err = asdf_mapping_set_sequence(map, key, seq);

    if (ASDF_IS_ERR(err))
        asdf_sequence_destroy(seq);

    return err;
}


asdf_value_err_t asdf_gwcs_transform_serialize_base(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform, asdf_mapping_t *map) {
    asdf_value_err_t err = ASDF_VALUE_OK;

    if (transform->name) {
        err = asdf_mapping_set_string0(map, "name", transform->name);

        if (ASDF_IS_ERR(err))
            return err;
    }

    if (transform->inputs && transform->n_inputs > 0) {
        err = serialize_string_sequence(
            file, map, "inputs", transform->inputs, transform->n_inputs);

        if (ASDF_IS_ERR(err))
            return err;
    }

    if (transform->outputs && transform->n_outputs > 0) {
        err = serialize_string_sequence(
            file, map, "outputs", transform->outputs, transform->n_outputs);

        if (ASDF_IS_ERR(err))
            return err;
    }

    if (transform->bounding_box) {
        asdf_value_t *bb_val = asdf_value_of_gwcs_bounding_box(file, transform->bounding_box);

        if (!bb_val)
            return ASDF_VALUE_ERR_EMIT_FAILURE;

        err = asdf_mapping_set(map, "bounding_box", bb_val);

        if (ASDF_IS_ERR(err)) {
            asdf_value_destroy(bb_val);
            return err;
        }
    }

    return ASDF_VALUE_OK;
}


static asdf_value_t *asdf_gwcs_transform_generic_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_transform_t *transform = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, transform, map);

    if (ASDF_IS_ERR(err)) {
        asdf_mapping_destroy(map);
        return NULL;
    }

    return asdf_value_of_mapping(map);
}


asdf_value_t *asdf_value_of_gwcs_transform(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform) {
    if (!transform)
        return NULL;

    const asdf_extension_t *ext = (asdf_extension_t *)transform->type;
    return asdf_value_of_extension_type(file, transform, ext);
}


/**
 * Hard-coded mapping between known transform tags and their associated
 * `asdf_gwcs_transform_type_t` value
 *
 * .. todo::
 *
 *   Later we will want to have routines for extending this map
 *   programmatically, e.g. by extensions for the GWCS plugin.  Also will still need to
 *   better support different schema versions.
 */
ASDF_CONSTRUCTOR static void asdf_gwcs_transform_map_create() {
    if (atomic_load_explicit(&g_transform_map_initialized, memory_order_acquire))
        return;

    g_transform_map = asdf_gwcs_transform_map_init();
    atomic_store_explicit(&g_transform_map_initialized, true, memory_order_release);
}


ASDF_DESTRUCTOR static void asdf_gwcs_transform_map_destroy(void) {
    if (atomic_load_explicit(&g_transform_map_initialized, memory_order_acquire)) {
        asdf_gwcs_transform_map_drop(&g_transform_map);
        atomic_store_explicit(&g_transform_map_initialized, false, memory_order_release);
    }
}


/* This is much like the main ASDF extension registry but also registers
 * specific extensions as known transforms (basically members of a transform
 * "class" hierarchy).
 *
 * May be useful to extend the asdf extension registry itself to include this
 * notion of registration groups to avoid duplicate work...
 *
 * Transforms are registered uniquely based on their tag.
 */
void asdf_gwcs_transform_register(asdf_gwcs_transform_type_t type) {
    /* TODO: Handle tag overlaps on registration */
    // Ensure extension map initialized
    asdf_gwcs_transform_map_create();
    const char *tag = ((asdf_extension_t *)type)->tag;

    char *full_tag = tag_canonicalize(tag);
    if (!full_tag) {
        ASDF_LOG(NULL, ASDF_LOG_FATAL, "failed to allocate memory for transform tag %s", tag);
        return;
    }
    asdf_gwcs_transform_map_result res = asdf_gwcs_transform_map_emplace(
        &g_transform_map, full_tag, type);

#ifdef ASDF_LOG_ENABLED
    if (res.inserted)
        ASDF_LOG(NULL, ASDF_LOG_DEBUG, "registered transform for tag %s", full_tag);
    else
        ASDF_LOG(NULL, ASDF_LOG_WARN, "failed to register transform for tag %s", full_tag);
#else
    (void)res;
#endif

    free(full_tag);
}


const asdf_extension_t *asdf_gwcs_transform_get(UNUSED(asdf_file_t *file), const char *tag) {
    const asdf_gwcs_transform_map_value *ext = NULL;
    char *full_tag = tag_canonicalize(tag);

    if (!full_tag) {
        // TODO: Needed in public API for extension authors...
        // ASDF_ERROR_OOM(file);
        return NULL;
    }

    ext = asdf_gwcs_transform_map_get(&g_transform_map, full_tag);

    if (!ext) {
        ASDF_LOG(file, ASDF_LOG_TRACE, "no transform registered for tag %s", full_tag);
        free(full_tag);
        return NULL;
    }

    free(full_tag);
    return (const asdf_extension_t *)ext->second;
}


/** Register a "generic" transform
 *
 * These don't have any special implementation details at the moment, and
 * are registered mainly so that their tags are recognized as known
 * transforms.
 *
 * .. todo::
 *
 *   This would be a very good use case for updating `ASDF_REGISTER_EXTENSION`
 *   to allow registering multiple tags to an extension (besides better
 *   multi-version support which also needs to be done).
 */
#define ASDF_GWCS_REGISTER_TRANSFORM_GENERIC(extname, ttype, tag) \
    ASDF_GWCS_REGISTER_TRANSFORM( \
        extname, ttype, tag, asdf_gwcs_transform_t, \
        &libasdf_gwcs_software, \
        asdf_gwcs_transform_generic_serialize, \
        asdf_gwcs_transform_generic_deserialize, \
        NULL, \
        asdf_gwcs_transform_generic_dealloc, \
        NULL);


#define ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(extname, ttype, tag, ctype) \
    ASDF_GWCS_REGISTER_TRANSFORM_WITH_CTYPE( \
        extname, ttype, tag, asdf_gwcs_transform_t, \
        &libasdf_gwcs_software, \
        asdf_gwcs_transform_generic_serialize, \
        asdf_gwcs_transform_generic_deserialize, \
        NULL, \
        asdf_gwcs_transform_generic_dealloc, \
        ctype);


/**
 * Register extension for the base transform type
 *
 * Transform subtypes are registered through ASDF_GWCS_REGISTER_TRANSFORM
 */
ASDF_GWCS_REGISTER_TRANSFORM_GENERIC(
    transform_generic,
    GENERIC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform-1.2.0");


/**
 * Register additional known transforms as generic transforms
 */
ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    airy,
    AIRY,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy-1.3.0",
    AIR);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    bonne_equal_area,
    BONNE_EQUAL_AREA,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.3.0",
    BON);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    cobe_quad_spherical_cube,
    COBE_QUAD_SPHERICAL_CUBE,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube-1.3.0",
    CSC);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_equal_area,
    CONIC_EQUAL_AREA,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.3.0",
    COE);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_equidistant,
    CONIC_EQUIDISTANT,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.3.0",
    COD);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_orthomorphic,
    CONIC_ORTHOMORPHIC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.3.0",
    COO);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_perspective,
    CONIC_PERSPECTIVE,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.3.0",
    COP);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    cylindrical_equal_area,
    CYLINDRICAL_EQUAL_AREA,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.3.0",
    CEA);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    cylindrical_perspective,
    CYLINDRICAL_PERSPECTIVE,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.3.0",
    CYP);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    gnomonic,
    GNOMONIC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic-1.3.0",
    TAN);

/** 
 * TODO: Register the remaining FITS WCS projection transform types:
 * 
 * [ASDF_GWCS_TRANSFORM_HAMMER_AITOFF] = "AIT",
 * [ASDF_GWCS_TRANSFORM_HEALPIX_POLAR] = "XPH",
 * [ASDF_GWCS_TRANSFORM_MOLLEWEIDE] = "MOL",
 * [ASDF_GWCS_TRANSFORM_PARABOLIC] = "PAR",
 * [ASDF_GWCS_TRANSFORM_PLATE_CARREE] = "CAR",
 * [ASDF_GWCS_TRANSFORM_POLYCONIC] = "PCO",
 * [ASDF_GWCS_TRANSFORM_SANSON_FLAMSTEED] = "SFL",
 * [ASDF_GWCS_TRANSFORM_SLANT_ORTHOGRAPHIC] = "SIN",
 * [ASDF_GWCS_TRANSFORM_STEREOGRAPHIC] = "STG",
 * [ASDF_GWCS_TRANSFORM_QUAD_SPHERICAL_CUBE] = "QSC",
 * [ASDF_GWCS_TRANSFORM_SLANT_ZENITHAL_PERSPECTIVE] = "SZP",
 * [ASDF_GWCS_TRANSFORM_TANGENTIAL_SPHERICAL_CUBE] = "TSC",
 * [ASDF_GWCS_TRANSFORM_ZENITHAL_EQUAL_AREA] = "ZEA",
 * [ASDF_GWCS_TRANSFORM_ZENITHAL_EQUIDISTANT] = "ARC",
 * [ASDF_GWCS_TRANSFORM_ZENITHAL_PERSPECTIVE] = "AZP",
 */
