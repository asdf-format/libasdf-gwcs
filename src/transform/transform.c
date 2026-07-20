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
#include "transform.h"
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


static void asdf_gwcs_transform_deinit_base(asdf_gwcs_transform_t *transform) {
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

    free((char *)transform->name);
    asdf_gwcs_bounding_box_destroy((asdf_gwcs_bounding_box_t *)transform->bounding_box);
    asdf_gwcs_transform_destroy((asdf_gwcs_transform_t *)transform->inverse);
    ZERO_MEMORY(transform, sizeof(asdf_gwcs_transform_t));
}


/* Copy the base transform fields into dst.  Invoked (together with the
 * subclass's own copy method) by the copy shim that ASDF_GWCS_REGISTER_TRANSFORM
 * installs as each transform extension's vtab copy method, so that both the
 * generated per-type copy and the polymorphic asdf_gwcs_transform_copy fill in
 * the base fields. */
static bool asdf_gwcs_transform_copy_base(
    asdf_file_t *file, const asdf_gwcs_transform_t *src, asdf_gwcs_transform_t *dst) {
    dst->type = src->type;
    dst->n_inputs = src->n_inputs;
    dst->n_outputs = src->n_outputs;

    /* dst may have been shallow-copied from src (see the copy shim for shallow
     * transforms), so clear the base heap pointers before taking deep copies;
     * this keeps the deinit-on-failure path free of shared (double-freed)
     * pointers. */
    dst->name = NULL;
    dst->inputs = NULL;
    dst->outputs = NULL;
    dst->bounding_box = NULL;
    dst->inverse = NULL;

    if (src->name) {
        dst->name = strdup(src->name);

        if (UNLIKELY(!dst->name))
            return false;
    }

    const char **src_io[2] = {src->inputs, src->outputs};
    const char ***dst_io[2] = {&dst->inputs, &dst->outputs};
    uint32_t io_counts[2] = {src->n_inputs, src->n_outputs};

    for (int kdx = 0; kdx < 2; kdx++) {
        if (!src_io[kdx] || io_counts[kdx] == 0)
            continue;

        const char **arr = calloc(io_counts[kdx], sizeof(char *));

        if (UNLIKELY(!arr))
            return false;

        *dst_io[kdx] = arr;

        for (uint32_t idx = 0; idx < io_counts[kdx]; idx++) {
            if (src_io[kdx][idx]) {
                arr[idx] = strdup(src_io[kdx][idx]);

                if (UNLIKELY(!arr[idx]))
                    return false;
            }
        }
    }

    if (src->bounding_box) {
        dst->bounding_box = asdf_gwcs_bounding_box_copy(file, src->bounding_box);

        if (UNLIKELY(!dst->bounding_box))
            return false;
    }

    if (src->inverse) {
        dst->inverse = asdf_gwcs_transform_copy(file, src->inverse);

        if (UNLIKELY(!dst->inverse))
            return false;
    }

    return true;
}


/* Shallow-copy the whole (POD) transform struct, then deep-copy the base
 * fields.  Used by the copy shim for transforms with no type-specific copy
 * method: their concrete type is shallow apart from the embedded base and
 * plain-old-data fields, which the memcpy copies directly. */
static bool asdf_gwcs_transform_copy_shallow(
    asdf_file_t *file, const void *src, void *dst, size_t size) {
    memcpy(dst, src, size);
    return asdf_gwcs_transform_copy_base(file, src, dst);
}


/* Recover the shim vtab (and hence the type's own methods) from an object's
 * extension.  The libasdf vtab is the first member of
 * asdf_gwcs_transform_shim_vtab_t, so ext->vtab points straight at it. */
static const asdf_gwcs_transform_shim_vtab_t *transform_shim_of(const void *obj) {
    const asdf_extension_t *ext = (const asdf_extension_t *)((const asdf_gwcs_transform_t *)obj)->type;
    return (const asdf_gwcs_transform_shim_vtab_t *)ext->vtab;
}


/* Shared copy method installed for every transform: copy the base fields plus
 * the type's own fields.  Types with no own copy method are shallow apart from
 * the base and POD fields, so the whole struct is shallow-copied. */
static bool asdf_gwcs_transform_copy_shim(asdf_file_t *file, const void *src, void *dst) {
    const asdf_gwcs_transform_shim_vtab_t *shim = transform_shim_of(src);

    if (shim->orig->copy) {
        if (!asdf_gwcs_transform_copy_base(file, src, dst))
            return false;
        return shim->orig->copy(file, src, dst);
    }

    const asdf_extension_t *ext = (const asdf_extension_t *)((const asdf_gwcs_transform_t *)src)->type;
    return asdf_gwcs_transform_copy_shallow(file, src, dst, ext->size);
}


