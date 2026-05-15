/**
 * Core types shared across all libasdf-gwcs public headers
 */
#ifndef ASDF_GWCS_CORE_H
#define ASDF_GWCS_CORE_H

#include <asdf/extension.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS


/** Prefix for all GWCS schema tags */
#define ASDF_GWCS_TAG_PREFIX "tag:stsci.edu:gwcs/"


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


ASDF_END_DECLS

#endif /* ASDF_GWCS_CORE_H */
