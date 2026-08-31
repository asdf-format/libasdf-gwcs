import os
import re
import subprocess
from datetime import datetime
from pathlib import Path

from docutils.parsers.rst import directives
from sphinx.directives.patches import Code


# -- Project information ------------------------------------------------------
def read_config_h() -> tuple[str, str, str]:
    """Read package data out of config.h if possible"""
    project = 'libasdf-gwcs'
    release = '0.1.0'

    config_h_path = Path(__file__).parent.parent / 'config.h'

    if not config_h_path.is_file():
        version = '.'.join(release.split('.')[:2])
        return project, version, release

    content = config_h_path.read_text()
    if (m := re.search(r'#define\s+PACKAGE_NAME\s+"([^"]+)"', content)):
        project = m.group(1)

    if (m := re.search(r'#define\s+PACKAGE_VERSION\s+"([^"]+)"', content)):
        release = m.group(1)

    version = '.'.join(release.split('.')[:2])
    return project, version, release


project, version, release = read_config_h()
author = 'The ASDF Developers'
copyright = f"{datetime.now().year}, {author}"

# It is a C library--use the 'c' domain by default
primary_domain = 'c'
default_role = 'c:expr'

# -- Options for HTML output ---------------------------------------------------
html_title = f"{project} v{release}"

# Output file base name for HTML help builder.
htmlhelp_basename = project + "doc"

# -- Options for LaTeX output --------------------------------------------------
latex_documents = [(
    "index",
    project + ".tex",
    project + " Documentation", author, "manual"
)]

# -- Options for manual page output --------------------------------------------
# Each tuple is (source doc, name, description, authors, manual section)
man_pages = []

todo_include_todos = True


# Epilogue appended to each rst file; use this to append commonly used link
# references
rst_epilog = ''

with open('links.rst') as fobj:
    rst_epilog += fobj.read()

exclude_patterns = [
    'links.rst'
]


# Enable nitpicky mode - which ensures that all references in the docs
# resolve.

nitpicky = True

# Nitpicks to ignore
# Because we use c:expr as the default role which is *very* convenient, any
# standard C identifiers used within backticks will try to resolve as well.
# I haven't found any Sphinx documents that cover the C standard library
# (someone should write one!) so we list most of those here when they come up
# in the docs.  Try to keep this sorted...
nitpick_ignore = [
    # Standard C library / language identifiers.  The default role is c:expr,
    # so any bare identifier in backticks is looked up in the C domain; there
    # is no Sphinx inventory for the C standard library to resolve these
    # against.  Keep sorted.
    ('c:identifier', 'NULL'),
    ('c:identifier', 'int'),
    ('c:identifier', 'size_t'),
    ('c:identifier', 'uint32_t'),
    ('c:identifier', 'uint64_t'),
]

# Add intersphinx mappings
# e.g. intersphinx_mapping["semantic_version"] = ("https://python-semanticversion.readthedocs.io/en/latest/", None)
intersphinx_mapping = {
    'asdf': ('https://www.asdf-format.org/projects/asdf/en/stable', None),
    'asdf-standard': ('https://www.asdf-format.org/projects/asdf-standard/en/latest/', None),
    # ASDF schema docs
    # Each expose a std:doc target per schema, e.g.
    # :external+asdf-transform-schemas:doc:`generated/schemas/affine-1.5.0`.
    'asdf-transform-schemas': (
        'https://www.asdf-format.org/projects/asdf-transform-schemas/en/latest/', None),
    'asdf-coordinates-schemas': (
        'https://www.asdf-format.org/projects/asdf-coordinates-schemas/en/latest/', None),
    'asdf-wcs-schemas': (
        'https://www.asdf-format.org/projects/asdf-wcs-schemas/en/latest/', None),
    'gwcs': ('https://gwcs.readthedocs.io/en/latest/', None),
    # TODO: Change to www.asdf-format.org URL once the libasdf docs are hosted under it
    'libasdf': ('https://libasdf.readthedocs.io/en/latest/', None),
    'numpy': ('https://numpy.org/doc/stable/', None)
}

extensions = ['sphinx.ext.intersphinx', 'sphinx.ext.todo', 'hawkmoth']

# -- Options for hawkmoth extension --------------------------------------------

hawkmoth_root = Path(__file__).parent.parent

