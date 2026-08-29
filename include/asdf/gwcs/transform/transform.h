/**
 * Representation of the http://stsci.edu/schemas/asdf/transform/transform-1.4.0
 * base schema for all GWCS transforms.
 *
 * .. todo::
 *
 *   This is not fully implemented yet and is included right now with future
 *   ABI compatibility in mind.
 *
 *   Specific transforms should use this as a base member.
 */

//

#ifndef ASDF_GWCS_TRANSFORM_TRANSFORM_H
#define ASDF_GWCS_TRANSFORM_TRANSFORM_H

#include <asdf/core/asdf.h>
#include <asdf/extension.h>
#include <asdf/gwcs/core.h>
#include <asdf/gwcs/transform/property/bounding_box.h>

ASDF_BEGIN_DECLS


#define ASDF_GWCS_TRANSFORM_TAG_PREFIX ASDF_STANDARD_TAG_PREFIX "transform/"

/**
 * The base transform type
 *
 * Forward-declared here so that the transform headers can refer to it before
 * it is defined; see :c:struct:`asdf_gwcs_transform` below for the fields.
 */
typedef struct asdf_gwcs_transform asdf_gwcs_transform_t;


/** Extension to standard `asdf_extension_t` for transforms */
typedef struct {
    asdf_extension_t ext;
} asdf_gwcs_transform_ext_t;


/** Object representing a unique identifier for a transform
 *
 * This serves like an enum value uniquely identifying a transform type,
 * but its value is set by the transform registration system as a global
 * constant.  The actual value is intended to be opaque.
 */
typedef asdf_gwcs_transform_ext_t *asdf_gwcs_transform_type_t;


/**
 * Per-type static data attached to a transform's extension registration
 *
 * Every transform registered through ``ASDF_GWCS_REGISTER_TRANSFORM`` gets one
 * of these as its extension ``userdata``, so a transform's own userdata moves
 * into the ``userdata`` member here.  The serialize and deserialize shims
 * unwrap it again, so a transform's own vtab methods still receive the
 * pointer the registration passed.
 */
typedef struct {
    /**
     * The transform's type name, derived from its preferred tag
     *
     * Filled in at registration from the last path element of ``tags[0]``
     * with the version suffix removed, so ``affine-1.5.0`` becomes
     * ``affine``.  Read it with `asdf_gwcs_transform_type_name`.
     */
    char name[ASDF_GWCS_TYPE_NAME_MAX];

    /** Optional FITS WCS CTYPE value associated with a transform */
    const char *ctype;

    /** The ``userdata`` the transform itself registered, if any */
    const void *userdata;
} asdf_gwcs_transform_data_t;


static const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_INVALID;


/**
 * Base type for all transforms
 *
 * This is the base of a tagged union: ``type`` identifies the concrete transform
 * type, and `asdf_gwcs_transform_tag` renders that as a schema tag.  Concrete
 * transform types embed these fields via ``ASDF_GWCS_TRANSFORM_BASE``, which
 * makes them reachable both directly and through an explicit ``.base``.
 *
 * Most of the fields below come from the base
 * ``stsci.edu/transform/transform-1.4.0`` schema; several are laid out for
 * future ABI compatibility and are not yet populated, as noted individually.
 */
