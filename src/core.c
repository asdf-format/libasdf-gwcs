#include "asdf/gwcs/core.h"


const char *asdf_gwcs_strerror(asdf_gwcs_err_t err) {
    switch (err) {
    case ASDF_GWCS_OK:
        return "no error";
    case ASDF_GWCS_ERR_OOM:
        return "out of memory";
    case ASDF_GWCS_ERR_NOT_IMPLEMENTED:
        return "operation not implemented";
    case ASDF_GWCS_ERR_BACKEND_NOT_AVAILABLE:
        return "no evaluation backend available";
    case ASDF_GWCS_ERR_TRANSFORM_NOT_SUPPORTED:
        return "transform type not supported by the backend";
    case ASDF_GWCS_ERR_PARSE_FAILED:
        return "backend failed to load the WCS";
    case ASDF_GWCS_ERR_EVALUATION_FAILED:
        return "coordinate evaluation failed";
    }

    return "unknown error";
}
