libasdf-gwcs
############

.. image:: https://github.com/asdf-format/libasdf-gwcs/workflows/Build/badge.svg
    :target: https://github.com/asdf-format/libasdf-gwcs/actions
    :alt: CI Status

A `libasdf <https://github.com/asdf-format/libasdf>`__ extension library implementing
`GWCS <https://gwcs.readthedocs.io/>`__ (Generalized World Coordinate System) support
for reading ASDF files containing WCS transforms and coordinate frames.


Getting Started
---------------

libasdf-gwcs is a plugin library for libasdf. Install libasdf first, then build
libasdf-gwcs pointing to it via ``PKG_CONFIG_PATH``.

**Autotools**::

    ./autogen.sh
    ./configure PKG_CONFIG_PATH=/path/to/libasdf/lib/pkgconfig
    make
    sudo make install

**CMake**::

    mkdir build && cd build
    cmake .. -DCMAKE_PREFIX_PATH=/path/to/libasdf -DENABLE_TESTING=YES
    make
    sudo make install


Development
===========

Building from git
-----------------

Requirements
^^^^^^^^^^^^

- `libasdf <https://github.com/asdf-format/libasdf>`__
- **CMake** or autotools (autoconf/automake/libtool)
- **pkg-config**
- **libstatgrab** (optional, for test utilities)

On **Debian/Ubuntu**::

    sudo apt install build-essential pkg-config libstatgrab-dev

On **macOS** (with Homebrew)::

    brew install pkg-config libstatgrab

Notes
^^^^^

- Run ``make check`` (autotools) or ``ctest`` (CMake) to run tests.
