/**
 * Representation of the astropy/coordinates/frames/baseframe-1.0.0 schema
 * and its derived concrete coordinate frame types.
 */

#ifndef ASDF_GWCS_COORDINATES_BASEFRAME_H
#define ASDF_GWCS_COORDINATES_BASEFRAME_H

#include <asdf/extension.h>
#include <asdf/util.h>
#include <asdf/value.h>

ASDF_BEGIN_DECLS

#define ASDF_COORDINATES_TAG_PREFIX \
    "tag:astropy.org:astropy/coordinates/frames/"

typedef struct {
    asdf_extension_t ext;
} asdf_gwcs_coordinate_frame_ext_t;

/**
 * Opaque type token identifying a concrete coordinate frame schema.
 *
 * Plays the same role as `asdf_gwcs_transform_type_t` in the transform
 * subsystem: each registered coordinate frame schema gets a unique constant
 * of this type at load time.
 */
typedef asdf_gwcs_coordinate_frame_ext_t *asdf_gwcs_coordinate_frame_type_t;


/**
 * Base struct for all astropy coordinate frame objects.
 *
 * Concrete frames whose schema defines no additional ``frame_attributes``
 * (ICRS, Galactic, Supergalactic, BarycentricMeanEcliptic) may be
 * represented directly as ``asdf_gwcs_baseframe_t``.  Frames that carry
 * additional data embed this struct as their first member.
 */
typedef struct {
    asdf_gwcs_coordinate_frame_type_t type;
} asdf_gwcs_baseframe_t;


/**
 * Helper macro for embedding the base frame fields inside a concrete type.
 */
#define ASDF_GWCS_COORDINATE_FRAME_BASE \
    union { \
        asdf_gwcs_baseframe_t base; \
        struct { asdf_gwcs_coordinate_frame_type_t type; }; \
    }


/**
 * Register a concrete coordinate frame schema.
 *
 * Mirrors ``ASDF_GWCS_REGISTER_TRANSFORM``.  The ``extname`` is
 * prefixed with ``coordinates_`` internally so there is no collision with
 * other extension namespaces.
 */
#define ASDF_GWCS_REGISTER_COORDINATE_FRAME(extname, ttype, ctype, software, vtab, userdata, ...) \
    ASDF_REGISTER_EXTENSION(coordinates_##extname, ctype, software, vtab, userdata, __VA_ARGS__); \
    const asdf_gwcs_coordinate_frame_type_t ASDF_GWCS_COORDINATE_FRAME_##ttype = \
        (asdf_gwcs_coordinate_frame_type_t)&ASDF_EXT_STATIC_NAME(coordinates_##extname); \
    static ASDF_CONSTRUCTOR void asdf_gwcs_coordinate_frame_register_##extname(void) { \
        asdf_gwcs_coordinate_frame_register( \
            (asdf_gwcs_coordinate_frame_type_t)&ASDF_EXT_STATIC_NAME(coordinates_##extname)); \
    }


/**
 * Declare an already-registered coordinate frame type (for use in headers).
 */
#define ASDF_GWCS_DECLARE_COORDINATE_FRAME(extname, ttype, ctype) \
    ASDF_DECLARE_EXTENSION(coordinates_##extname, ctype); \
    ASDF_EXPORT extern const asdf_gwcs_coordinate_frame_type_t ASDF_GWCS_COORDINATE_FRAME_##ttype;


/* Coordinate frames with empty frame_attributes */
ASDF_GWCS_DECLARE_COORDINATE_FRAME(icrs, ICRS, asdf_gwcs_baseframe_t);
ASDF_GWCS_DECLARE_COORDINATE_FRAME(galactic, GALACTIC, asdf_gwcs_baseframe_t);

/* Coordinate frames with frame_attributes */
ASDF_GWCS_DECLARE_COORDINATE_FRAME(fk5, FK5, asdf_gwcs_baseframe_t);
ASDF_GWCS_DECLARE_COORDINATE_FRAME(fk4, FK4, asdf_gwcs_baseframe_t);
ASDF_GWCS_DECLARE_COORDINATE_FRAME(fk4noeterms, FK4_NO_E, asdf_gwcs_baseframe_t);


ASDF_EXPORT void asdf_gwcs_coordinate_frame_register(asdf_gwcs_coordinate_frame_type_t type);

/**
 * Deserialize any recognized coordinate frame value into its C representation.
 */
ASDF_EXPORT asdf_value_err_t asdf_value_as_gwcs_coordinate_frame(
    asdf_value_t *value, asdf_gwcs_baseframe_t **out);

/**
 * Serialize a coordinate frame to a tagged ASDF value.
 */
ASDF_EXPORT asdf_value_t *asdf_gwcs_coordinate_frame_value_of(
    asdf_file_t *file, const asdf_gwcs_baseframe_t *frame);

/**
 * Deep-copy any recognized coordinate frame, dispatching on its type.
 */
ASDF_EXPORT asdf_gwcs_baseframe_t *asdf_gwcs_coordinate_frame_copy(
    asdf_file_t *file, const asdf_gwcs_baseframe_t *frame);

/**
 * Release resources held by a coordinate frame's fields
 */
ASDF_EXPORT void asdf_gwcs_coordinate_frame_deinit(asdf_gwcs_baseframe_t *frame);

/**
 * Release resources held by a coordinate frame and deallocate
 */
ASDF_EXPORT void asdf_gwcs_coordinate_frame_destroy(asdf_gwcs_baseframe_t *frame);


ASDF_END_DECLS

#endif /* ASDF_GWCS_COORDINATES_BASEFRAME_H */
