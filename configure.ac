AC_PREREQ([2.69])
AC_INIT(
  [libasdf-gwcs], [0.1.0],
  [https://github.com/asdf-format/libasdf-gwcs/issues],
  [],
  [https://github.com/asdf-format/libasdf-gwcs],
)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_USE_SYSTEM_EXTENSIONS

LT_INIT([disable-static])

AM_SILENT_RULES([yes])

AC_CONFIG_SRCDIR([src/wcs.c])
AC_CONFIG_MACRO_DIR([m4])

AC_LANG([C])
AC_PROG_CC

AS_IF([test "x$ac_cv_prog_cc_c11" = "xno"],
  [AC_MSG_ERROR([C11 support is required but not available])])

AC_PROG_INSTALL

# Check and add compiler flags
AX_CHECK_COMPILE_FLAG([-Wall],          [ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra],        [ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-Wparentheses],  [ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -Wparentheses"])
AX_CHECK_COMPILE_FLAG([-Wpointer-sign], [ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -Wpointer-sign"])
AX_CHECK_COMPILE_FLAG([-Wshadow],       [ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -Wshadow"])

# --enable-debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Enable debug build (-g -O0)])],
  [enable_debug=yes],
  [enable_debug=no]
)

AS_IF([test "x$enable_debug" = "xyes"], [
  ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -g -O0"
  AC_DEFINE([DEBUG], [1], [Define if debugging is enabled])
])

# Dependencies
PKG_CHECK_MODULES([LIBASDF], [libasdf])

# STC (header-only hashmap library) -- git submodule at third_party/STC
AC_MSG_CHECKING([for STC headers])
AS_IF([test -f "$srcdir/third_party/STC/include/stc/hmap.h"],
  [AC_MSG_RESULT([yes (submodule)])
   STC_CPPFLAGS="-I\$(top_srcdir)/third_party/STC/include"],
  [AC_MSG_ERROR([STC submodule not found; run: git submodule update --init third_party/STC])]
)
AC_SUBST([STC_CPPFLAGS])

AC_SUBST([ASDF_GWCS_CFLAGS])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  include/Makefile
  libasdf-gwcs.pc
  tests/Makefile
  third_party/Makefile
])

AC_OUTPUT

printf "\n============= Configuration Summary =============\n"
printf "  %22s %s\n" "libasdf-gwcs version:" "$VERSION"
printf "  %22s %s\n" "compiler:" "$CC"
printf "  %22s %s\n" "libasdf:" "$(pkg-config --modversion libasdf)"
printf "=================================================\n"