typedef struct asdf_gwcs_transform {
    /**
     * Enum value specifying the type of transform
     */
    asdf_gwcs_transform_type_t type;

    /**
     * The full YAML tag this transform was read with (may be `NULL`)
     *
     * Each transform type is registered for several versions of its schema.
     * For a transform deserialized from a file this records the tag *as it
     * appeared in that file*, including its version, rather than the
     * preferred tag the type would be written with.  It is `NULL` for
     * transforms constructed in memory.
     *
     * Prefer `asdf_gwcs_transform_tag` to reading this directly; it falls
     * back to the type's preferred tag when this is `NULL`.
     */
    const char *tag;

    /**
     * A human-readable name for the transform (may be `NULL`)
     *
     * This and other fields up through `input_units_equivalencies` are from
     * the base ``stsci.edu/transform/transform-1.4.0`` schema but are not
     * fully implemented (many of them are not used or needed for FITS WCS
     * and will likely be moved later into a base transform field, but they
     * are laid out here with future ABI compatibility in mind...)
     */
    const char *name;

    /**
     * Not yet implemented, but would be an inverse transform
     * (currently always `NULL`)
     */
    const asdf_gwcs_transform_t *inverse;

    /** Number of input variables. */
    uint32_t n_inputs;

    /** Number of output variables. */
    uint32_t n_outputs;

    /** Array of ``n_inputs`` input variable name strings (heap-allocated). */
    const char **inputs;

    /** Array of ``n_outputs`` output variable name strings (heap-allocated). */
    const char **outputs;

    /** Bounding box of the model as `asdf_gwcs_bounding_box_t` */
    const asdf_gwcs_bounding_box_t *bounding_box;

    /** Fixed properties (not yet implemented) */
    const void *fixed;

    /** Bounded parameters (not yet implemented) */
    const void *bounds;

    /** Input unit equivalences (not yet implemented) */
    const void *input_units_equivalencies;
} asdf_gwcs_transform_t;


/**
 * Return the full YAML tag identifying a transform's type
 *
 * For a transform deserialized from a file this is the tag as it appeared in
 * that file, preserving the schema version actually used.  For a transform
 * constructed in memory it is the preferred tag its type would be serialized
 * with.
 *
 * The returned string is owned by the transform (or by its extension
 * registration) and must not be freed.
 *
 * :param transform: The transform to inspect
 * :return: The transform's tag, or ``NULL`` if ``transform`` is ``NULL`` or
 *   has no type
 */
ASDF_EXPORT const char *asdf_gwcs_transform_tag(
    const asdf_gwcs_transform_t *transform);


/**
 * Return the short name of a transform's type
 *
 * This is the transform's schema name with neither the tag's namespace nor
 * its version: ``affine``, ``compose``, ``fitswcs_imaging``.  Unlike the
 * ``name`` member of `asdf_gwcs_transform_t`, which is an optional
 * user-supplied label for one particular transform, this identifies what kind
 * of transform it is, and is the same for every transform of that type
 * regardless of which schema version it was read with.
 *
 * The returned string is owned by the transform's extension registration and
 * must not be freed.
 *
 * :param transform: The transform to inspect
 * :return: The type name, or ``NULL`` if ``transform`` is ``NULL`` or has no
 *   type
 */
ASDF_EXPORT const char *asdf_gwcs_transform_type_name(
    const asdf_gwcs_transform_t *transform);


/**
 * Iterator over the sub-transforms a composite transform is built from
 *
 * Several transforms are composites, built by combining other transforms:
 * ``compose`` and ``concatenate`` hold an ordered list of them, while
 * ``divide`` holds a numerator and a denominator.  How each stores its parts
 * differs, so they are reached through this iterator rather than through any
 * one member.
 *
 * Initialize with `asdf_gwcs_transform_iter_init`.  After each successful call
 * to `asdf_gwcs_transform_iter_next`, the fields below describe the current
 * sub-transform.
 *
 * Because sub-transforms may be composites in turn, the iterator can descend
 * into them: ``max_depth`` given to `asdf_gwcs_transform_iter_init` says how
 * far.  The walk is depth-first and pre-order, so a sub-transform is always
 * reported before anything nested inside it.
 *
 * Sub-transforms are owned by their parent: they are valid as long as it is,
 * and must not be destroyed individually.
 */
