SUBDIRS = include tests third_party

src_files = \
    src/backend.c \
    src/eval.c \
    src/fitswcs_imaging.c \
    src/frame.c \
    src/frame2d.c \
    src/frame_celestial.c \
    src/step.c \
    src/transform.c \
    src/transforms/affine.c \
    src/transforms/compose.c \
    src/transforms/constant.c \
    src/transforms/concatenate.c \
    src/transforms/divide.c \
    src/transforms/identity.c \
    src/transforms/polynomial.c \
    src/transforms/spherical_cartesian.c \
    src/transforms/property/bounding_box.c \
    src/transforms/remap_axes.c \
    src/transforms/rotate_sequence_3d.c \
    src/transforms/scale.c \
    src/transforms/shift.c \
    src/wcs.c

src_headers = \
    src/backend.h \
    src/eval.h \
    src/fitswcs_imaging.h \
    src/frame.h \
    src/gwcs.h \
    src/step.h \
    src/transform.h \
    src/transforms/affine.h \
    src/transforms/compose.h \
    src/transforms/concatenate.h \
    src/transforms/constant.h \
    src/transforms/divide.h \
    src/transforms/identity.h \
    src/transforms/polynomial.h \
    src/transforms/remap_axes.h \
    src/transforms/rotate_sequence_3d.h \
    src/transforms/scale.h \
    src/transforms/shift.h \
    src/transforms/spherical_cartesian.h \
    src/types/asdf_gwcs_backend_map.h \
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

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libasdf-gwcs.pc
CLEANFILES = libasdf-gwcs.pc

format:
	clang-format -i $(addprefix $(srcdir)/,$(src_files)) $(addprefix $(srcdir)/,$(src_headers))

.PHONY: format
