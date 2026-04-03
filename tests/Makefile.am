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

check_PROGRAMS = test-gwcs.unit

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
unit_test_ldadd = libmunit.a $(top_builddir)/libasdf-gwcs.la $(LIBASDF_LIBS)

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

check-local: $(munit_submodule)
