AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/third_party/STC/include \
    -I$(top_srcdir)/src

munit_dir = $(top_builddir)/tests/munit
munit_submodule = $(munit_dir)/.git
munit_cflags = -std=gnu11 -I$(top_srcdir)/tests

$(munit_submodule):
	@if test -d $(top_srcdir)/.git; then \
	    echo "Initializing submodule tests/munit ..."; \
	    git submodule update --init tests/munit; \
	else \
	    echo "Skipping submodule init: not in a Git repository"; \
	fi

check_PROGRAMS = test-gwcs.unit test-gwcs-eval.unit

TESTS_ENVIRONMENT = top_builddir=$(top_builddir) top_srcdir=$(top_srcdir)
TESTS = $(check_PROGRAMS)

unit_test_cppflags = \
    $(AM_CPPFLAGS) \
    -DREFERENCE_FILES_DIR=\"$(abs_srcdir)/fixtures\" \
    -DFIXTURES_DIR=\"$(abs_srcdir)/fixtures\" \
    -DTEMP_DIR=\"$(abs_builddir)/tmp\" \
    -D_GNU_SOURCE

unit_test_cflags = $(munit_cflags) $(LIBASDF_CFLAGS) $(ASDF_GWCS_CFLAGS)
unit_test_ldflags = $(LDFLAGS)

if DEBUG
unit_test_cflags += -DMUNIT_NO_FORK
unit_test_ldflags += -rdynamic
endif

unit_test_ldadd = libmunit.a $(top_builddir)/libasdf-gwcs.la $(LIBASDF_LIBS)
if HAVE_AST
unit_test_ldadd += $(AST_LIBS) -lm
endif

check_LIBRARIES = libmunit.a
libmunit_a_SOURCES = util.c $(munit_dir)/munit.c
libmunit_a_CPPFLAGS = $(unit_test_cppflags)
libmunit_a_CFLAGS = $(unit_test_cflags) -w

# test-gwcs.unit
test_gwcs_unit_SOURCES = test-gwcs.c
test_gwcs_unit_CPPFLAGS = $(unit_test_cppflags)
test_gwcs_unit_CFLAGS = $(unit_test_cflags)
test_gwcs_unit_LDFLAGS = $(unit_test_ldflags)
test_gwcs_unit_LDADD = $(unit_test_ldadd)

# test-gwcs-eval.unit
test_gwcs_eval_unit_SOURCES = test-gwcs-eval.c
test_gwcs_eval_unit_CFLAGS = $(unit_test_cflags) $(AST_CFLAGS)
test_gwcs_eval_unit_LDFLAGS = $(unit_test_ldflags)
test_gwcs_eval_unit_LDADD = $(unit_test_ldadd) $(AST_LIBS)

if HAVE_AST
test_gwcs_eval_unit_CPPFLAGS = $(unit_test_cppflags) -DHAVE_AST
else
test_gwcs_eval_unit_CPPFLAGS = $(unit_test_cppflags)
endif

roman_build21_fixtures = \
    fixtures/roman/build21/r0000101001001001001_0001_wfi01_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi02_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi03_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi04_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi05_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi06_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi07_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi08_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi09_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi10_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi11_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi12_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi13_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi14_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi15_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi16_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi17_f158_cal.asdf \
    fixtures/roman/build21/r0000101001001001001_0001_wfi18_f158_cal.asdf

roman_build21_refs = \
    fixtures/roman/build21/ast_wcs_results.csv \
    fixtures/roman/build21/gwcs_wcs_results.csv

EXTRA_DIST = \
    $(roman_build21_fixtures) \
    $(roman_build21_refs) \
    scripts/create_roman_fixture.py

check-local: $(munit_submodule)
