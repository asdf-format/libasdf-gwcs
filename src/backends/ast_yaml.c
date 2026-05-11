#ifdef HAVE_AST

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ast.h>

#include <asdf/file.h>

#include "asdf/gwcs/backend.h"
#include "asdf/gwcs/core.h"
#include "asdf/gwcs/eval.h"
#include "asdf/gwcs/wcs.h"

#include "../eval.h"


typedef struct {
    asdf_gwcs_eval_t base;
    AstFrameSet *frameset;
    AstMapping *mapping;
} asdf_gwcs_ast_eval_t;


static asdf_gwcs_err_t ast_eval_2d(
        asdf_gwcs_eval_t *self,
        const double *xin, const double *yin,
        double *xout, double *yout, size_t n) {

    asdf_gwcs_ast_eval_t *ctx = (asdf_gwcs_ast_eval_t *)self;

    astTran2(ctx->mapping, (int)n, xin, yin, 1, xout, yout);

    if (astOK)
        return ASDF_GWCS_OK;

    astClearStatus;
    return ASDF_GWCS_ERR_EVALUATION_FAILED;
}


static void ast_eval_destroy(asdf_gwcs_eval_t *self) {
    asdf_gwcs_ast_eval_t *ctx = (asdf_gwcs_ast_eval_t *)self;

    if (ctx->mapping)
        ctx->mapping = astAnnul(ctx->mapping);

    if (ctx->frameset)
        ctx->frameset = astAnnul(ctx->frameset);

    free(ctx);
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
        asdf_file_t *file, const asdf_gwcs_t *wcs,
        asdf_gwcs_err_t *err_out) {

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_ast_eval_t *ctx = NULL;
    asdf_file_t *tmp = NULL;
    void *buf = NULL;
    size_t len = 0;
    line_cursor_t cur = {0};
    AstYamlChan *chan = NULL;
    AstObject *obj = NULL;

    tmp = asdf_open(NULL);

    if (!tmp) {
        err = ASDF_GWCS_ERR_OOM;
        goto done;
    }

    asdf_set_gwcs(tmp, "wcs", wcs);

    if (asdf_write_to_mem(tmp, &buf, &len) != 0) {
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
    ctx->frameset = (AstFrameSet *)obj;
    ctx->mapping = astGetMapping(ctx->frameset, AST__BASE, AST__CURRENT);
    obj = NULL;

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

            free(ctx);
            ctx = NULL;
        }

        return NULL;
    }

    return &ctx->base;
}


static const asdf_gwcs_pipeline_vtable_t ast_pipeline_vtable = {
    .create = ast_pipeline_create,
};

ASDF_GWCS_REGISTER_BACKEND(ast_yaml, &ast_pipeline_vtable, NULL, NULL)

#endif /* HAVE_AST */
