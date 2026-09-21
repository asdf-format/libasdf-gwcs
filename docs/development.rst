.. _development-resources:

Development resources
#####################

This page covers building libasdf-gwcs from a git checkout, the conventions the
project follows, and how releases are made.  If you only want to *use* the
library, the build instructions under :ref:`getting-started` *should* be
sufficient.


.. _build-systems:

The two build systems
=====================

libasdf-gwcs ships two parallel build systems, and both are kept working at all
times:

**Autotools** (``configure.ac`` / ``Makefile.am``) is the primary build system
for development, and the one used to produce release tarballs.  ``make dist``
and ``make distcheck`` give a canonical, self-contained source archive that
builds without any of the tooling used to generate it--no autotools, no
Sphinx, no git checkout.  ``make distcheck`` also verifies that the tarball
builds *out of tree*, that everything needed was actually distributed, and that
it uninstalls cleanly, which is what makes it trustworthy as a release
artifact.

**CMake** (``CMakeLists.txt``) exists for consumers, not for producing
releases.  A great deal of C and C++ tooling assumes CMake: it drops into
projects that use ``FetchContent`` or ``find_package``, into meta-build systems
and package managers, and into IDEs that understand CMake projects natively.
Requiring downstream users to deal with autotools in those settings would be a
real obstacle.

The two are not independent: ``make distcheck`` runs the CMake build *from the
distribution tarball* (the ``distcheck-cmake`` target in the top-level
``Makefile.am``), so a change that breaks CMake, or that forgets to distribute
a file CMake needs, fails the autotools release check.  Both are also built and
tested in CI, by the ``Build`` and ``CMake Build`` workflows respectively.

The practical consequences:

* **When you add a source file, add it to both build systems.**  Both list
  their sources explicitly--``src_files`` in the top-level ``Makefile.am`` and
  ``libasdf_gwcs_sources`` in ``src/CMakeLists.txt``.

* **When you add a public header, add it to both install lists**--
  ``include/Makefile.am`` and ``include/CMakeLists.txt``.  Drift between these
  two breaks the *installed* library without breaking the build tree, so it
  tends to be noticed late.

* **When you add a documentation page, add it to EXTRA_DIST in
  docs/Makefile.am**, or it will be missing from the release tarball and
  the documentation build inside ``make distcheck`` will fail.

* **When you change the ABI, update the interface version in both.**  The
  ``LIBASDF_GWCS_VERSION_INFO`` triple in ``configure.ac`` and the
  ``PROJECT_SOVERSION``/``PROJECT_LIBVERSION`` pair in ``CMakeLists.txt``
  describe the same thing by different means and must agree; see
  :ref:`abi-versioning`.


Submodules
==========

The project has three git submodules, so clone with ``--recurse-submodules``
(or run ``git submodule update --init --recursive`` afterwards):

``third_party/ast``
    Starlink AST, providing the ``ast_yaml`` evaluation backend.  This is a
    *fork*; see :ref:`ast-backend` for why, and :ref:`licensing` for the
    consequences of linking it.

``third_party/STC``
    A header-only C11 container library, used for the runtime tag-to-type
    hash maps.

``tests/munit``
    The unit-test framework.

The autotools build bootstraps AST itself: if ``third_party/ast`` has no
``configure`` script it runs AST's ``bootstrap.local``, then configures it as a
sub-package via ``AX_SUBDIRS_CONFIGURE``.  The ``tests/`` makefile likewise
initializes ``tests/munit`` on demand as a prerequisite of ``make check``.

Both build systems configure AST's YAML channel against **libfyaml**, the same
YAML library libasdf itself uses.


Building with autotools
=======================

libasdf-gwcs is a libasdf extension, so ``configure`` has to be able to find
libasdf.  It looks with ``pkg-config``, which needs no help when libasdf is
installed somewhere standard.  When it is installed under a prefix of its
own--as it will be when developing against a side-by-side build--set
``PKG_CONFIG_PATH`` to the directory holding its ``libasdf.pc``, as below.

