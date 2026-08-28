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

ASDF_GWCS_BUILD_MODE=

# --enable-debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Enable debug build (-g -O0)])],
  [enable_debug=yes],
  [enable_debug=no]
)

AS_IF([test "x$enable_debug" = "xyes"], [
  ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -g -O0"
  AC_DEFINE([DEBUG], [1], [Define if debugging is enabled])
  ASDF_GWCS_BUILD_MODE="${ASDF_GWCS_BUILD_MODE}debug "
])

AM_CONDITIONAL([DEBUG], [test "x$enable_debug" = "xyes"])

# --with-asan
AC_ARG_WITH([asan],
  [AS_HELP_STRING([--with-asan], [Build with AddressSanitizer support])],
  [with_asan=yes],
  [with_asan=no])

AS_IF([test "x$with_asan" = "xyes"], [
  ASDF_GWCS_CFLAGS="$ASDF_GWCS_CFLAGS -O1 -g -fsanitize=address -fno-omit-frame-pointer"
  ASDF_GWCS_LDFLAGS="$ASDF_GWCS_LDFLAGS -fsanitize=address"
  AC_DEFINE([WITH_ASAN], [1], [Defined if building with AddressSanitizer])
  ASDF_GWCS_BUILD_MODE="${ASDF_GWCS_BUILD_MODE}asan "
])


# Logging flags
AC_ARG_ENABLE([logging],
  [AS_HELP_STRING([--enable-logging], [Enable logging (default: yes)])],
  [enable_logging=$enableval],
  [enable_logging=yes]
)

AS_IF([test "x$enable_logging" = "xyes"],
  [AC_DEFINE([ASDF_LOG_ENABLED], [1], [Enable logging])]
)

# Submodule initialization (required dependencies)
# Use a distributed file as the sentinel rather than the default PATH/.git:
# a release tarball has no .git anywhere, so keying on it would make the
# tarball impossible to configure.
AX_SUBMODULE_INIT([third_party/STC], [third_party/STC/include/stc/common.h])
AX_SUBMODULE_INIT([tests/munit], [tests/munit/munit.h])

# --enable-ast (default: yes)
# Uses vendored submodule at third_party/ast; falls back to system libast.
AC_ARG_ENABLE([ast],
  [AS_HELP_STRING([--enable-ast],
    [Enable AST evaluation backend (default: yes)])],
  [enable_ast=$enableval],
  [enable_ast=yes]
)

dnl Dependencies 1
dnl Detect libfyaml early (before AST) so we can pass the results of FYAML_CFLAGS
dnl and FYAML_LIBS through to AST's configure
PKG_CHECK_MODULES([FYAML], [libfyaml], [], [
  AC_MSG_ERROR([libfyaml is required but was not found])
])

dnl Workaround for https://github.com/pantoniou/libfyaml/pull/275
dnl This can be deleted if libfyaml is fixed upstream, though system
dnl packagers are likely to fix this too
FYAML_LIBS=$(printf '%s\n' "$FYAML_LIBS" | sed 's/none required//g')


dnl --- AST configure --------------------------------------------------------
HAVE_AST=no
AST_CFLAGS=
AST_LIBS=
ASDF_GWCS_BACKENDS=

