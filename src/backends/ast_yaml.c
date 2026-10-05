#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ast.h>

#include <asdf/file.h>

#include "asdf/emitter.h"
#include "asdf/gwcs/backend.h"
#include "asdf/gwcs/core.h"
#include "asdf/gwcs/wcs.h"

#include "../eval.h"
#include "../util.h"


typedef struct {
    asdf_gwcs_eval_t base;
    /** unowned at rest; the object other threads borrow in order to copy */
    AstFrameSet *frameset;
    /** stays locked to the thread that owns this context; what eval_2d uses */
    AstMapping *mapping;
    /** output frame is a SkyFrame -> outputs are in radians */
    bool is_sky_frame;
} asdf_gwcs_ast_eval_t;


static const double R2D = 180.0 / M_PI;


/* Give up the frameset, leaving it owned by no thread.
 *
 * Everything arrives locked to the calling thread, since that is how astRead
 * and astCopy hand objects back.  Releasing the frameset is what makes it
 * reachable by ast_eval_copy from another thread: a copier queues for it with
 * astLock, and a frameset this thread never released would block that copier
 * forever.  So this is required for liveness, not tidiness, and every path
 * that builds a context has to end here.
 */
static void ast_eval_rest(asdf_gwcs_ast_eval_t *ctx) {
    if (ctx->frameset)
        astUnlock(ctx->frameset, 1);
}


/* Reclaim what ast_eval_rest gave up, so the objects can be annulled. */
static void ast_eval_reacquire(asdf_gwcs_ast_eval_t *ctx) {
    if (ctx->frameset)
        astLock(ctx->frameset, 1);
}


static asdf_gwcs_err_t ast_eval_2d(
    asdf_gwcs_eval_t *self,
    const double *xin,
    const double *yin,
    double *xout,
    double *yout,
    size_t n) {

    asdf_gwcs_ast_eval_t *ctx = (asdf_gwcs_ast_eval_t *)self;

    astTran2(ctx->mapping, (int)n, xin, yin, 1, xout, yout);

    if (!astOK) {
        astClearStatus;
        return ASDF_GWCS_ERR_EVALUATION_FAILED;
    }

    /* AST SkyFrame outputs are in radians; convert to degrees. */
    if (ctx->is_sky_frame) {
        for (size_t kdx = 0; kdx < n; kdx++) {
            xout[kdx] *= R2D;
            yout[kdx] *= R2D;
        }
    }

    return ASDF_GWCS_OK;
}


static void ast_eval_destroy(asdf_gwcs_eval_t *self) {
    asdf_gwcs_ast_eval_t *ctx = (asdf_gwcs_ast_eval_t *)self;

    /* astAnnul itself tolerates an object this thread does not own, but
     * deleting the last reference to one does not (upstream bug).
     *
     * Waiting for other threads to finish and reacquiring the lock allows
     * safe deletion.
     */
    ast_eval_reacquire(ctx);

    if (ctx->mapping)
        ctx->mapping = astAnnul(ctx->mapping);

    if (ctx->frameset)
        ctx->frameset = astAnnul(ctx->frameset);

    astClearStatus;
    free(ctx);
}


/* astGetMapping normally hands back an independent astCopy, but when the base
 * and current Frames are the same it returns an astClone of a Frame the
 * FrameSet contains (frameset.c:5437).  A shared object would be unlocked out
 * from under the owner the moment the FrameSet is released, so copy
 * unconditionally.
 */
static AstMapping *ast_mapping_of(AstFrameSet *frameset) {
    AstMapping *shared = astGetMapping(frameset, AST__BASE, AST__CURRENT);

    if (!shared)
        return NULL;

    AstMapping *mapping = astCopy(shared);

    astAnnul(shared);
    return mapping;
}


static asdf_gwcs_eval_t *ast_eval_copy(asdf_gwcs_eval_t *self, asdf_gwcs_err_t *err_out) {
    asdf_gwcs_ast_eval_t *ctx = (asdf_gwcs_ast_eval_t *)self;
    asdf_gwcs_ast_eval_t *new_ctx = calloc(1, sizeof(*new_ctx));
    asdf_gwcs_err_t err = ASDF_GWCS_OK;

    if (!new_ctx) {
        err = ASDF_GWCS_ERR_OOM;
        goto done;
    }

    new_ctx->base = ctx->base;
    new_ctx->is_sky_frame = ctx->is_sky_frame;

    /* Take a handle of our own to the frameset, then wait for the object
     * through it.  astClone does not require us to own the object (it only
     * bumps a refcount under the object's secondary mutex), and astLock on
     * our own handle blocks until whichever thread is mid-copy releases it.
     * AST does the queueing, so no mutex of ours is involved. */
    AstFrameSet *handle = astClone(ctx->frameset);

    if (handle) {
        astLock(handle, 1);
        /* astCopy hands objects back already locked to the calling thread. */
        new_ctx->frameset = astCopy(handle);
        astUnlock(handle, 1);
        astAnnul(handle);
    }

    if (new_ctx->frameset)
        new_ctx->mapping = ast_mapping_of(new_ctx->frameset);

    /* An astCopy failure here is an allocation failure in practice. */
    if (!astOK || !new_ctx->frameset || !new_ctx->mapping) {
        astClearStatus;
        err = ASDF_GWCS_ERR_OOM;
        goto done;
    }

    ast_eval_rest(new_ctx);

done:
    if (err_out)
        *err_out = err;

    if (err != ASDF_GWCS_OK) {
        if (new_ctx) {
            if (new_ctx->mapping)
                astAnnul(new_ctx->mapping);

            if (new_ctx->frameset)
                astAnnul(new_ctx->frameset);

            astClearStatus;
            free(new_ctx);
        }

        return NULL;
    }

    return &new_ctx->base;
}


