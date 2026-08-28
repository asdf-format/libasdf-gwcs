/**
 * Core types shared across all libasdf-gwcs public headers
 */

//

#ifndef ASDF_GWCS_CORE_H
#define ASDF_GWCS_CORE_H

#include <asdf/extension.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS


/** Prefix for all GWCS schema tags */
#define ASDF_GWCS_TAG_PREFIX "tag:stsci.edu:gwcs/"


/**
 * Size of the buffer holding a type name derived from a schema tag
 *
 * Type names are stored inline in each extension's per-type data struct, so
 * they are bounded; the longest name among the schemas implemented here is
 * ``slant_zenithal_perspective``, at 26 characters.
 */
#define ASDF_GWCS_TYPE_NAME_MAX 32


ASDF_EXPORT extern asdf_software_t libasdf_gwcs_software;


/**
 * Error codes returned by libasdf-gwcs functions
 */
typedef enum {
    /** No error */
    ASDF_GWCS_OK = 0,
    /** Out of memory */
    ASDF_GWCS_ERR_OOM,
    /** The requested operation is not yet implemented */
    ASDF_GWCS_ERR_NOT_IMPLEMENTED,
    /** No backend is available to evaluate the WCS */
    ASDF_GWCS_ERR_BACKEND_NOT_AVAILABLE,
    /** The WCS transform type is not supported by the backend */
    ASDF_GWCS_ERR_TRANSFORM_NOT_SUPPORTED,
    /** Backend failed to parse/load the WCS for evaluation */
    ASDF_GWCS_ERR_PARSE_FAILED,
    /** An error occurred during coordinate evaluation */
    ASDF_GWCS_ERR_EVALUATION_FAILED,
} asdf_gwcs_err_t;


/**
 * Return a short human-readable description of an `asdf_gwcs_err_t`
 *
 * The returned string is statically allocated and must not be freed.  An
 * unrecognized error code yields ``"unknown error"`` rather than ``NULL``, so
 * the result is always safe to pass to ``printf``.
 *
 * :param err: The error code to describe
 * :return: A NUL-terminated description of ``err``
 */
ASDF_EXPORT const char *asdf_gwcs_strerror(asdf_gwcs_err_t err);


ASDF_END_DECLS

#endif /* ASDF_GWCS_CORE_H */