AS_IF([test "x$enable_ast" = "xyes"], [
  dnl Try vendored submodule first; fall back to system libast
  have_ast_submodule=no
  AS_IF([test -e "$srcdir/third_party/ast/.git"],
    [have_ast_submodule=yes],
    [AS_IF([test -d "$srcdir/.git"],
      [AC_MSG_NOTICE([Initializing third_party/ast submodule...])
       AS_IF([(cd "$srcdir" && git submodule update --init third_party/ast)],
         [have_ast_submodule=yes],
         [AC_MSG_WARN([Failed to initialize third_party/ast submodule])])])])

  AS_IF([test "x$have_ast_submodule" = "xyes"], [
    dnl Bootstrap AST's autotools if configure hasn't been generated yet
    AS_IF([test ! -f "$srcdir/third_party/ast/configure"],
      [AC_MSG_NOTICE([Bootstrapping AST submodule...])
       AS_IF([(cd "$srcdir/third_party/ast" && ./bootstrap.local)],
         [],
         [AC_MSG_ERROR([Failed to bootstrap AST submodule])])])
    dnl Build vendored AST as a PIC static library so it can be embedded in
    dnl our shared library.
    dnl
    dnl AST's configure uses AC_CHECK_LIB to find libyaml, which only searches
    dnl standard system paths.  On macOS (Homebrew) or other non-standard
    dnl installs libyaml may live outside those paths.  Pass it
    dnl CPPFLAGS/LDFLAGS with our pkg-config findings so AST's configure sees
    dnl it.
    AX_SUBDIRS_CONFIGURE(
      [[third_party/ast]],
      [[--disable-shared], [--enable-static], [--with-pic], [--with-simd], [--with-yaml=fyaml]],
      [[FYAML_CFLAGS=$FYAML_CFLAGS], [FYAML_LIBS=$FYAML_LIBS]],
      [], [])
    dnl ast.h is generated in the top of the AST build directory
    AST_CFLAGS="-I\$(top_builddir)/third_party/ast"
    dnl Reference .la files directly so libtool absorbs them into
    dnl libasdf-gwcs.so.
    dnl Libtool only absorbs static archives listed directly in LIBADD; archives
    dnl referenced only via dependency_libs of another .la are treated as -l
    dnl flags and are NOT statically linked in. Thus all AST sub-libraries
    dnl must be listed explicitly here.
    _ast_dir="\$(top_builddir)/third_party/ast"
    AST_LIBS="$_ast_dir/libast.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_pal.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_cminpack.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_grf_2.0.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_grf_3.2.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_grf_5.6.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_grf3d.la"
    AST_LIBS="$AST_LIBS $_ast_dir/libast_err.la"
    HAVE_AST=yes
  ], [
    dnl Vendored not available; try system libast
    AC_CHECK_LIB([ast], [astBegin],
      [AC_CHECK_HEADER([ast.h],
        [dnl libyaml is still required for YamlChan
         PKG_CHECK_MODULES([YAML], [yaml-0.1], [],
           [AC_MSG_ERROR([libyaml (yaml-0.1) is required for the AST backend but was not found])])
         AST_LIBS="-last"
         HAVE_AST=yes],
        [AC_MSG_WARN([ast.h not found; AST backend disabled])]
      )],
      [AC_MSG_WARN([libast not found and vendored submodule unavailable; AST backend disabled])]
    )
  ])
])

AS_IF([test "x$HAVE_AST" = "xyes"],
  [AC_DEFINE([HAVE_AST], [1], [Define if AST evaluation backend is available])
   ASDF_GWCS_BACKENDS="${ASDF_GWCS_BACKENDS}ast_yaml "])

AM_CONDITIONAL([HAVE_AST], [test "x$HAVE_AST" = "xyes"])
AC_SUBST([AST_CFLAGS])
AC_SUBST([AST_LIBS])

dnl --- End AST configure -----------------------------------------------------

# Dependencies 2
PKG_CHECK_MODULES([LIBASDF], [libasdf])
dnl Workaround for https://github.com/pantoniou/libfyaml/pull/275 leaking
dnl through libasdf's Requires: libfyaml into LIBASDF_LIBS
LIBASDF_LIBS=$(printf '%s\n' "$LIBASDF_LIBS" | sed 's/none required//g')

# STC (header-only hashmap library) -- git submodule at third_party/STC
STC_CPPFLAGS="-I\$(top_srcdir)/third_party/STC/include"
AC_SUBST([STC_CPPFLAGS])

# --- Optional features -------------------------------------------------------

# Enable/check doc build requirements
ASDF_CHECK_SPHINX

# Python is used to extract the testable examples from the documentation
AC_PATH_PROG([PYTHON3], [python3], [no])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON3" != "no"])
AC_SUBST([PYTHON3])

AC_SUBST([ASDF_GWCS_CFLAGS])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  docs/Makefile
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
printf "  %22s %s\n" "libfyaml:" "$(pkg-config --modversion libfyaml)"
AS_IF([test "x$ASDF_GWCS_BACKENDS" != "x"],
  [printf "  %22s %s\n" "backends:" "$ASDF_GWCS_BACKENDS"],
  [printf "  %22s %s\n" "backends:" "(none)"])
AS_IF([test "x$ASDF_GWCS_BUILD_MODE" != "x"], [
  printf "  %22s %s\n" "build:" "$ASDF_GWCS_BUILD_MODE"
])
printf "=================================================\n"