A git checkout has no ``configure`` script; generate it first with
``autogen.sh`` (a wrapper script around ``autoreconf --install``).  This is
normally only needed once, or again after editing ``configure.ac`` or any
``Makefile.am``, though the generated makefiles normally re-run the necessary
steps by themselves:

.. code:: console

    $ git clone --recurse-submodules https://github.com/asdf-format/libasdf-gwcs.git
    $ cd libasdf-gwcs
    $ ./autogen.sh
    $ ./configure PKG_CONFIG_PATH=/path/to/libasdf/lib/pkgconfig  # if needed
    $ make
    $ make check

Useful ``configure`` options:

``--enable-debug``
    Debug build: ``CFLAGS=-g -O0``, and ``DEBUG`` is defined.  This also
    enables munit's ``--no-fork`` behaviour by default, which matters when
    running tests under a debugger.

``--with-asan``
    Build with AddressSanitizer.

``--without-ast``
    Build without the AST evaluation backend.  Reading and writing GWCS
    objects still works; only evaluation becomes unavailable.  Also removes
    libasdf-gwcs's only LGPL-licensed dependency--see :ref:`licensing`.

``--disable-logging``
    Compile out libasdf-gwcs's internal log statements (they are enabled by
    default).

``--enable-docs``
    Build the Sphinx documentation.  The default is ``auto``: docs are built if
    Sphinx and its extensions are found, and silently skipped otherwise.

Autotools fully supports out-of-tree builds, and keeping several configurations
side by side is the recommended way to work, since a change should pass under
more than one of them:

.. code:: console

    $ mkdir build-asan && cd build-asan
    $ ../configure --with-asan
    $ make && make check

Anything that touches library code should pass ``make check`` in both an
AddressSanitizer build and a debug build before being committed.


Building with CMake
===================

.. code:: console

    $ mkdir build && cd build
    $ cmake .. \
          -D CMAKE_PREFIX_PATH=/path/to/libasdf \
          -D CMAKE_BUILD_TYPE=RelWithDebInfo \
          -D ENABLE_TESTING=YES
    $ make
    $ ctest --output-on-failure

Options of note:

``-D ASDF_GWCS_WITH_AST=[ON/OFF]``
    Build the AST evaluation backend (default ``ON``); the counterpart of
    ``--without-ast``.

``-D ENABLE_ASAN=[YES/NO]``
    Build with AddressSanitizer.

``-D ENABLE_TESTING_DOCS=YES``
    Additionally build and run the example programs embedded in the
    documentation (see `Documentation examples`_).

``-D ENABLE_TESTING_ALL=YES``
    Enable every test target.  This is what CI uses.

``-D ENABLE_DOCS=YES``
    Build the Sphinx documentation (``make docs``).

``make package_source`` and ``make package`` produce CPack archives.  Note that
these are *not* what is published for a release--the release tarball comes
from ``make dist`` under autotools.

.. note::

    On systems with a libasdf or libasdf-gwcs already installed under, say,
    ``~/.local/lib``, be aware that CMake links test binaries with
    ``DT_RUNPATH`` rather than ``DT_RPATH``, and the dynamic loader searches
    ``LD_LIBRARY_PATH`` *before* ``DT_RUNPATH``.  An installed copy can
    therefore shadow the freshly built one and cause confusing failures. Unset
    ``LD_LIBRARY_PATH`` before running ``ctest`` if you hit this.



Running the tests
=================

The test suite uses the `µnit <https://nemequ.github.io/munit/>`__ framework,
with some custom wrappers around it (helper macros) defined in
``tests/munit.h``.

Test binaries are named ``test-<name>.unit`` and live in the ``tests/``
directory of the build tree.

From a build directory:

.. code:: console

    $ make check                      # everything
    $ tests/test-gwcs.unit            # one binary directly
    $ tests/test-gwcs.unit --help     # munit options

munit accepts a test path to run a single case, and other useful flags:

.. code:: console

    $ tests/test-gwcs.unit /gwcs/test_asdf_get_gwcs
    $ tests/test-gwcs.unit --seed 0x1234  # reproduce a specific ordering
    $ tests/test-gwcs.unit --no-fork      # keep the debugger attached

In particular, the ``--no-fork`` option is always enabled by default for debug
builds, as it's extremely helpful if you want to run the tests under gdb (or
your debugger of choice).

``test-gwcs-eval.unit`` includes a conformance test that evaluates every Roman
build21 fixture over a grid and compares the results against reference values
produced by AST and by Python's GWCS, asserting sub-milliarcsecond agreement.
It reports itself as *skipped* rather than failing when the ``ast_yaml``
backend is unavailable, so a ``--without-ast`` build still runs cleanly.


.. _documentation examples:

Documentation examples
======================

The example programs in ``README.rst`` and under ``docs/usage/`` are compiled
and executed as part of the test suite, so they cannot drift away from the API.

A code block opts in with the ``:test:`` option, naming the test, and may
declare an input file with ``:fixture:``:

.. code:: rst

    .. code:: c
       :test: test-gwcs-inspect
       :fixture: roman_l2_wcs.asdf

       #include <asdf/gwcs/gwcs.h>
       ...

``tests/scripts/extract_doc_examples.py`` pulls each marked block out into a
``.c`` file under ``tests/doc_examples/``, compiles it against the freshly
built library, and runs it with the resolved fixture path as its first
argument.  A block with no ``:fixture:`` is run with no arguments; a fixture of
``temp`` or ``temp:<name>`` resolves to a throwaway output path instead of an
input file.

The set of files scanned is listed explicitly, as ``DOC_EXAMPLE_FILES`` in
``tests/Makefile.am`` and ``DOC_FILES`` in ``tests/CMakeLists.txt``; **a new
documentation page containing examples must be added to both.**

The examples are run for their exit status, not compared against the output
quoted in the documentation.  When you change one, re-run it and paste its real
output back into the surrounding prose:

.. code:: console

    $ make check
    $ ./tests/doc_examples/test-gwcs-inspect tests/fixtures/roman_l2_wcs.asdf

Under autotools this needs Python 3, and is skipped if none is found; under
CMake it is gated on ``-D ENABLE_TESTING_DOCS=YES``.


Code style
==========

Formatting is enforced by ``clang-format``; the rules live in
``.clang-format``.  Run:

.. code:: console

    $ make format

from a build directory before committing.  It rewrites all library sources and
headers in place.

libasdf has a ``pre-commit`` hook that applies this automatically on commit;
this project does not ship one yet, so ``make format`` has to be run by hand.

.. note::

    Formatting can differ between major ``clang-format`` versions, so a tree
    formatted with a different one than the rest of the project will show
    spurious diffs.


Documentation
=============

The documentation is built with Sphinx.  API reference pages are generated from
the doc comments in the public headers under ``include/asdf/gwcs`` using
`Hawkmoth <https://hawkmoth.readthedocs.io/>`__, which extracts ``/** ... */``
comments and feeds them to Sphinx as reStructuredText.  Because of that, doc
comments in public headers are written in reST, using field lists
(``:param foo:``, ``:return:``) rather than a Doxygen-style syntax.

Build them with:

.. code:: console

    $ ./configure --enable-docs
    $ make docs

The rendered output lands in ``docs/_build/html`` under the build directory.
CI builds the docs with ``-W``, so warnings are errors; if you add a page, make
sure it is referenced from a ``toctree`` and that every cross-reference
resolves.

Two things about hawkmoth are worth knowing before writing header comments:

* **An undocumented declaration is dropped entirely, and takes its members with
  it.**  A struct whose fields all carry ``/** ... */`` comments will still be
  absent from the rendered API unless the struct *itself* has one.  If a type
  you expect is missing from the output, this is almost always why.

* **libasdf's headers must be on hawkmoth's include path.**  The public headers
  include ``<asdf/util.h>`` for ``ASDF_EXPORT``; without it clang cannot parse
  the declarations and they vanish silently.  ``docs/conf.py`` takes the path
  from ``LIBASDF_CFLAGS`` when the build sets it, falling back to pkg-config.

