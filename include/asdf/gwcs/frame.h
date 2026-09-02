/**
 * Partial implementation of version 1.2.0 of the :gwcs-schema:`gwcs/frame
 * <frame-1.0.0>` schema
 *
 * .. warning::
 *
 *   This API is still in progress--in particular the use of a frame type
 *   enum may not prove stable in the next releases.
 */

//

#ifndef ASDF_GWCS_FRAME_H
#define ASDF_GWCS_FRAME_H

#include <stdint.h>

#include <asdf/file.h>
#include <asdf/extension.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS


/**
 * Enum tagging which type of frame an `asdf_gwcs_frame_t` actually contains
 *
 * Check this before downcasting a frame pointer to a concrete frame type.
 */
typedef enum {
    /** A bare ``gwcs/frame`` with no additional structure */
    ASDF_GWCS_FRAME_GENERIC,
    /** An `asdf_gwcs_frame2d_t` (``gwcs/frame2d``) */
    ASDF_GWCS_FRAME_2D,
    /** An `asdf_gwcs_frame_celestial_t` (``gwcs/celestial_frame``) */
    ASDF_GWCS_FRAME_CELESTIAL,
} asdf_gwcs_frame_type_t;

/**
 * Base type for all GWCS coordinate frames
 *
 * Concrete frame types embed this as their first member, so a frame pointer
 * may be cast to the concrete type once ``type`` has been checked:
 *
 * .. code-block:: c
 *
 *    if (frame->type == ASDF_GWCS_FRAME_2D) {
 *        const asdf_gwcs_frame2d_t *f2d = (const asdf_gwcs_frame2d_t *)frame;
 *    }
 */
typedef struct {
    /** Which concrete frame type this is */
    asdf_gwcs_frame_type_t type;

    /** A human-readable name for the frame (may be ``NULL``) */
    const char *name;
} asdf_gwcs_frame_t;


/**
 * Embed the common frame fields in a concrete frame type
 *
 * Mirrors ``ASDF_GWCS_TRANSFORM_BASE`` and
 * ``ASDF_GWCS_COORDINATE_FRAME_BASE``.  The fields are reachable both
 * directly and through an explicit ``.base``, so a concrete frame can be
 * initialized flatly::
 *
 *   asdf_gwcs_frame2d_t detector = {
 *       .type = ASDF_GWCS_FRAME_2D, .name = "detector",
 *       .axes_names = {"x", "y"},
 *   };
 *
 * while ``&detector.base`` still yields an `asdf_gwcs_frame_t` pointer, which
 * is how a concrete frame is stored in an `asdf_gwcs_step_t` without a cast.
 *
 * The field list must match `asdf_gwcs_frame_t` exactly; static assertions in
 * ``src/frame.c`` fail the build if the two drift apart.
 */
#define ASDF_GWCS_FRAME_BASE \
    union { \
        asdf_gwcs_frame_t base; \
        struct { \
            asdf_gwcs_frame_type_t type; \
            const char *name; \
        }; \
    }


/* Extension name kept as gwcs_base_frame to avoid colliding with the
 * polymorphic asdf_gwcs_frame_destroy. */
ASDF_DECLARE_EXTENSION(gwcs_base_frame, asdf_gwcs_frame_t);

/**
 * Return the short name of a frame's type
 *
 * This is the frame's schema name with neither the tag's namespace nor its
 * version: ``frame``, ``frame2d``, ``celestial_frame``.  Unlike the ``name``
 * member of `asdf_gwcs_frame_t`, which is an optional user-supplied label for
 * one particular frame, this identifies what kind of frame it is.
 *
 * The returned string is static and must not be freed.
 *
 * :param frame: The frame to inspect
 * :return: The type name, or ``NULL`` if ``frame`` is ``NULL``
 */
ASDF_EXPORT const char *asdf_gwcs_frame_type_name(const asdf_gwcs_frame_t *frame);

/**
 * Polymorphic value constructor: dispatches to the appropriate typed
 * asdf_value_of_gwcs_frame* function based on frame->type.
 */
ASDF_EXPORT asdf_value_t *asdf_gwcs_frame_value_of(
    asdf_file_t *file, const asdf_gwcs_frame_t *frame);

/**
 * Polymorphic cast of a generic value to whichever frame type it is tagged as
 *
 * Unlike ``asdf_value_as_gwcs_base_frame``, this recognizes any of the
 * concrete frame tags and sets the resulting object's ``type`` accordingly, so
 * that the result may be downcast.
 *
 * :param value: The value to convert
 * :param out: Receives the new `asdf_gwcs_frame_t`, owned by the caller
 * :return: ``ASDF_VALUE_OK`` on success
 */
ASDF_EXPORT asdf_value_err_t asdf_value_as_gwcs_frame(asdf_value_t *value, asdf_gwcs_frame_t **out);

/**
 * Polymorphic deep copy of a frame of any type
 *
 * :param file: The file the copy is associated with
 * :param frame: The frame to copy
 * :return: A newly allocated copy owned by the caller, or ``NULL`` on failure
 */
ASDF_EXPORT asdf_gwcs_frame_t *asdf_gwcs_frame_copy(asdf_file_t *file, const asdf_gwcs_frame_t *frame);

/**
 * Polymorphic deep copy into caller-provided storage
 *
 * ``copy`` must point to storage large enough for ``frame``'s concrete type.
 *
 * :param file: The file the copy is associated with
 * :param frame: The frame to copy
 * :param copy: Destination storage
 * :return: ``true`` on success
 */
ASDF_EXPORT bool asdf_gwcs_frame_copy_into(
    asdf_file_t *file, const asdf_gwcs_frame_t *frame, asdf_gwcs_frame_t *copy);

/**
 * Release what a frame owns, without freeing the frame itself
 *
 * Use this for a frame embedded in storage you allocated yourself; use
 * `asdf_gwcs_frame_destroy` for one returned by this library.
 *
 * :param frame: The frame to de-initialize
 */
ASDF_EXPORT void asdf_gwcs_frame_deinit(asdf_gwcs_frame_t *frame);

/**
 * Free a frame of any type, along with everything it owns
 *
 * :param frame: The frame to destroy
 */
ASDF_EXPORT void asdf_gwcs_frame_destroy(asdf_gwcs_frame_t *frame);

ASDF_END_DECLS

#endif /* ASDF_GWCS_FRAME_H */
