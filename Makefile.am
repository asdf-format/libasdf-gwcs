SUBDIRS = include tests third_party docs

src_files = \
    src/backend.c \
    src/coordinates/baseframe.c \
    src/coordinates/fk4.c \
    src/coordinates/fk5.c \
    src/coordinates/frame_attributes.c \
    src/core.c \
    src/eval.c \
    src/grid.c \
    src/fitswcs_imaging.c \
    src/frame.c \
    src/frame2d.c \
    src/frame_celestial.c \
    src/step.c \
    src/transform/affine.c \
    src/transform/compose.c \
    src/transform/constant.c \
    src/transform/concatenate.c \
    src/transform/divide.c \
    src/transform/identity.c \
    src/transform/polynomial.c \
    src/transform/spherical_cartesian.c \
    src/transform/property/bounding_box.c \
    src/transform/remap_axes.c \
    src/transform/rotate_sequence_3d.c \
    src/transform/scale.c \
    src/transform/shift.c \
    src/transform/transform.c \
    src/util.c \
    src/wcs.c

src_headers = \
    src/backend.h \
    src/coordinates/frame_attributes.h \
    src/eval.h \
    src/fitswcs_imaging.h \
    src/frame.h \
    src/gwcs.h \
    src/step.h \
    src/transform/affine.h \
    src/transform/compose.h \
    src/transform/concatenate.h \
    src/transform/constant.h \
    src/transform/divide.h \
    src/transform/identity.h \
    src/transform/polynomial.h \
    src/transform/remap_axes.h \
    src/transform/rotate_sequence_3d.h \
    src/transform/scale.h \
    src/transform/shift.h \
    src/transform/spherical_cartesian.h \
    src/transform/transform.h \
    src/types/asdf_gwcs_backend_map.h \
    src/types/asdf_gwcs_coordinate_frame_map.h \
    src/types/asdf_gwcs_transform_map.h \
    src/util.h

AM_CFLAGS = $(ASDF_GWCS_CFLAGS) $(LIBASDF_CFLAGS) -fvisibility=hidden -fmacro-prefix-map=$(top_srcdir)/=
AM_CPPFLAGS = $(STC_CPPFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_LDFLAGS =
ACLOCAL_AMFLAGS = -Im4

noinst_HEADERS = $(src_headers)

lib_LTLIBRARIES = libasdf-gwcs.la
libasdf_gwcs_la_SOURCES = $(src_files)
libasdf_gwcs_la_CFLAGS = $(AM_CFLAGS)
libasdf_gwcs_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(LIBASDF_GWCS_VERSION_INFO)
libasdf_gwcs_la_LIBADD = third_party/libstc.la $(LIBASDF_LIBS)

if HAVE_AST
src_files += src/backends/ast_yaml.c
libasdf_gwcs_la_SOURCES += src/backends/ast_yaml.c
libasdf_gwcs_la_CFLAGS  += $(AST_CFLAGS)
libasdf_gwcs_la_LIBADD  += $(AST_LIBS) $(FYAML_LIBS)

# AST reaches the link through @AST_LIBS@, which automake cannot see into, so
# make is never told the library depends on it.  Building libast.la builds
# AST's other libraries along with it.
EXTRA_libasdf_gwcs_la_DEPENDENCIES = third_party/ast/libast.la

third_party/ast/libast.la:
	cd third_party/ast && $(MAKE) $(AM_MAKEFLAGS) all
endif

if BUILD_DOCS
# LIBASDF_CFLAGS is passed through so that hawkmoth's clang can find libasdf's
# headers; without them the public headers fail to parse and their declarations
# silently vanish from the generated API documentation.
# See libasdf_include_flags() in docs/conf.py
docs:
	$(AM_V_at)LIBASDF_CFLAGS="$(LIBASDF_CFLAGS)" \
	    $(SPHINXBUILD) $(SPHINX_FLAGS) -b html $(srcdir)/docs $(builddir)/docs/_build/html

distcheck-docs:
	$(MAKE) docs SPHINX_FLAGS=-W
endif
.PHONY: docs distcheck-docs

dist_doc_DATA = LICENSE CHANGES.rst

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libasdf-gwcs.pc
CLEANFILES = libasdf-gwcs.pc

format:
	clang-format -i $(addprefix $(srcdir)/,$(src_files)) $(addprefix $(srcdir)/,$(src_headers))

.PHONY: format

# CMake-related files to include in the distribution
EXTRA_DIST = \
    .clang-format \
    CMakeLists.txt \
    README.rst \
    autogen.sh \
    cmake/ASDFGWCSOptions.cmake \
    cmake/AddressAnalyzer.cmake \
    cmake/SubmoduleInit.cmake \
    config.h.cmake \
    scripts/check-soversion.sh \
    src/CMakeLists.txt \
    towncrier.toml

CMAKE_EXTRA_FLAGS=

# Try building and running tests with cmake to ensure the cmake build works
# from the distribution tarball
#
# Also checks that the library CMake produced carries the soname implied by
# LIBASDF_GWCS_VERSION_INFO.  CMake takes PROJECT_SOVERSION and
# PROJECT_LIBVERSION as independent inputs and cross-checks nothing, so it is
# the side that can drift away from configure.ac; libtool derives both names
# from the triple and cannot.  See "Shared library versioning" in
# docs/development.rst.
distcheck-cmake:
	@echo "=== Running CMake build inside distcheck ==="
	rm -rf $(distdir)/_build/cmake
	mkdir $(distdir)/_build/cmake
	cd $(distdir)/_build/cmake && \
	    cmake ../.. \
                -DENABLE_TESTING_ALL=YES \
                -DCMAKE_BUILD_TYPE=Release \
                $(CMAKE_EXTRA_FLAGS) && \
            $(MAKE) && \
            $(MAKE) test
	@echo "=== Checking the CMake build's soname against -version-info ==="
	$(SHELL) $(top_srcdir)/scripts/check-soversion.sh \
	    '$(LIBASDF_GWCS_VERSION_INFO)' '$(distdir)/_build/cmake/src'; \
	    ret=$$?; \
	    if [ $$ret -eq 77 ]; then \
	        true; \
	    elif [ $$ret -ne 0 ]; then \
	        exit $$ret; \
	    fi

.PHONY: distcheck-cmake


# 1. Check building docs from the distribution if possible
# 2. Test the CMake build
distcheck-hook:
	$(MAKE) distcheck-docs
	$(MAKE) distcheck-cmake
