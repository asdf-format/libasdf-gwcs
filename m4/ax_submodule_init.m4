# AX_SUBMODULE_INIT(PATH, [SENTINEL])
# ------------------------------------
# Ensure the git submodule at PATH (relative to $srcdir) is initialized.
# SENTINEL is a file or directory whose existence indicates the submodule is
# already populated; it defaults to PATH/.git.  If the sentinel is absent and
# $srcdir is a git repository, runs `git submodule update --init PATH`.
# Errors if the sentinel is absent and we are not in a git checkout.
AC_DEFUN([AX_SUBMODULE_INIT], [
    AS_IF([test ! -e "$srcdir/m4_default([$2], [$1/.git])"],
        [AS_IF([test -d "$srcdir/.git"],
            [AC_MSG_NOTICE([Initializing submodule $1...])
             AS_IF([(cd "$srcdir" && git submodule update --init "$1")],
                 [],
                 [AC_MSG_ERROR([Failed to initialize submodule $1])])],
            [AC_MSG_ERROR([Submodule $1 is not initialized and $srcdir is not a git repository])]
        )]
    )
])
