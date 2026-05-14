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


ASDF_EXPORT void asdf_gwcs_transform_register(asdf_gwcs_transform_type_t type);
ASDF_EXPORT const asdf_extension_t *asdf_gwcs_transform_get(asdf_file_t *file, const char *tag);


#define ASDF_GWCS_REGISTER_TRANSFORM( \
    extname, ttype, tag, type, software, serialize, deserialize, copy, dealloc, userdata) \
    ASDF_REGISTER_EXTENSION( \
        gwcs_##extname, tag, type, software, serialize, deserialize, copy, dealloc, userdata); \
    const asdf_gwcs_transform_type_t ASDF_GWCS_TRANSFORM_##ttype = \
        (asdf_gwcs_transform_type_t)&ASDF_EXT_STATIC_NAME(gwcs_##extname); \
    static ASDF_CONSTRUCTOR void asdf_gwcs_transform_register_##extname(void) { \
            asdf_gwcs_transform_register( \
                (asdf_gwcs_transform_type_t)&ASDF_EXT_STATIC_NAME(gwcs_##extname)); \
    }


#define ASDF_GWCS_REGISTER_TRANSFORM_WITH_CTYPE( \
    extname, ttype, tag, type, software, serialize, deserialize, copy, dealloc, _ctype) \
    static const asdf_gwcs_transform_data_t asdf_gwcs_##extname##_transform_data = { \
        .ctype = (#_ctype) \
    }; \
    ASDF_GWCS_REGISTER_TRANSFORM( \
        extname, ttype, tag, type, software, serialize, deserialize, copy, dealloc, \
        (void *)&asdf_gwcs_##extname##_transform_data);


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

/** 
 * TODO: Declare the remaining FITS WCS projection transform types:
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


ASDF_END_DECLS

#endif /* ASDF_GWCS_TRANSFORM_TRANSFORM_H */
