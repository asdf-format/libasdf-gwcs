SUBDIRS = include tests third_party

src_files = \
    src/fitswcs_imaging.c \
    src/frame.c \
    src/frame2d.c \
    src/frame_celestial.c \
    src/step.c \
    src/transform.c \
    src/transforms/property/bounding_box.c \
    src/wcs.c

src_headers = \
    src/fitswcs_imaging.h \
    src/frame.h \
    src/gwcs.h \
    src/step.h \
    src/transform.h \
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
