/**
 * Representation of the :frame-schema:`coordinates/frames/baseframe
 * <baseframe-1.0.0>` schema and its derived concrete coordinate frame types.
 */

//

#ifndef ASDF_GWCS_COORDINATES_BASEFRAME_H
#define ASDF_GWCS_COORDINATES_BASEFRAME_H

#include <asdf/extension.h>
#include <asdf/gwcs/core.h>
#include <asdf/util.h>
#include <asdf/value.h>

ASDF_BEGIN_DECLS

#define ASDF_COORDINATES_TAG_PREFIX \
    "tag:astropy.org:astropy/coordinates/frames/"

/**
 * Extension to the standard `asdf_extension_t` for coordinate frames
 *
 * A pointer to one of these is what an `asdf_gwcs_coordinate_frame_type_t`
 * token actually refers to.
 */
typedef struct {
    /** The underlying libasdf extension registration */
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
 * Per-type static data attached to a coordinate frame's extension registration
 *
 * The counterpart of `asdf_gwcs_transform_data_t`.  Every frame registered
 * through `ASDF_GWCS_REGISTER_COORDINATE_FRAME` gets one of these as its
 * extension ``userdata``, so a frame's own userdata moves into the
 * ``userdata`` member here.  Unlike transforms, coordinate frames have no shim
 * between libasdf and their vtab, so a frame's vtab methods receive *this*
 * struct as their ``userdata`` argument and must read ``userdata`` from it.
 */
typedef struct {
    /**
     * The frame's type name, derived from its preferred tag
     *
     * Filled in at registration from the last path element of ``tags[0]``
     * with the version suffix removed, so ``icrs-1.3.0`` becomes ``icrs``.
     * Read it with `asdf_gwcs_coordinate_frame_type_name`.
     */
    char name[ASDF_GWCS_TYPE_NAME_MAX];

    /**
     * The type token this frame was registered as
     *
     * Filled in at registration.  A frame's deserializer stamps this onto the
     * object it builds: the generated per-type accessors call the deserializer
     * directly, so without it ``type`` would be left zeroed for everything but
     * `asdf_value_as_gwcs_coordinate_frame`.
     */
    asdf_gwcs_coordinate_frame_type_t type;

    /** The ``userdata`` the frame itself registered, if any */
    const void *userdata;
} asdf_gwcs_coordinate_frame_data_t;


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
#define ASDF_GWCS_REGISTER_COORDINATE_FRAME(extname, ttype, ctype, software, vtab, _userdata, ...) \
    static asdf_gwcs_coordinate_frame_data_t asdf_gwcs_coordinates_##extname##_frame_data = { \
        .userdata = (_userdata), \
    }; \
    ASDF_REGISTER_EXTENSION(coordinates_##extname, ctype, software, vtab, \
        &asdf_gwcs_coordinates_##extname##_frame_data, __VA_ARGS__); \
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


/* Coordinate frames with empty frame_attributes.  The ones that do carry
 * frame_attributes (FK4, FK4NoETerms, FK5) have their own concrete types; see
 * <asdf/gwcs/coordinates/fk.h>. */
ASDF_GWCS_DECLARE_COORDINATE_FRAME(icrs, ICRS, asdf_gwcs_baseframe_t);
ASDF_GWCS_DECLARE_COORDINATE_FRAME(galactic, GALACTIC, asdf_gwcs_baseframe_t);


/**
 * Add a coordinate frame type to the runtime tag-to-type registry
 *
 * .. warning::
 *
 *    Internal plumbing, exported only because the registration macros expand
 *    in each frame's own translation unit.  Not intended for general use.
 *
 * :param type: The coordinate frame type token to register
 */
ASDF_EXPORT void asdf_gwcs_coordinate_frame_register(asdf_gwcs_coordinate_frame_type_t type);

/**
 * Return the short name of a coordinate frame's type
 *
 * This is the frame's schema name with neither the tag's namespace nor its
 * version: ``icrs``, ``galactic``, ``fk4noeterms``.
 *
 * The returned string is owned by the frame's extension registration and must
 * not be freed.
 *
 * :param frame: The coordinate frame to inspect
 * :return: The type name, or ``NULL`` if ``frame`` is ``NULL`` or has no type
 */
ASDF_EXPORT const char *asdf_gwcs_coordinate_frame_type_name(
    const asdf_gwcs_baseframe_t *frame);

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
