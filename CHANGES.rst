libasdf-gwcs 0.2.0 (2026-10-09)
===============================

Feature
-------

- Added `asdf_gwcs_eval_copy`, which makes an independent copy of an evaluation
  context.

  This is handled in a thread-safe manner with the AST backend, making it
  possible for user code to copy an evaluation context safely between threads.
  This is necessary at least for the AST backend since otherwise the context
  can only be used from the thread which created it. (`#34
  <https://github.com/asdf-format/libasdf-gwcs/issues/34>`_)
- The ``inverse`` property of transforms is now read into
  ``asdf_gwcs_transform_t.inverse`` and written back out on serialization.

  Previously any declared inverse was silently dropped, so a GWCS read and
  re-written (including the copy passed to the AST evaluation backend) lost it.
  (`#39 <https://github.com/asdf-format/libasdf-gwcs/issues/39>`_)
- Added `asdf_gwcs_eval_invert`, which returns a new evaluation context that
  evaluates the inverse of an existing one (e.g. world to pixel coordinates).

  Declared ``inverse`` transforms in the file are used as given.  If the
  backend cannot invert a WCS it returns ``NULL`` with the new error code
  `ASDF_GWCS_ERR_NO_INVERSE`. (`#41
  <https://github.com/asdf-format/libasdf-gwcs/issues/41>`_)
- Added support for the ``transform/rotate3d`` transform, as
  `asdf_gwcs_rotate3d_t`.

  This is the transform astropy writes for ``RotateNative2Celestial``,
  ``RotateCelestial2Native`` and ``EulerAngleRotation``.


Bugfix
------

- Fixed the use of the public asdf/gwcs headers in C++ sources.

  This deletes the accidental ``static const ... ASDF_GWCS_TRANSFORM_INVALID``
  from the public headers--it was never meant to be there (maybe accidental
  paste).  C accepted it anyways, but C++ does not accept the uninitialized
  static const. (`#33
  <https://github.com/asdf-format/libasdf-gwcs/issues/33>`_)
- Fixed a bug in the build system where AST's headers (i.e. ast.h) was used
  preferentially from an installed AST in the target prefix, rather than the
  built copy of the vendored AST, resulting in compiling against
  stale/incorrect versions of ast.h. (`#36
  <https://github.com/asdf-format/libasdf-gwcs/issues/36>`_)


libasdf-gwcs 0.1.0 (2026-09-21)
===============================

No significant changes.


libasdf-gwcs 0.1.0rc1 (2026-09-21)
==================================

Bugfix
------

- Fix accidental installation of AST build artifacts when running ``make
  install``.

  AST is only included as a vendored dependency statically linked into
  libasdf-gwcs: this package's build tools should not install anything from AST
  itself.
- Fixed inconsistency in the SONAME written by CMake versus autotools.

  This is the same as a similar fix made to libasdf in
  https://github.com/asdf-format/libasdf/pull/264 .
- Incorporated fixes to AST's build system needed for building within
  Homebrew's sandboxed build environment.


libasdf-gwcs 0.1.0rc0 (2026-09-18)
==================================

General
-------

- Initial version, extracted from ``libasdf``, where the GWCS extension
  previously lived as an optional feature. It is now a separate extension
  library. (`#2 <https://github.com/asdf-format/libasdf-gwcs/issues/2>`_)


Feature
-------

- Added serialization/deserialization support for the transforms needed by the
  Roman Level 2 WCS, including ``shift``, ``remap_axes``, ``polynomial``,
  ``compose`` and ``concatenate``. (`#5
  <https://github.com/asdf-format/libasdf-gwcs/issues/5>`_)
- Added a pluggable evaluation backend interface (``asdf_gwcs_eval_t`` and
  friends), and integrated Starlink AST as the first built-in backend, so that
  a WCS read from an ASDF file can actually be evaluated. (`#6
  <https://github.com/asdf-format/libasdf-gwcs/issues/6>`_)
- Added serialization/deserialization support for further commonly used
  transforms: ``affine``, ``constant``, ``divide``, ``identity``, ``scale``,
  ``rotate_sequence_3d`` and ``spherical_cartesian``, as well as the
  ``bounding_box`` transform property. (`#8
  <https://github.com/asdf-format/libasdf-gwcs/issues/8>`_)
- Added the ``asdf_gwcs_grid2d_t`` API for conveniently building and evaluating
  2D pixel grids, including ``asdf_gwcs_grid2d_fill`` and
  ``asdf_gwcs_eval_grid2d``. (`#10
  <https://github.com/asdf-format/libasdf-gwcs/issues/10>`_)
- Several API additions and improvements that came out of writing the
  documentation: e.g. added support for the optional ``pixel_shape`` property
  of a WCS, and iteration over sub-transforms of compound transforms (such as
  ``compose`` and ``concatenate``). Support for ``frame_attributes`` on FK4 and
  FK5 celestial frames. (`#21
  <https://github.com/asdf-format/libasdf-gwcs/issues/21>`_)


Documentation
-------------

- Added full Sphinx documentation: a getting-started guide, usage pages with
  worked examples, generated API reference for the public headers. (`#21
  <https://github.com/asdf-format/libasdf-gwcs/issues/21>`_)


Misc
----

- Reorganized the library sources to mirror the layout of the GWCS and
  ``astropy`` transform schema namespaces, so that a given tag's implementation
  is where its schema name suggests it should be. (`#6
  <https://github.com/asdf-format/libasdf-gwcs/issues/6>`_)
- Added a performance benchmark harness under ``utils/perf/``, comparing
  libasdf-gwcs plus AST against Python's ``gwcs`` on the Roman build21
  calibration files, together with the recorded results and plots. (`#11
  <https://github.com/asdf-format/libasdf-gwcs/issues/11>`_)
- Reworked how transform types implement copying and destruction: each
  transform now implements only its own type-specific ``copy`` and ``deinit``
  steps, and the transform registration machinery installs shims that handle
  the base transform fields.  This removes the boilerplate previously required
  of every transform, and makes it easier to add new ones, including from
  third-party plugins. (`#18
  <https://github.com/asdf-format/libasdf-gwcs/issues/18>`_)
- Gave transform serialization and deserialization the same treatment as
  ``copy`` and ``deinit``: individual transforms are now only responsible for
  their own properties, with the base transform properties handled once, by the
  base transform's (de)serialization methods. (`#19
  <https://github.com/asdf-format/libasdf-gwcs/issues/19>`_)
