#include <stdatomic.h>

#include <asdf/extension_util.h>

#include "asdf/gwcs/backend.h"

#include "backend.h"
#include "types/asdf_gwcs_backend_map.h"


static asdf_gwcs_backend_map_t g_backends = {0};
static atomic_bool g_backends_initialized = false;


void asdf_gwcs_backend_register(const asdf_gwcs_backend_t *backend) {
    asdf_gwcs_backend_map_emplace_or_assign(&g_backends, backend->name, backend);
}


const asdf_gwcs_backend_t *asdf_gwcs_backend_get(const char *name) {
    if (!atomic_load_explicit(&g_backends_initialized, memory_order_acquire))
        return NULL;

    const asdf_gwcs_backend_map_value *item =
        asdf_gwcs_backend_map_get(&g_backends, name);

    if (!item)
        return NULL;

    return item->second;
}


const asdf_gwcs_backend_t *asdf_gwcs_backend_get_default(void) {
    if (!atomic_load_explicit(&g_backends_initialized, memory_order_acquire))
        return NULL;

    c_foreach(it, asdf_gwcs_backend_map, g_backends)
        return it.ref->second;

    return NULL;
}


ASDF_CONSTRUCTOR static void asdf_gwcs_backends_init(void) {
    g_backends = asdf_gwcs_backend_map_init();
    atomic_store_explicit(&g_backends_initialized, true, memory_order_release);
}


ASDF_DESTRUCTOR static void asdf_gwcs_backends_destroy(void) {
    if (atomic_load_explicit(&g_backends_initialized, memory_order_acquire)) {
        asdf_gwcs_backend_map_drop(&g_backends);
        atomic_store_explicit(&g_backends_initialized, false, memory_order_release);
    }
}