Because ``conf.py`` sets ``nitpicky = True`` and uses ``c:expr`` as the default
role, *any* bare identifier written in single backticks is looked up in the C
domain.  Standard C names have no inventory to resolve against and are listed
in ``nitpick_ignore``; for anything else that is not a real API symbol--a file
name, a schema name, a field mentioned in passing--use

.. code:: rst

    ``double backticks``

instead.


Changelog entries
=================

The changelog is assembled by `towncrier
<https://towncrier.readthedocs.io/>`__ from individual *news fragments* in the
``changes/`` directory.  Each fragment is a small reStructuredText file named
for the issue or pull request it relates to, with the category as its
extension::

    changes/123.feature
    changes/456.bugfix

The available categories are ``general``, ``feature``, ``bugfix``, ``doc``,
``removal`` and ``misc``.  For a change with no associated issue number, use a
descriptive name prefixed with ``+``, for example
``changes/+cmake-build.misc``.

Write the entry for the reader of the release notes, not for the reviewer of
the diff.


.. _abi-versioning:

Shared library versioning
=========================

The ABI version is not the same as the version of the *package*, and the two
move on different schedules.  A release that only fixes bugs changes the
package version while leaving the binary interface untouched; a change that
adds one public function changes the interface without being a notable
release.  Conflating them is how a project ends up with an SONAME that churns
on every release, breaking installed binaries needlessly.

Three names, one library
------------------------

A shared library is installed on GNU/Linux under three names::

    libasdf-gwcs.so.0.0.0   the real file
    libasdf-gwcs.so.0       the soname: a symlink, and the name recorded in
                            the ELF
    libasdf-gwcs.so         the linker name: a symlink used only at link time

At link time ``-lasdf-gwcs`` finds ``libasdf-gwcs.so``, follows it to the real
file, and records *that file's* ``DT_SONAME``--``libasdf-gwcs.so.0``--in the
program being linked.  At run time the dynamic loader looks for exactly that
name; it never sees ``libasdf-gwcs.so``, and never looks at the trailing
``.0.0``.

The soname is therefore the only compatibility identifier that matters.  Two
libraries sharing a soname are asserting that either can satisfy the other's
consumers.  This is also why distributions split the packages: e.g. on
Debian-based systems the runtime package ships the real file and the soname
symlink, the ``-dev`` package ships ``libasdf-gwcs.so`` and the headers.

The interface version
---------------------

Both build systems derive those names from a single ``current:revision:age``
triple, libtool's ``-version-info``.  It is not a version number; it describes
the set of interfaces the library implements:

``current``
    A monotonically increasing number of the newest interface.

``age``
    How many older consecutive interfaces are still supported.  The library
    implements interfaces ``current - age`` through ``current``.

``revision``
    The implementation serial within ``current``: changes that alter no
    interface at all.

From which::

    soname   = libasdf-gwcs.so.(current - age)
    filename = libasdf-gwcs.so.(current - age).(age).(revision)

``current - age`` is the oldest interface still supported, which is what makes
it the right SONAME.  Adding new symbols raises ``current`` and ``age``
together and leaves the difference unchanged, so existing binaries keep
resolving; removing or changing one resets ``age`` and moves the difference,
so those binaries correctly fail to find their library rather than silently
binding to an incompatible one.

Updating it
-----------

Apply these in order, considering everything that has changed since the last
release:

#. Any source change at all: ``revision++``.
#. Any interface added, removed or changed: ``current++``, ``revision = 0``.
#. Interfaces added, and none removed or changed: ``age++``.
#. Any interface removed or changed: ``age = 0``.

"Interface" means anything a *compiled* consumer can observe: an exported
function appearing, disappearing, or changing signature; a public struct
changing size, alignment or field order; an enum constant changing value; a
public typedef whose underlying type changes width on any supported platform.
Adding a macro or a ``static inline`` to a public header is a source change
but not an interface change, since nothing new is exported.