typedef struct {
    /** The current sub-transform */
    const asdf_gwcs_transform_t *value;

    /**
     * What the current sub-transform is to its parent (may be ``NULL``)
     *
     * For a composite whose parts are named, this is the name of the property
     * holding it: ``"numerator"``, ``"denominator"``, ``"projection"``.  For a
     * composite whose parts are an ordered list, such as ``compose``, the
     * parts have no individual names and this is ``NULL``.
     */
    const char *role;

    /** Index of the current sub-transform among its immediate siblings */
    uint32_t index;

    /** How many immediate siblings the current sub-transform has, itself
     * included */
    uint32_t size;

    /**
     * Tree depth of the transform being iterated
     *
     * ``0`` for an immediate sub-transform, ``1`` for a sub-transform of one
     * of those, and so on.  Always ``0`` unless a non-zero ``max_depth`` was
     * given to `asdf_gwcs_transform_iter_init`.
     */
    int depth;
} asdf_gwcs_transform_iter_t;


/**
 * Enumerate a transform type's sub-transforms
 *
 * The single method a composite transform implements.  It always returns the
 * total number of sub-transforms; when ``out`` is non-``NULL`` and ``index``
 * is less than that total, it additionally fills ``out``'s ``value`` and
 * ``role``.  Passing ``NULL`` for ``out`` is how the count is queried on its
 * own, so the count never has to be found by probing.
 *
 * :param transform: The transform to enumerate
 * :param index: Which sub-transform to report through ``out``
 * :param out: Receives the sub-transform, or ``NULL`` to query only the count
 * :return: The total number of sub-transforms
 */
typedef uint32_t (*asdf_gwcs_transform_children_t)(
    const asdf_gwcs_transform_t *transform, uint32_t index,
    asdf_gwcs_transform_iter_t *out);


/**
 * Return how many sub-transforms a transform is built from
 *
 * :param transform: The transform to inspect
 * :return: The number of sub-transforms, or ``0`` for a transform that is not
 *   a composite (or is ``NULL``)
 */
ASDF_EXPORT uint32_t asdf_gwcs_transform_n_children(const asdf_gwcs_transform_t *transform);


/**
 * Fetch a single sub-transform by index
 *
 * :param transform: The composite transform to read from
 * :param index: Which sub-transform to fetch
 * :param out: Receives the sub-transform
 * :return: ``true`` if ``index`` named a sub-transform, ``false`` otherwise
 */
ASDF_EXPORT bool asdf_gwcs_transform_get_child(
    const asdf_gwcs_transform_t *transform, uint32_t index,
    asdf_gwcs_transform_iter_t *out);


/** Pass as ``max_depth`` to descend as far as the tree goes */
#define ASDF_GWCS_DEPTH_UNLIMITED (-1)


/**
 * Create a new iterator over a transform's sub-transforms
 *
 * ``max_depth`` bounds how far the walk descends.  ``0`` visits only the
 * transform's immediate sub-transforms; ``1`` also visits theirs, and so on.
 * `ASDF_GWCS_DEPTH_UNLIMITED` walks the entire tree, which for a
 * real WCS can be a great many transforms.
 *
 * Iterating a transform that is not a composite is not an error; the
 * iteration simply yields nothing.
 *
 * :param transform: The transform whose sub-transforms to walk
 * :param max_depth: How many levels below ``transform`` to descend
 * :return: A new `asdf_gwcs_transform_iter_t *` handle, or ``NULL`` on
 *   allocation failure
 */
ASDF_EXPORT asdf_gwcs_transform_iter_t *asdf_gwcs_transform_iter_init(
    const asdf_gwcs_transform_t *transform, int max_depth);


/**
 * Advance the iterator to the next sub-transform
 *
 * Typical usage::
 *
 *   asdf_gwcs_transform_iter_t *iter = asdf_gwcs_transform_iter_init(transform, 0);
 *   while (asdf_gwcs_transform_iter_next(&iter)) {
 *       // iter->value, iter->role and iter->index are valid here
 *   }
 *
 * When iteration is exhausted the iterator is freed automatically and
 * ``*iter`` is set to ``NULL``.  For early exit call
 * `asdf_gwcs_transform_iter_destroy` before breaking.
 *
 * :param iter: Pointer to the iterator handle; set to ``NULL`` on exhaustion
 * :return: ``true`` if a sub-transform was found; ``false`` when done
 */
