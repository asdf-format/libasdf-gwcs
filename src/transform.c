#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <asdf/extension_util.h>
#include <asdf/extension.h>
#include <asdf/log.h>
#include <asdf/value.h>

#include "gwcs.h"
#include "types/asdf_gwcs_transform_map.h"
#include "util.h"


static asdf_gwcs_transform_map_t global_transform_map = {0};
static atomic_bool global_transform_map_initialized = false;


static asdf_gwcs_transform_type_t asdf_gwcs_transform_type_get(const char *tagstr) {
    const asdf_gwcs_transform_map_value *item = asdf_gwcs_transform_map_get(
        &global_transform_map, tagstr);

    if (!item)
        return ASDF_GWCS_TRANSFORM_INVALID;

    return item->second;
}


asdf_value_err_t asdf_gwcs_transform_parse(asdf_value_t *value, asdf_gwcs_transform_t *transform) {

    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *transform_map = NULL;
    asdf_tag_t *parsed_tag = NULL;

    if (asdf_value_as_mapping(value, &transform_map) != ASDF_VALUE_OK)
        goto failure;

    const char *tag = asdf_value_tag(value);

    if (!tag) {
        // Not a transform as far as we know
        err = ASDF_VALUE_ERR_TYPE_MISMATCH;
        goto failure;
    }

    parsed_tag = asdf_tag_parse(tag);

    if (!parsed_tag) {
        err = ASDF_VALUE_ERR_TYPE_MISMATCH;
        goto failure;
    }

    asdf_gwcs_transform_type_t type = asdf_gwcs_transform_type_get(parsed_tag->name);

    /* Unknown tags are treated as generic rather than an error */
    if (ASDF_GWCS_TRANSFORM_INVALID == type)
        type = ASDF_GWCS_TRANSFORM_GENERIC;

    transform->type = type;
    transform->ext = asdf_extension_get(asdf_value_file(value), tag);

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

    for (int k = 0; k < 2; k++) {
        asdf_sequence_t *seq = NULL;
        err = asdf_get_optional_property(
            transform_map, io_keys[k], ASDF_VALUE_SEQUENCE, NULL, (void *)&seq);

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

            *io_counts[k] = (uint32_t)n;
            *io_arrays[k] = (const char **)arr;

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
    asdf_tag_destroy(parsed_tag);
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

    if (transform->ext && transform->ext->dealloc) {
        transform->ext->dealloc(transform);
        return;
    }

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
 * Reverse lookup: transform type enum → tag string (without version).
 *
 * The entries intentionally mirror the forward map in
 * `asdf_gwcs_transform_map_create` so that a round-trip through serialize /
 * deserialize succeeds.
 */
static const char *const transform_type_to_tag_map[ASDF_GWCS_TRANSFORM_LAST] = {
    [ASDF_GWCS_TRANSFORM_GENERIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform",
    [ASDF_GWCS_TRANSFORM_AIRY] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy",
    [ASDF_GWCS_TRANSFORM_BONNE_EQUAL_AREA] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area",
    [ASDF_GWCS_TRANSFORM_COBE_QUAD_SPHERICAL_CUBE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "cobe_quad_spherical_cube",
    [ASDF_GWCS_TRANSFORM_CONIC_EQUAL_AREA] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area",
    [ASDF_GWCS_TRANSFORM_CONIC_EQUIDISTANT] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant",
    [ASDF_GWCS_TRANSFORM_CONIC_ORTHOMORPHIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic",
    [ASDF_GWCS_TRANSFORM_CONIC_PERSPECTIVE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective",
    [ASDF_GWCS_TRANSFORM_CYLINDRICAL_EQUAL_AREA] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "cylindrical_equal_area",
    [ASDF_GWCS_TRANSFORM_CYLINDRICAL_PERSPECTIVE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "cylindrical_perspective",
    [ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING] = ASDF_GWCS_TAG_PREFIX "fitswcs_imaging",
    [ASDF_GWCS_TRANSFORM_GNOMONIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic",
    [ASDF_GWCS_TRANSFORM_HAMMER_AITOFF] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff",
    [ASDF_GWCS_TRANSFORM_HEALPIX_POLAR] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar",
    [ASDF_GWCS_TRANSFORM_MOLLEWEIDE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide",
    [ASDF_GWCS_TRANSFORM_PARABOLIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic",
    [ASDF_GWCS_TRANSFORM_PLATE_CARREE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree",
    [ASDF_GWCS_TRANSFORM_POLYCONIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic",
    [ASDF_GWCS_TRANSFORM_SANSON_FLAMSTEED] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed",
    [ASDF_GWCS_TRANSFORM_SLANT_ORTHOGRAPHIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic",
    [ASDF_GWCS_TRANSFORM_STEREOGRAPHIC] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic",
    [ASDF_GWCS_TRANSFORM_QUAD_SPHERICAL_CUBE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "quad_spherical_cube",
    [ASDF_GWCS_TRANSFORM_SLANT_ZENITHAL_PERSPECTIVE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "slant_zenithal_perspective",
    [ASDF_GWCS_TRANSFORM_TANGENTIAL_SPHERICAL_CUBE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "tangential_spherical_cube",
    [ASDF_GWCS_TRANSFORM_ZENITHAL_EQUAL_AREA] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "zenithal_equal_area",
    [ASDF_GWCS_TRANSFORM_ZENITHAL_EQUIDISTANT] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "zenithal_equidistant",
    [ASDF_GWCS_TRANSFORM_ZENITHAL_PERSPECTIVE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "zenithal_perspective",
    [ASDF_GWCS_TRANSFORM_SHIFT] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift",
    [ASDF_GWCS_TRANSFORM_SCALE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale",
    [ASDF_GWCS_TRANSFORM_REMAP_AXES] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes",
    [ASDF_GWCS_TRANSFORM_POLYNOMIAL] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "polynomial",
    [ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "rotate_sequence_3d",
    [ASDF_GWCS_TRANSFORM_COMPOSE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "compose",
    [ASDF_GWCS_TRANSFORM_CONCATENATE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate",
};


const char *asdf_gwcs_transform_type_to_tag(asdf_gwcs_transform_type_t type) {
    if (type < 0 || (unsigned int)type >= ASDF_GWCS_TRANSFORM_LAST)
        return NULL;

    return transform_type_to_tag_map[type];
}


/** Versioned tags for concrete (registered) transforms, for extension lookup
 *
 * This is a temporary solution for the lack of wildcard extension type lookup
 * in libasdf; for now we just hard-code the supported versions, but longer-
 * term this should be more easily maintainable...
 */
static const char *const transform_type_to_versioned_tag_map[ASDF_GWCS_TRANSFORM_LAST] = {
    [ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING] = ASDF_GWCS_TAG_PREFIX "fitswcs_imaging-1.0.0",
    [ASDF_GWCS_TRANSFORM_SHIFT] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift-1.3.0",
    [ASDF_GWCS_TRANSFORM_REMAP_AXES] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes-1.4.0",
    [ASDF_GWCS_TRANSFORM_POLYNOMIAL] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "polynomial-1.2.0",
    [ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D] = ASDF_GWCS_TRANSFORM_TAG_PREFIX
    "rotate_sequence_3d-1.1.0",
    [ASDF_GWCS_TRANSFORM_COMPOSE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "compose-1.3.0",
    [ASDF_GWCS_TRANSFORM_CONCATENATE] = ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate-1.3.0",
};


asdf_value_err_t asdf_gwcs_transform_serialize_base(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform, asdf_mapping_t *map) {
    asdf_value_err_t err = ASDF_VALUE_OK;

    if (transform->name) {
        err = asdf_mapping_set_string0(map, "name", transform->name);

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


static asdf_value_t *asdf_gwcs_generic_transform_serialize(
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

    if (transform->ext)
        return asdf_value_of_extension_type(file, transform, transform->ext);

    /* For programmatically-constructed transforms (ext == NULL), look up
     * the extension by versioned tag so the concrete serializer is used. */
    if ((unsigned int)transform->type < ASDF_GWCS_TRANSFORM_LAST) {
        const char *vtag = transform_type_to_versioned_tag_map[transform->type];

        if (vtag) {
            const asdf_extension_t *ext = asdf_extension_get(file, vtag);

            if (ext)
                return asdf_value_of_extension_type(file, transform, ext);
        }
    }

    const char *tag = asdf_gwcs_transform_type_to_tag(transform->type);

    if (!tag)
        return NULL;

    asdf_extension_t tmp_ext = {
        .tag = tag,
        .software = &libasdf_software,
        .serialize = asdf_gwcs_generic_transform_serialize,
    };

    return asdf_value_of_extension_type(file, transform, &tmp_ext);
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
    if (atomic_load_explicit(&global_transform_map_initialized, memory_order_acquire))
        return;

    global_transform_map = c_make(
        asdf_gwcs_transform_map,
        {{ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform", ASDF_GWCS_TRANSFORM_GENERIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy", ASDF_GWCS_TRANSFORM_AIRY},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area", ASDF_GWCS_TRANSFORM_BONNE_EQUAL_AREA},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube",
          ASDF_GWCS_TRANSFORM_COBE_QUAD_SPHERICAL_CUBE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area", ASDF_GWCS_TRANSFORM_CONIC_EQUAL_AREA},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant",
          ASDF_GWCS_TRANSFORM_CONIC_EQUIDISTANT},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic",
          ASDF_GWCS_TRANSFORM_CONIC_ORTHOMORPHIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective",
          ASDF_GWCS_TRANSFORM_CONIC_PERSPECTIVE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area",
          ASDF_GWCS_TRANSFORM_CYLINDRICAL_EQUAL_AREA},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective",
          ASDF_GWCS_TRANSFORM_CYLINDRICAL_PERSPECTIVE},
         {ASDF_GWCS_TAG_PREFIX "fitswcs_imaging", ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic", ASDF_GWCS_TRANSFORM_GNOMONIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff", ASDF_GWCS_TRANSFORM_HAMMER_AITOFF},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar", ASDF_GWCS_TRANSFORM_HEALPIX_POLAR},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide", ASDF_GWCS_TRANSFORM_MOLLEWEIDE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic", ASDF_GWCS_TRANSFORM_PARABOLIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree", ASDF_GWCS_TRANSFORM_PLATE_CARREE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic", ASDF_GWCS_TRANSFORM_POLYCONIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed", ASDF_GWCS_TRANSFORM_SANSON_FLAMSTEED},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic",
          ASDF_GWCS_TRANSFORM_SLANT_ORTHOGRAPHIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic", ASDF_GWCS_TRANSFORM_STEREOGRAPHIC},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "quad_spherical_cube",
          ASDF_GWCS_TRANSFORM_QUAD_SPHERICAL_CUBE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_zenithal_perspective",
          ASDF_GWCS_TRANSFORM_SLANT_ZENITHAL_PERSPECTIVE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "tangential_spherical_cube",
          ASDF_GWCS_TRANSFORM_TANGENTIAL_SPHERICAL_CUBE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equal_area",
          ASDF_GWCS_TRANSFORM_ZENITHAL_EQUAL_AREA},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equidistant",
          ASDF_GWCS_TRANSFORM_ZENITHAL_EQUIDISTANT},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective",
          ASDF_GWCS_TRANSFORM_ZENITHAL_PERSPECTIVE},
         /* Atomic transforms */
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "shift", ASDF_GWCS_TRANSFORM_SHIFT},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "scale", ASDF_GWCS_TRANSFORM_SCALE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "remap_axes", ASDF_GWCS_TRANSFORM_REMAP_AXES},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "polynomial", ASDF_GWCS_TRANSFORM_POLYNOMIAL},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate_sequence_3d",
          ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "compose", ASDF_GWCS_TRANSFORM_COMPOSE},
         {ASDF_GWCS_TRANSFORM_TAG_PREFIX "concatenate", ASDF_GWCS_TRANSFORM_CONCATENATE}});

    atomic_store_explicit(&global_transform_map_initialized, true, memory_order_release);
}


ASDF_DESTRUCTOR static void asdf_gwcs_transform_map_destroy(void) {
    if (atomic_load_explicit(&global_transform_map_initialized, memory_order_acquire)) {
        asdf_gwcs_transform_map_drop(&global_transform_map);
        atomic_store_explicit(&global_transform_map_initialized, false, memory_order_release);
    }
}