/* Shared deinit method installed for every transform: the type's own deinit
 * (if any) followed by the base deinit. */
static void asdf_gwcs_transform_deinit_shim(void *obj) {
    const asdf_gwcs_transform_shim_vtab_t *shim = transform_shim_of(obj);

    if (shim->orig->deinit)
        shim->orig->deinit(obj);
    asdf_gwcs_transform_deinit_base((asdf_gwcs_transform_t *)obj);
}


static asdf_value_err_t asdf_gwcs_transform_serialize_base(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform, asdf_mapping_t *map);


/* Shared serialize method installed for every transform: create the mapping,
 * write the base fields, then merge in the type's own fields.  The type's own
 * serializer (a plain libasdf serializer) returns a mapping of just its own
 * fields, which is merged onto the base mapping. */
static asdf_value_t *asdf_gwcs_transform_serialize_shim(
    asdf_file_t *file, const void *obj, const void *userdata) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_transform_shim_vtab_t *shim = transform_shim_of(obj);
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    if (ASDF_IS_ERR(asdf_gwcs_transform_serialize_base(file, obj, map)))
        goto failure;

    if (shim->orig->serialize) {
        asdf_value_t *own = shim->orig->serialize(file, obj, userdata);

        if (!own)
            goto failure;

        asdf_mapping_t *own_map = NULL;
        asdf_value_err_t err = asdf_value_as_mapping(own, &own_map);

        if (!ASDF_IS_ERR(err))
            err = asdf_mapping_update(map, own_map);

        asdf_value_destroy(own);

        if (ASDF_IS_ERR(err))
            goto failure;
    }

    return asdf_value_of_mapping(map);
failure:
    asdf_mapping_destroy(map);
    return NULL;
}


/* Base deserialize: allocate an object of the concrete type's size and parse
 * the base transform fields into it. */
static asdf_value_err_t asdf_gwcs_transform_deserialize_base(
    asdf_value_t *value, asdf_gwcs_transform_type_t type, void **out) {
    const asdf_extension_t *ext = (const asdf_extension_t *)type;
    asdf_gwcs_transform_t *transform = calloc(1, ext->size);

    if (!transform)
        return ASDF_VALUE_ERR_OOM;

    transform->type = type;

    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *transform_map = NULL;

    if (asdf_value_as_mapping(value, &transform_map) != ASDF_VALUE_OK)
        goto failure;

    const char *name = NULL;
    err = asdf_get_optional_property(
        transform_map, "name", ASDF_VALUE_STRING, NULL, (void *)&name);

    if (!ASDF_IS_OPTIONAL_OK(err))
        goto failure;

    if (name) {
        transform->name = strdup(name);

        if (!transform->name) {
            err = ASDF_VALUE_ERR_OOM;
            goto failure;
        }
    }

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

    *out = transform;
    return ASDF_VALUE_OK;
failure:
    asdf_gwcs_transform_deinit(transform);
    free(transform);
    return err;
}


/* Shared deserialize method installed for every transform: allocate + parse the
 * base fields, then let the type's own deserialize fill its fields into the
 * (pre-allocated) object at *out.  Types that add no fields of their own leave
 * their vtab deserialize NULL. */
static asdf_value_err_t asdf_gwcs_transform_deserialize_shim(
    asdf_value_t *value, const void *userdata, void **out) {
    const char *tag = asdf_value_tag(value);

    if (!tag)
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    asdf_gwcs_transform_type_t type = asdf_gwcs_transform_type_get(tag);

    if (type == ASDF_GWCS_TRANSFORM_INVALID)
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    const asdf_extension_t *ext = (const asdf_extension_t *)type;

    asdf_value_err_t err = asdf_gwcs_transform_deserialize_base(value, type, out);

    if (ASDF_IS_ERR(err))
        return err;

    const asdf_gwcs_transform_shim_vtab_t *shim =
        (const asdf_gwcs_transform_shim_vtab_t *)ext->vtab;

    if (shim->orig->deserialize) {
        err = shim->orig->deserialize(value, userdata, out);

        if (ASDF_IS_ERR(err)) {
            asdf_gwcs_transform_deinit(*out);
            free(*out);
            *out = NULL;
            return err;
        }
    }

    return ASDF_VALUE_OK;
}