ASDF_EXPORT bool asdf_gwcs_transform_iter_next(asdf_gwcs_transform_iter_t **iter);


/**
 * Release resources held by an in-progress sub-transform iterator
 *
 * Call this only when breaking out of iteration early.  Safe to call with
 * ``NULL``.
 *
 * :param iter: The `asdf_gwcs_transform_iter_t *` to destroy
 */
ASDF_EXPORT void asdf_gwcs_transform_iter_destroy(asdf_gwcs_transform_iter_t *iter);


/**
 * Polymorphic value constructor: dispatches to asdf_value_of_<transform> for
 * known transforms, or uses a temporary extension for generic ones.
 */
ASDF_EXPORT asdf_value_t *asdf_value_of_gwcs_transform(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform);


/** Polymorphic transform copy */
ASDF_EXPORT asdf_gwcs_transform_t *asdf_gwcs_transform_copy(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform);

/** Polymorphic transform copy into */
ASDF_EXPORT bool asdf_gwcs_transform_copy_into(
    asdf_file_t *file, const asdf_gwcs_transform_t *transform, asdf_gwcs_transform_t *copy);

/**
 * Read an `asdf_value_t *` as any type of GWCS transform
 */
ASDF_EXPORT asdf_value_err_t asdf_value_as_gwcs_transform(
    asdf_value_t *value, asdf_gwcs_transform_t **out);

/**
 * Polymorphic transform deinit
 */
ASDF_EXPORT void asdf_gwcs_transform_deinit(asdf_gwcs_transform_t *transform);

/**
 * Polymorphic transform destroy
 */
ASDF_EXPORT void asdf_gwcs_transform_destroy(asdf_gwcs_transform_t *transform);


/*
 * NOTE: This duplicates all the fields in the definition of
 * `asdf_gwcs_transform_t`.  The duplication is a practical matter of
 *  documentation (the documentation extractor won't be able to properly
 *  document `asdf_gwcs_transform_t` otherwise. For that reason this must
 *  be kept in sync with `asdf_gwcs_transform_t`.
 */
#define ASDF_GWCS_TRANSFORM_BASE \
    union { \
        asdf_gwcs_transform_t base; \
        struct { \
            asdf_gwcs_transform_type_t type; \
            const char *tag; \
            const char *name; \
            const asdf_gwcs_transform_t *inverse; \
            uint32_t n_inputs; \
            uint32_t n_outputs; \
            const char **inputs; \
            const char **outputs; \
            const asdf_gwcs_bounding_box_t *bounding_box; \
            const void *fixed; \
            const void *bounds; \
            const void *input_units_equivalencies; \
        }; \
    }


/**
 * Add a transform type to the runtime tag-to-type registry
 *
 * .. warning::
 *
 *    Internal plumbing, exported only because the registration macros expand
 *    in each transform's own translation unit.  Not intended for general use.
 *
 * :param type: The transform type token to register
 */
ASDF_EXPORT void asdf_gwcs_transform_register(asdf_gwcs_transform_type_t type);

/**
 * Look up the extension registered for a transform tag
 *
 * .. warning::
 *
 *    Internal plumbing; not intended for general use.
 *
 * :param file: The file providing the extension context
 * :param tag: The full YAML tag to look up
 * :return: The matching extension, or ``NULL`` if the tag is unknown
 */
ASDF_EXPORT const asdf_extension_t *asdf_gwcs_transform_get(asdf_file_t *file, const char *tag);


/*
 * Storage for the shim `ASDF_GWCS_REGISTER_TRANSFORM` installs over a transform
 * extension.  ``vtab``--the libasdf-facing table ``ext->vtab`` points at--is
 * filled with shims that run the base transform handling
 * (serialize/deserialize/copy/deinit) together with the transform's own
 * ``orig`` methods, so both the polymorphic entry points and the per-type
 * accessors libasdf generates produce a complete object.  ``vtab`` must be the
 * first member so the shims can recover this struct from ``ext->vtab``.
 *
 * NOTE: This is an internal implementation detail of the transform
 * polymorphiism support.
 */
