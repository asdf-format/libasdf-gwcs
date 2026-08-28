SUBDIRS = include tests third_party

src_files = \
    src/backend.c \
    src/coordinates/baseframe.c \
    src/coordinates/fk4.c \
    src/coordinates/fk5.c \
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
libasdf_gwcs_la_LIBADD = third_party/libstc.la $(LIBASDF_LIBS)

if HAVE_AST
src_files += src/backends/ast_yaml.c
libasdf_gwcs_la_SOURCES += src/backends/ast_yaml.c
libasdf_gwcs_la_CFLAGS  += $(AST_CFLAGS)
libasdf_gwcs_la_LIBADD  += $(AST_LIBS) $(FYAML_LIBS)
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libasdf-gwcs.pc
CLEANFILES = libasdf-gwcs.pc

format:
	clang-format -i $(addprefix $(srcdir)/,$(src_files)) $(addprefix $(srcdir)/,$(src_headers))

.PHONY: format

# CMake-related files to include in the distribution
EXTRA_DIST = \
    README.rst \
    CHANGES.rst \
    LICENSE \
    autogen.sh \
    towncrier.toml \
    .clang-format \
    CMakeLists.txt \
    config.h.cmake \
    cmake/AddressAnalyzer.cmake \
    cmake/ASDFGWCSOptions.cmake \
    cmake/SubmoduleInit.cmake \
    src/CMakeLists.txt

CMAKE_EXTRA_FLAGS=

# Try building and running tests with cmake to ensure the cmake build works
# from the distribution tarball
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

.PHONY: distcheck-cmake


# 1. Test the CMake build
distcheck-hook:
	$(MAKE) distcheck-cmake