/* Per-thread cursor for feeding YAML lines to YamlChan */
typedef struct {
    char **lines;
    size_t idx;
    size_t nlines;
    char *buf;
} line_cursor_t;


static const char *yaml_source(void) {
    line_cursor_t *cur = astChannelData;

    if (!cur || cur->idx >= cur->nlines)
        return NULL;

    return cur->lines[cur->idx++];
}


/* Split a NUL-terminated buffer into lines; returns heap-allocated array.
   Caller frees lines[] but NOT the underlying buf. */
static char **split_lines(char *buf, size_t *nlines_out) {
    size_t cap = 64;
    size_t n = 0;
    char **lines = malloc(cap * sizeof(char *));

    if (!lines)
        return NULL;

    char *p = buf;

    while (*p) {
        char *end = strchr(p, '\n');

        if (n == cap) {
            cap *= 2;
            char **tmp = realloc(lines, cap * sizeof(char *));

            if (!tmp) {
                free(lines);
                return NULL;
            }

            lines = tmp;
        }

        lines[n++] = p;

        if (end) {
            *end = '\0';
            p = end + 1;
        } else {
            break;
        }
    }

    *nlines_out = n;
    return lines;
}


static asdf_gwcs_eval_t *ast_pipeline_create(
    UNUSED(asdf_file_t *file), const asdf_gwcs_t *wcs, asdf_gwcs_err_t *err_out) {

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_ast_eval_t *ctx = NULL;
    asdf_file_t *tmp = NULL;
    char *buf = NULL;
    size_t len = 0;
    line_cursor_t cur = {0};
    AstYamlChan *chan = NULL;
    AstObject *obj = NULL;
    AstFrame *frame = NULL;

    /* Force all ndarrays inline so AST's YamlChan can read them without
     * needing access to binary block data. */
    asdf_config_t cfg = {
        .emitter = {
            .array_storage = ASDF_ARRAY_STORAGE_INLINE, .inline_ndarray_warning_thresh = SIZE_MAX}};
    tmp = asdf_open_mem_ex(NULL, 0, &cfg);

    if (!tmp) {
        err = ASDF_GWCS_ERR_OOM;
        goto done;
    }

    asdf_set_gwcs(tmp, "wcs", wcs);

    if (asdf_write_to_mem(tmp, (void **)&buf, &len) != 0) {
        err = ASDF_GWCS_ERR_PARSE_FAILED;
        goto done;
    }

    cur.buf = buf;
    cur.lines = split_lines(cur.buf, &cur.nlines);

    if (!cur.lines) {
        err = ASDF_GWCS_ERR_OOM;
        goto done;
    }

    chan = astYamlChan(yaml_source, NULL, "");

    if (!chan || !astOK) {
        astClearStatus;
        err = ASDF_GWCS_ERR_PARSE_FAILED;
        goto done;
    }

    astPutChannelData(chan, &cur);

    obj = astRead(chan);

    if (!obj || !astOK) {
        astClearStatus;
        err = ASDF_GWCS_ERR_PARSE_FAILED;
        goto done;
    }

    if (!astIsAFrameSet(obj)) {
        err = ASDF_GWCS_ERR_TRANSFORM_NOT_SUPPORTED;
        goto done;
    }

    ctx = calloc(1, sizeof(*ctx));

    if (!ctx) {
        err = ASDF_GWCS_ERR_OOM;
        goto done;
    }

    ctx->base.eval_2d = ast_eval_2d;
    ctx->base.destroy = ast_eval_destroy;
    ctx->base.copy = ast_eval_copy;
    ctx->frameset = (AstFrameSet *)obj;
    ctx->mapping = ast_mapping_of(ctx->frameset);
    obj = NULL;

    if (!ctx->mapping || !astOK) {
        astClearStatus;
        err = ASDF_GWCS_ERR_PARSE_FAILED;
        goto done;
    }

    /* TODO: When dealing with sky frames, by default the units of the AST
     * output are in radians, while the default units in GWCS are degrees.
     * A correct fix would be to integrate proper units handling (we may
     * still want radians for example) so this is just a temporary fix to
     * get the correct results in the common case
     */
    frame = astGetFrame(ctx->frameset, AST__CURRENT);
    ctx->is_sky_frame = frame ? astIsASkyFrame(frame) : false;

    if (frame)
        astAnnul(frame);

    astClearStatus;
    ast_eval_rest(ctx);

done:
    if (chan)
        astAnnul(chan);

    if (obj)
        astAnnul(obj);

    if (tmp)
        asdf_close(tmp);

    free(cur.lines);
    free(buf);

    if (err_out)
        *err_out = err;

    if (err != ASDF_GWCS_OK) {
        if (ctx) {
            if (ctx->mapping)
                astAnnul(ctx->mapping);

            if (ctx->frameset)
                astAnnul(ctx->frameset);

            astClearStatus;
            free(ctx);
            ctx = NULL;
        }

        return NULL;
    }

    return &ctx->base;
}


static const asdf_gwcs_pipeline_vtab_t ast_pipeline_vtab = {
    .create = ast_pipeline_create,
};

ASDF_GWCS_REGISTER_BACKEND(ast_yaml, &ast_pipeline_vtab, NULL, NULL)