/**
 * Storage for the shim that supplies base-transform behaviour to a type
 *
 * .. warning::
 *
 *    Internal plumbing; not intended for general use.
 */
typedef struct {
    /** The vtab actually registered, whose methods are the shims */
    asdf_extension_vtab_t vtab;

    /** The transform type's own vtab, handling only its own fields */
    const asdf_extension_vtab_t *orig;

    /**
     * Enumerates the type's sub-transforms; ``NULL`` for a non-composite
     *
     * This has no counterpart in `asdf_extension_vtab_t`: sub-transforms are a
     * transform-level notion, so the method lives here rather than in the
     * libasdf-facing table.
     */
    asdf_gwcs_transform_children_t children;
} asdf_gwcs_transform_shim_vtab_t;

/*
 * Point ``ext`` at ``shim``'s ``vtab``, whose serialize/deserialize/copy/deinit
 * run the base transform handling together with ``shim``'s own methods, so that
 * both the polymorphic entry points and the per-type accessors libasdf
 * generates produce a complete object.  Called once per transform at
 * registration (normally via `ASDF_GWCS_REGISTER_TRANSFORM`).
 */
ASDF_EXPORT void asdf_gwcs_transform_install_shim(
    asdf_extension_t *ext, asdf_gwcs_transform_shim_vtab_t *shim);


/*
 * Declare the shim storage for a transform from its own `asdf_extension_vtab_t`.
 *
 * Declares a static `asdf_gwcs_transform_shim_vtab_t` whose ``orig`` points at
 * the transform's own vtab; `ASDF_GWCS_REGISTER_TRANSFORM` registers its
 * ``vtab`` member and `asdf_gwcs_transform_install_shim` fills that in.
 */
#define ASDF_GWCS_TRANSFORM_SHIM_VTAB(extname, ext_vtab, _children) \
    static asdf_gwcs_transform_shim_vtab_t asdf_gwcs_##extname##_shim_vtab = { \
        .orig = (ext_vtab), \
        .children = (_children), \
    }


/*
 * Register a transform "subclass", with every per-type datum spelled out.
 *
 * ``ext_vtab`` is a pointer to the transform's own `asdf_extension_vtab_t`; its
 * methods handle only the type's own fields (a serializer returns a mapping of
 * just those fields), and the base transform handling is supplied centrally by
 * the installed shim (see `asdf_gwcs_transform_install_shim`).
 *
 * The extension's ``userdata`` is an `asdf_gwcs_transform_data_t` carrying the
 * type name, the optional ``_ctype``, and ``_userdata``.  The shims unwrap the
 * last of these before calling ``ext_vtab``'s methods, so a transform's own
 * methods see the pointer registered here and not the wrapper.
 *
 * ``_children`` is the type's `asdf_gwcs_transform_children_t`, or NULL if the
 * transform holds no sub-transforms.
 *
 * TODO: this is a stopgap until libasdf formalizes extension hierarchies and
 * can generate this itself.
 */