void asdf_gwcs_transform_install_shim(
    asdf_extension_t *ext, asdf_gwcs_transform_shim_vtab_t *shim) {
    shim->vtab.serialize = asdf_gwcs_transform_serialize_shim;
    shim->vtab.deserialize = asdf_gwcs_transform_deserialize_shim;
    shim->vtab.copy = asdf_gwcs_transform_copy_shim;
    shim->vtab.deinit = asdf_gwcs_transform_deinit_shim;
    ext->vtab = &shim->vtab;
}


bool asdf_gwcs_transform_copy_into(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform, asdf_gwcs_transform_t *copy) {
    if (!transform || !copy)
        return false;

    const asdf_extension_t *ext = (const asdf_extension_t *)transform->type;

    /* The registered vtab copy method is a shim that copies the base fields
     * and then the concrete type's own fields (see ASDF_GWCS_REGISTER_TRANSFORM).
     * A transform with no registered copy is a bare base transform. */
    if (ext && ext->vtab && ext->vtab->copy) {
        if (!ext->vtab->copy(file, transform, copy))
            goto failure;
    } else if (!asdf_gwcs_transform_copy_base(file, transform, copy)) {
        goto failure;
    }

    return true;
failure:
    asdf_gwcs_transform_deinit(copy);
    return false;
}


asdf_gwcs_transform_t *asdf_gwcs_transform_copy(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform) {

    if (!transform)
        return NULL;

    const asdf_extension_t *ext = (const asdf_extension_t *)transform->type;

    if (UNLIKELY(!ext))
        return NULL;

    asdf_gwcs_transform_t *copy = calloc(ext->size, sizeof(uint8_t));

    if (UNLIKELY(!copy))
        return NULL;

    if (!asdf_gwcs_transform_copy_into(file, transform, copy)) {
        free(copy);
        return NULL;
    }

    return copy;
}