# These are options that should be passed to the compiler when hawkmoth
# processes files.
#
# libasdf's headers must be on the include path too: the public headers include
# <asdf/util.h> for ASDF_EXPORT and friends, and if that cannot be found clang
# fails to parse the declarations, which silently drops them from the generated
# documentation.  Ask pkg-config where libasdf lives, honouring PKG_CONFIG_PATH
# so that an uninstalled build is found the same way the C build finds it.
def libasdf_include_flags() -> list[str]:
    # The build systems already know where libasdf is; they pass it through so
    # that an uninstalled libasdf is found without pkg-config configuration.
    if (flags := os.environ.get('LIBASDF_CFLAGS')):
        return [f for f in flags.split() if f.startswith('-I')]

    try:
        out = subprocess.run(
            ['pkg-config', '--cflags-only-I', 'libasdf'],
            capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return []

    return out.split()


hawkmoth_clang = [f'-I{hawkmoth_root}/include'] + libasdf_include_flags()


# -- Options for theme and HTML output -----------------------------------------
html_theme = "furo"
html_static_path = ["_static"]
# Override default settings from sphinx_asdf / sphinx_astropy (incompatible with furo)
html_sidebars = {}
# The name of an image file (within the static path) to use as favicon of the
# docs.  This file should be a Windows icon file (.ico) being 16x16 or 32x32
# pixels large.
html_favicon = "_static/images/favicon.ico"
html_logo = ""

globalnavlinks = {
    "ASDF Projects": "https://www.asdf-format.org",
    "Tutorials": "https://www.asdf-format.org/en/latest/tutorials/index.html",
    "Community": "https://www.asdf-format.org/en/latest/community/index.html",
}

topbanner = ""
for text, link in globalnavlinks.items():
    topbanner += f"<a href={link}>{text}</a>"

html_theme_options = {
    "light_logo": "images/logo-light-mode.png",
    "dark_logo": "images/logo-dark-mode.png",
    "announcement": topbanner,
}

pygments_style = "monokai"
# NB Dark style pygments is furo-specific at this time
pygments_dark_style = "monokai"

# -- Options for LaTeX output --------------------------------------------------

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, documentclass [howto/manual]).
latex_documents = [("index", project + ".tex", project + " Documentation", author, "manual")]

latex_logo = "_static/images/logo-light-mode.png"


# -- Index grouping ------------------------------------------------------------
# Nearly every symbol in this library is prefixed ``asdf_`` or ``ASDF_``, so the
# generated index piles all of them under a single "A" heading and is close to
# useless for finding anything.
#
# Sphinx builds each index heading from the *category key* of an index entry--
# the fifth element of the ``(type, value, target_id, main, key)`` tuples the
# domains emit--falling back to the entry's first letter only when that key is
# ``None`` (see ``sphinx.environment.adapters.indexentries._group_by_func``).
# Setting it lets us file each symbol under the first letter that actually
# distinguishes it, so ``asdf_gwcs_transform_tag`` lands under "T".
#
# The entry text is left alone, so searching still works on the full name.

# index_strip_prefixes is a new config setting for this purpose
# it is used by _regroup_index_entries run as an env-check-consistency hook.
index_strip_prefixes = ['asdf_gwcs_', 'ASDF_GWCS_', 'asdf_', 'ASDF_']


def _index_group_key(entry_text: str, prefixes: list[str]) -> str | None:
    """Letter to file an index entry under, or None to leave Sphinx's default"""
    # Index text looks like "asdf_gwcs_transform_tag (C function)", and for a
    # struct member "asdf_gwcs_t.name (C member)".  Group members with their
    # parent so a struct and its fields never land under different letters.
    name = entry_text.split(' ', 1)[0].split('.', 1)[0].strip()

    # Longest first, so 'asdf_gwcs_' wins over 'asdf_'.  A prefix that would
    # leave a single character behind is skipped in favour of a shorter one:
    # 'asdf_gwcs_t' is far easier to find under G than under T.
    for prefix in sorted(prefixes, key=len, reverse=True):
        if not name.startswith(prefix):
            continue

        rest = name[len(prefix):].lstrip('_')

        if len(rest) > 1 and rest[:1].isalpha():
            return rest[0].upper()

    return None


def _regroup_index_entries(app, env):
    prefixes = app.config.index_strip_prefixes

    if not prefixes:
        return

    domain = env.domains['index']

    for docname, entries in domain.entries.items():
        regrouped = []

        for entry_type, value, target_id, main, category_key in entries:
            if entry_type == 'single' and category_key is None:
                category_key = _index_group_key(value, prefixes)

            regrouped.append((entry_type, value, target_id, main, category_key))

        domain.entries[docname] = regrouped


# -- Doc-example test directive options ----------------------------------------
# The tests/scripts/extract_doc_examples.py script extracts ``.. code:: c``
# blocks from the documentation and compiles/runs them as part of the test
# suite.  A block is marked for extraction with the ``:test:`` option (whose
# value is the test name) and may declare an input file with ``:fixture:``.
#
# These options are meaningful only to the extraction script; here we simply
# extend the ``code`` directive to accept (and otherwise ignore) them so that
# the documentation still builds without "unknown option" errors.
#
# TODO: Make this more extensible; maybe spin out to a separate plugin
# Sphinx could use an extension for compilable source code doctests like this...
class TestableCode(Code):
    option_spec = dict(Code.option_spec)
    option_spec["test"] = directives.unchanged
    option_spec["fixture"] = directives.unchanged


def setup(app):
    app.add_css_file("css/globalnav.css")
    app.add_directive("code", TestableCode, override=True)
    app.add_config_value("index_strip_prefixes", [], "env")
    app.connect("env-check-consistency", _regroup_index_entries)
