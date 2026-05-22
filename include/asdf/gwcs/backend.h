/**
 * Backend descriptor and registration interface for WCS evaluation
 */
#ifndef ASDF_GWCS_BACKEND_H
#define ASDF_GWCS_BACKEND_H

#include <asdf/extension_util.h>
#include <asdf/file.h>
#include <asdf/util.h>

#include "asdf/gwcs/core.h"
#include "asdf/gwcs/eval.h"
#include "asdf/gwcs/wcs.h"

ASDF_BEGIN_DECLS


/**
 * vtable for whole-pipeline (WCS-level) operations
 */
typedef struct {
    /**
     * Create an evaluation context for the given WCS.
     *
     * :param file: Source file handle (may be NULL).
     * :param wcs: The WCS to load into the backend.
     * :param err_out: Receives an error code on failure.
     * :return: A new `asdf_gwcs_eval_t`, or NULL on error.
     */
    asdf_gwcs_eval_t *(*create)(asdf_file_t *file, const asdf_gwcs_t *wcs,
                                asdf_gwcs_err_t *err_out);
} asdf_gwcs_pipeline_vtable_t;

/**
 * vtable for per-transform operations (optional; may be NULL on the backend)
 *
 * .. note::
 *
 *   Reserved for future use; no operations are defined yet.
 */
typedef struct {
    void *reserved;
} asdf_gwcs_transform_vtable_t;

/**
 * Descriptor for a WCS evaluation backend.
 *
 * Backends are registered at library-load time via `ASDF_GWCS_REGISTER_BACKEND`
 * and retrieved by name with `asdf_gwcs_backend_get`.
 */
struct asdf_gwcs_backend {
    /** Unique name identifying this backend (e.g. ``"ast_yaml"``). */
    const char *name;

    /** Pipeline-level operations; must not be NULL if the backend is usable. */
    const asdf_gwcs_pipeline_vtable_t *pipeline;

    /** Per-transform operations; may be NULL. */
    const asdf_gwcs_transform_vtable_t *transform;

    /** Arbitrary backend-private data. */
    void *userdata;
};


/**
 * Retrieve a registered backend by name.
 *
 * :param name: The backend name string (e.g. ``"ast_yaml"``).
 * :return: The backend descriptor, or NULL if no backend with that name is
 *   registered.
 */
ASDF_EXPORT const asdf_gwcs_backend_t *asdf_gwcs_backend_get(const char *name);

/**
 * Register a backend.
 *
 * Intended for use via `ASDF_GWCS_REGISTER_BACKEND`; not exported from the
 * shared library ABI so that third-party backends must link statically or be
 * loaded via the same process image.
 */
extern void asdf_gwcs_backend_register(const asdf_gwcs_backend_t *backend);

/**
 * Register a backend at library load time.
 *
 * Places a `asdf_gwcs_backend_t` descriptor in static storage and arranges
 * for it to be registered automatically when the library is loaded.
 *
 * :param bname: Unquoted backend name token; becomes both the C identifier
 *   suffix and the runtime name string.
 * :param pipeline_ptr: Pointer to the `asdf_gwcs_pipeline_vtable_t`.
 * :param transform_ptr: Pointer to the `asdf_gwcs_transform_vtable_t`, or NULL.
 * :param ud: Arbitrary userdata pointer stored on the backend.
 */
#define ASDF_GWCS_REGISTER_BACKEND(bname, pipeline_ptr, transform_ptr, ud) \
    static const asdf_gwcs_backend_t _asdf_gwcs_backend_##bname = {        \
        .name = #bname,                                                     \
        .pipeline = (pipeline_ptr),                                         \
        .transform = (transform_ptr),                                       \
        .userdata = (ud),                                                   \
    };                                                                      \
    ASDF_CONSTRUCTOR static void _asdf_gwcs_register_backend_##bname(void) { \
        asdf_gwcs_backend_register(&_asdf_gwcs_backend_##bname);            \
    }


ASDF_END_DECLS

#endif /* ASDF_GWCS_BACKEND_H */
