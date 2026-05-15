#include <math.h>
#include <stddef.h>

#include <asdf.h>
#include <asdf/gwcs/backend.h>
#include <asdf/gwcs/eval.h>
#include <asdf/gwcs/gwcs.h>

#include "munit.h"
#include "util.h"


MU_TEST(test_asdf_gwcs_backend_get_nonexistent) {
    const asdf_gwcs_backend_t *b = asdf_gwcs_backend_get("nonexistent_backend");
    assert_null(b);
    return MUNIT_OK;
}


#ifdef HAVE_AST

MU_TEST(test_asdf_gwcs_backend_get_ast_yaml) {
    const asdf_gwcs_backend_t *b = asdf_gwcs_backend_get("ast_yaml");
    assert_not_null(b);
    assert_string_equal(b->name, "ast_yaml");
    assert_not_null(b->pipeline);
    assert_not_null(b->pipeline->create);
    return MUNIT_OK;
}


MU_TEST(test_asdf_gwcs_eval_2d_roman_l3) {
    const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");
    if (!backend)
        return MUNIT_SKIP;

    const char *path = get_fixture_file_path("roman_l3_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_t *wcs = NULL;
    assert_int(asdf_get_gwcs(file, "roman/meta/wcs", &wcs), ==, ASDF_VALUE_OK);
    assert_not_null(wcs);

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, backend, &err);
    assert_int(err, ==, ASDF_GWCS_OK);
    assert_not_null(eval);

    /* 4×4 pixel grid */
    static const double xin[4] = {0.0, 1.0, 2.0, 3.0};
    static const double yin[4] = {0.0, 1.0, 2.0, 3.0};
    double xout[4], yout[4];

    assert_int(asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, 4),
               ==, ASDF_GWCS_OK);

    /* Sanity check: sky coordinates should be finite */
    for (size_t idx = 0; idx < 4; idx++) {
        assert_true(isfinite(xout[idx]));
        assert_true(isfinite(yout[idx]));
    }

    asdf_gwcs_eval_destroy(eval);
    asdf_gwcs_destroy(wcs);
    asdf_close(file);
    return MUNIT_OK;
}

#endif /* HAVE_AST */


MU_TEST_SUITE(
    gwcs_eval,
    MU_RUN_TEST(test_asdf_gwcs_backend_get_nonexistent)
#ifdef HAVE_AST
    , MU_RUN_TEST(test_asdf_gwcs_backend_get_ast_yaml)
    , MU_RUN_TEST(test_asdf_gwcs_eval_2d_roman_l3)
#endif
);


MU_RUN_SUITE(gwcs_eval);