void asdf_gwcs_transform_arity_set(
    asdf_gwcs_transform_t *transform,
    const asdf_file_t *file,
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


void asdf_gwcs_transform_deinit(asdf_gwcs_transform_t *transform) {
    if (!transform)
        return;

    const asdf_extension_t *ext = (const asdf_extension_t *)transform->type;

    if (ext && ext->vtab && ext->vtab->deinit) {
        ext->vtab->deinit(transform);
        return;
    }

    asdf_gwcs_transform_deinit_base(transform);
}


void asdf_gwcs_transform_destroy(asdf_gwcs_transform_t *transform) {
    asdf_gwcs_transform_deinit(transform);
    free(transform);
}


asdf_value_err_t asdf_value_as_gwcs_transform(asdf_value_t *value, asdf_gwcs_transform_t **out) {
    const char *tag_str = asdf_value_tag(value);

    if (UNLIKELY(!tag_str))
        return ASDF_VALUE_ERR_TYPE_MISMATCH;

    const asdf_extension_t *ext = asdf_extension_get(asdf_value_file(value), tag_str);

    if (ext)
        return asdf_value_as_extension_type(value, ext, (void **)out);

    return ASDF_VALUE_ERR_TYPE_MISMATCH;
}



const char *asdf_gwcs_transform_type_to_tag(asdf_gwcs_transform_type_t type) {
    const char *full_tag = NULL;
    const asdf_extension_t *ext = (const asdf_extension_t *)type;

    if (!ext)
        return NULL;

    /* tags[0] is the tag written when serializing */
    full_tag = tag_canonicalize(ext->tags[0]);

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


static asdf_value_err_t asdf_gwcs_transform_serialize_base(
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
    const char *const *tags = ((asdf_extension_t *)type)->tags;

    for (const char *const *tag = tags; *tag; tag++) {
        char *full_tag = tag_canonicalize(*tag);
        if (!full_tag) {
            ASDF_LOG(NULL, ASDF_LOG_FATAL, "failed to allocate memory for transform tag %s", *tag);
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


/* A bare/generic transform has only base fields, so all methods are NULL; the
 * registration shim supplies the base serialize/deserialize/copy/deinit. */
static const asdf_extension_vtab_t asdf_gwcs_transform_generic_vtab = {
    .serialize = NULL,
    .deserialize = NULL,
    .copy = NULL,
    .deinit = NULL,
};


/** Register a "generic" transform
 *
 * These don't have any special implementation details at the moment, and
 * are registered mainly so that their tags are recognized as known
 * transforms.
 *
 * One or more tags may be given; the first is the one written on serialize,
 * and the rest are additional versions recognized when reading.
 */
#define ASDF_GWCS_REGISTER_TRANSFORM_GENERIC(extname, ttype, ...) \
    ASDF_GWCS_REGISTER_TRANSFORM( \
        extname, ttype, asdf_gwcs_transform_t, \
        &libasdf_gwcs_software, \
        &asdf_gwcs_transform_generic_vtab, \
        NULL, \
        __VA_ARGS__);


#define ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(extname, ttype, ctype, ...) \
    ASDF_GWCS_REGISTER_TRANSFORM_WITH_CTYPE( \
        extname, ttype, asdf_gwcs_transform_t, \
        &libasdf_gwcs_software, \
        &asdf_gwcs_transform_generic_vtab, \
        ctype, \
        __VA_ARGS__);


/**
 * Register extension for the base transform type
 *
 * Transform subtypes are registered through ASDF_GWCS_REGISTER_TRANSFORM
 *
 * NOTE: The main differences between schema versions are additional (optional)
 * properties added at different versions.  Not all transform properties are
 * fully supported yet, though each version is nominally supported.
 */
ASDF_GWCS_REGISTER_TRANSFORM_GENERIC(
    transform_generic,
    GENERIC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "transform-1.0.0"
);


/**
 * Register additional known transforms as generic transforms
 *
 * NOTE: Most of these schemas' versions differ primarily in the
 * base transform schema version.  Nominally all current versions are
 * supported, but differences between the versions are not fully realized
 * in the deserializers.
 *
 * TODO: Would be useful in libasdf to have a convenience macro to expand
 * to multiple versions of the same tag...
 */
ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    airy,
    AIRY,
    AIR,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "airy-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    bonne_equal_area,
    BONNE_EQUAL_AREA,
    BON,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "bonne_equal_area-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    cobe_quad_spherical_cube,
    COBE_QUAD_SPHERICAL_CUBE,
    CSC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cobe_quad_spherical_cube-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_equal_area,
    CONIC_EQUAL_AREA,
    COE,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equal_area-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_equidistant,
    CONIC_EQUIDISTANT,
    COD,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_equidistant-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_orthomorphic,
    CONIC_ORTHOMORPHIC,
    COO,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_orthomorphic-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    conic_perspective,
    CONIC_PERSPECTIVE,
    COP,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "conic_perspective-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    cylindrical_equal_area,
    CYLINDRICAL_EQUAL_AREA,
    CEA,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_equal_area-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    cylindrical_perspective,
    CYLINDRICAL_PERSPECTIVE,
    CYP,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "cylindrical_perspective-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    gnomonic,
    GNOMONIC,
    TAN,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "gnomonic-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    hammer_aitoff,
    HAMMER_AITOFF,
    AIT,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "hammer_aitoff-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    healpix_polar,
    HEALPIX_POLAR,
    XPH,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "healpix_polar-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    molleweide,
    MOLLEWEIDE,
    MOL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "molleweide-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    parabolic,
    PARABOLIC,
    PAR,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "parabolic-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    plate_carree,
    PLATE_CARREE,
    CAR,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "plate_carree-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    polyconic,
    POLYCONIC,
    PCO,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "polyconic-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    quad_spherical_cube,
    QUAD_SPHERICAL_CUBE,
    QSC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "quad_spherical_cube-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "quad_spherical_cube-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "quad_spherical_cube-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "quad_spherical_cube-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "quad_spherical_cube-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    sanson_flamsteed,
    SANSON_FLAMSTEED,
    SFL,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "sanson_flamsteed-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    slant_orthographic,
    SLANT_ORTHOGRAPHIC,
    SIN,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_orthographic-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    slant_zenithal_perspective,
    SLANT_ZENITHAL_PERSPECTIVE,
    SZP,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_zenithal_perspective-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_zenithal_perspective-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_zenithal_perspective-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_zenithal_perspective-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "slant_zenithal_perspective-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    stereographic,
    STEREOGRAPHIC,
    STG,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "stereographic-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    tangential_spherical_cube,
    TANGENTIAL_SPHERICAL_CUBE,
    TSC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "tangential_spherical_cube-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "tangential_spherical_cube-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "tangential_spherical_cube-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "tangential_spherical_cube-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "tangential_spherical_cube-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    zenithal_equal_area,
    ZENITHAL_EQUAL_AREA,
    ZEA,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equal_area-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equal_area-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equal_area-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equal_area-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equal_area-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    zenithal_equidistant,
    ZENITHAL_EQUIDISTANT,
    ARC,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equidistant-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equidistant-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equidistant-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equidistant-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_equidistant-1.0.0"
);

ASDF_GWCS_REGISTER_TRANSFORM_GENERIC_WITH_CTYPE(
    zenithal_perspective,
    ZENITHAL_PERSPECTIVE,
    AZP,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective-1.5.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective-1.4.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective-1.3.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective-1.2.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective-1.1.0",
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "zenithal_perspective-1.0.0"
);
