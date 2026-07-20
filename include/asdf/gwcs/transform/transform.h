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

#ifndef ASDF_GWCS_TRANSFORM_TRANSFORM_H
#define ASDF_GWCS_TRANSFORM_TRANSFORM_H

#include <asdf/core/asdf.h>
#include <asdf/extension.h>
#include <asdf/gwcs/transform/property/bounding_box.h>

ASDF_BEGIN_DECLS


#define ASDF_GWCS_TRANSFORM_TAG_PREFIX ASDF_STANDARD_TAG_PREFIX "transform/"

// Forward-declaration
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


typedef struct {
    /** Optional FITS WCS CTYPE value associated with a transform */
    const char *ctype;
} asdf_gwcs_transform_data_t;


static const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_INVALID;


typedef struct asdf_gwcs_transform {
    /**
     * Enum value specifying the type of transform
     */
    asdf_gwcs_transform_type_t type;

    /**
     * A human-readable name for the transform (may be `NULL`)
     *
     * This an other fields up through `input_units_equivalencies` are from
     * the base `stsci.edu/transform/transform-1.4.0 schema` but are not
     * fully implemented (many of them are not used or needed for FITS WCS
     * and will likely be moved later into a base transform field, but they
     * are laid out here with future ABI compatibility in mind...)
     * */
    const char *name;

    /**
     * Not yet implemented, but would be an inverse transform
     * (currently always `NULL`)
     * */
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


ASDF_EXPORT void asdf_gwcs_transform_register(asdf_gwcs_transform_type_t type);
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
typedef struct {
    asdf_extension_vtab_t vtab;
    const asdf_extension_vtab_t *orig;
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
#define ASDF_GWCS_TRANSFORM_SHIM_VTAB(extname, ext_vtab) \
    static asdf_gwcs_transform_shim_vtab_t asdf_gwcs_##extname##_shim_vtab = { \
        .orig = (ext_vtab), \
    }


/*
 * Register a transform "subclass".
 *
 * ``ext_vtab`` is a pointer to the transform's own `asdf_extension_vtab_t`; its
 * methods handle only the type's own fields (a serializer returns a mapping of
 * just those fields), and the base transform handling is supplied centrally by
 * the installed shim (see `asdf_gwcs_transform_install_shim`).
 *
 * TODO: this is a stopgap until libasdf formalizes extension hierarchies and
 * can generate this itself.
 */
#define ASDF_GWCS_REGISTER_TRANSFORM( \
    extname, ttype, type, software, ext_vtab, userdata, ...) \
    ASDF_GWCS_TRANSFORM_SHIM_VTAB(extname, ext_vtab); \
    ASDF_REGISTER_EXTENSION(gwcs_##extname, type, software, \
        &asdf_gwcs_##extname##_shim_vtab.vtab, userdata, __VA_ARGS__); \
    const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_##ttype = \
        (asdf_gwcs_transform_type_t)&ASDF_EXT_STATIC_NAME(gwcs_##extname); \
    static ASDF_CONSTRUCTOR void asdf_gwcs_transform_register_##extname(void) { \
        asdf_gwcs_transform_install_shim( \
            &ASDF_EXT_STATIC_NAME(gwcs_##extname), \
            &asdf_gwcs_##extname##_shim_vtab); \
        asdf_gwcs_transform_register( \
            (asdf_gwcs_transform_type_t)&ASDF_EXT_STATIC_NAME(gwcs_##extname)); \
    }


#define ASDF_GWCS_REGISTER_TRANSFORM_WITH_CTYPE(extname, ttype, type, software, vtab, _ctype, ...) \
    static const asdf_gwcs_transform_data_t asdf_gwcs_##extname##_transform_data = { \
        .ctype = (#_ctype) \
    }; \
    ASDF_GWCS_REGISTER_TRANSFORM( \
        extname, ttype, type, software, vtab, (void *)&asdf_gwcs_##extname##_transform_data, \
        __VA_ARGS__);


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