**Make this part of the change that causes it, not a step at release time.**
The author of a patch that adds a public function knows it is an addition;
whoever cuts the release two months later has to reconstruct that from the
changelog.  Only the first such change in a release cycle needs to move
``current``--once it has moved, further *additions* in the same cycle are
already covered by it, though a later removal still forces ``age = 0``.

Where it lives
--------------

``configure.ac`` holds the triple itself, after ``AC_INIT``, together
with the rules above and a history of its past values::

    LIBASDF_GWCS_VERSION_INFO=0:0:0

CMake cannot consume that directly, and takes the two derived names as
independent inputs rather than deriving them::

    set(PROJECT_SOVERSION 0)        # current - age
    set(PROJECT_LIBVERSION 0.0.0)   # (current - age).(age).(revision)

**These must be kept in step.**  libtool computes both names from one input
and so cannot contradict itself, but CMake cross-checks nothing and will
happily build a library whose real name and SONAME disagree.  That is not
hypothetical: before this was written down, the autotools build produced
SONAME ``libasdf-gwcs.so.0`` while the CMake build produced the incorrect
``libasdf-gwcs.so.0.0.0`` from the same tree.  ``make distcheck`` now checks
this, via ``scripts/check-soversion.sh``.

Note that ``PROJECT_LIBVERSION`` will usually *not* match the package version.
A release with only bug fixes leaves the interface alone, so v0.3.0 might well
ship, for example, ``libasdf-gwcs.so.0.1.1``.

None of these values are managed by bumpver, deliberately, and they must not
be: deriving them from the package version would move the soname on every
release, breaking installed binaries for changes that altered no interface.

.. _making-a-release:

Making a release
================

Despite not being a Python package, version numbers follow :pep:`440`
(because the author likes it).

Tags are the bare version string, with no ``v`` prefix--``0.1.0a2``, not
``v0.1.0a2``.

.. note::

   CMake does *not* support :pep:`440`-style version tags ("a1", "rc0", etc.),
   so the full version is written in ``CMakeLists.txt`` as a
   ``PACKAGE_VERSION`` variable (mirroring autoconf); the CMake standard
   variable ``PROJECT_VERSION`` only contains the ``MAJOR.MINOR.PATCH``
   portion of the version.

Signing the tag
---------------

Release tags should be signed with your GPG key.  ``git tag --annotate``
honours git's ``tag.gpgSign`` setting, so configure it once and release tags
are signed automatically:

.. code:: console

    $ git config --local tag.gpgSign true

This should be configured before making a release; you can find more
information about generating a GPG key and registering it with GitHub
at `Telling Git about your signing key`_.

Cutting the release
-------------------

.. This isn't accurate yet, as the release workflow hasn't been integrated
   yet here.  But it will be possible to copy, more-or-less, straight from
   libasdf so leave here for now.

#. Make sure ``main`` is up to date, the working tree is clean, and CI is
   passing.

#. Check that the interface version reflects this cycle.  If any public
   interface was added, removed or changed since the last release,
   ``LIBASDF_GWCS_VERSION_INFO`` and the matching CMake variables must already
   account for it (see :ref:`abi-versioning`).  This is normally done by the
   change that caused it, so this is a last check rather than the place to do
   the work.

#. Check that every merged change has a news fragment in ``changes/``, and
   preview the assembled changelog:

   .. code:: console

       $ towncrier build --draft --version <new version>

#. Update the version, build the changelog, then commit, tag and push.

#. Pushing the tag triggers the ``Build`` workflow; once it succeeds on that
   tag the ``Release`` workflow creates a **draft** GitHub release, with the
   release notes converted from the new ``CHANGES.rst`` section and the
   ``make dist`` tarball attached.

#. Review the draft release on GitHub and publish it.

If the ``Release`` workflow needs to be re-run against a tag that has already
built successfully, for instance after fixing something in the workflow
itself, it can be triggered by hand, e.g. with the GitHub CLI:

.. code:: console

    $ gh workflow run release.yml -f tag=<version>

Re-running updates the existing draft rather than failing.