#define ASDF_GWCS_REGISTER_TRANSFORM_FULL( \
    extname, ttype, type, software, ext_vtab, _ctype, _children, _userdata, ...) \
    ASDF_GWCS_TRANSFORM_SHIM_VTAB(extname, ext_vtab, _children); \
    static asdf_gwcs_transform_data_t asdf_gwcs_##extname##_transform_data = { \
        .ctype = (_ctype), \
        .userdata = (_userdata), \
    }; \
    ASDF_REGISTER_EXTENSION(gwcs_##extname, type, software, \
        &asdf_gwcs_##extname##_shim_vtab.vtab, \
        &asdf_gwcs_##extname##_transform_data, __VA_ARGS__); \
    const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_##ttype = \
        (asdf_gwcs_transform_type_t)&ASDF_EXT_STATIC_NAME(gwcs_##extname); \
    static ASDF_CONSTRUCTOR void asdf_gwcs_transform_register_##extname(void) { \
        asdf_gwcs_transform_install_shim( \
            &ASDF_EXT_STATIC_NAME(gwcs_##extname), \
            &asdf_gwcs_##extname##_shim_vtab); \
        asdf_gwcs_transform_register( \
            (asdf_gwcs_transform_type_t)&ASDF_EXT_STATIC_NAME(gwcs_##extname)); \
    }


/* Register a leaf transform: no FITS WCS CTYPE and no sub-transforms. */
#define ASDF_GWCS_REGISTER_TRANSFORM( \
    extname, ttype, type, software, ext_vtab, userdata, ...) \
    ASDF_GWCS_REGISTER_TRANSFORM_FULL( \
        extname, ttype, type, software, ext_vtab, NULL, NULL, userdata, __VA_ARGS__)


/* Register a transform that corresponds to a FITS WCS projection code. */
#define ASDF_GWCS_REGISTER_TRANSFORM_WITH_CTYPE(extname, ttype, type, software, vtab, _ctype, ...) \
    ASDF_GWCS_REGISTER_TRANSFORM_FULL( \
        extname, ttype, type, software, vtab, (#_ctype), NULL, NULL, __VA_ARGS__)


/* Register a composite transform, i.e. one built from sub-transforms. */
#define ASDF_GWCS_REGISTER_TRANSFORM_WITH_CHILDREN( \
    extname, ttype, type, software, ext_vtab, _children, userdata, ...) \
    ASDF_GWCS_REGISTER_TRANSFORM_FULL( \
        extname, ttype, type, software, ext_vtab, NULL, _children, userdata, __VA_ARGS__)


#define ASDF_GWCS_DECLARE_TRANSFORM(extname, ttype, type) \
    ASDF_DECLARE_EXTENSION(gwcs_##extname, type); \
    ASDF_EXPORT extern const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_##ttype;


#define ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(extname, ttype) \
    ASDF_GWCS_DECLARE_TRANSFORM(extname, ttype, asdf_gwcs_transform_t);


ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(transform_generic, GENERIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(airy, AIRY);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(bonne_equal_area, BONNE_EQUAL_AREA);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(cobe_quad_spherical_cube, COBE_QUAD_SPHERICAL_CUBE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(conic_equal_area, CONIC_EQUAL_AREA);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(conic_equidistant, CONIC_EQUIDISTANT);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(conic_orthomorphic, CONIC_ORTHOMORPHIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(conic_perspective, CONIC_PERSPECTIVE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(cylindrical_equal_area, CYLINDRICAL_EQUAL_AREA);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(cylindrical_perspective, CYLINDRICAL_PERSPECTIVE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(gnomonic, GNOMONIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(hammer_aitoff, HAMMER_AITOFF);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(healpix_polar, HEALPIX_POLAR);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(molleweide, MOLLEWEIDE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(parabolic, PARABOLIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(plate_carree, PLATE_CARREE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(polyconic, POLYCONIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(quad_spherical_cube, QUAD_SPHERICAL_CUBE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(sanson_flamsteed, SANSON_FLAMSTEED);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(slant_orthographic, SLANT_ORTHOGRAPHIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(slant_zenithal_perspective, SLANT_ZENITHAL_PERSPECTIVE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(stereographic, STEREOGRAPHIC);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(tangential_spherical_cube, TANGENTIAL_SPHERICAL_CUBE);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(zenithal_equal_area, ZENITHAL_EQUAL_AREA);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(zenithal_equidistant, ZENITHAL_EQUIDISTANT);
ASDF_GWCS_DECLARE_TRANSFORM_GENERIC(zenithal_perspective, ZENITHAL_PERSPECTIVE);


ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_TRANSFORM_H */
