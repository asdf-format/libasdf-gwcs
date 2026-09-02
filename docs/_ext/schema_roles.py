"""More concise roles for linking into the ASDF schema documentation

Written out in full, an intersphinx reference to a schema page is::

    :external+asdf-transform-schemas:doc:`affine <generated/schemas/affine-1.5.0>`

which is too wordy at the density these appear in this project's docs.  This
extension registers a short role per schema inventory, so the same link reads::

    :transform-schema:`affine <affine-1.5.0>`

or, letting the display text default to the schema name::

    :transform-schema:`affine-1.5.0`

Configure it with ``schema_roles`` in ``conf.py``, mapping a role name to the
intersphinx inventory it draws on and the path prefix its pages live under::

    schema_roles = {
        'transform-schema': ('asdf-transform-schemas', 'generated/schemas/'),
    }

The reference itself is still resolved by intersphinx, so a target the linked
docs does not publish is reported as a warning exactly as the long form would
be.
"""

import re

from sphinx.ext.intersphinx import IntersphinxRole


__all__ = ['setup']


# Splits an explicit title off a role's text, as in `title <target>`.
_EXPLICIT_TITLE = re.compile(r'^(?P<title>.+?)\s*<(?P<target>.+)>$', re.DOTALL)


def _make_role(inventory, prefix):
    """Build a role that expands to an external ``doc`` reference."""

    xref_role_name = f'external+{inventory}:doc'

    def role(name, rawtext, text, lineno, inliner, options=None, content=None):
        match = _EXPLICIT_TITLE.match(text)

        if match:
            title = match.group('title')
            target = match.group('target').strip()
        else:
            # Default the display text to the schema name, not the full path.
            title = target = text.strip()

        expanded = f'{title} <{prefix}{target}>'
        return IntersphinxRole(xref_role_name)(
            xref_role_name,
            rawtext,
            expanded,
            lineno,
            inliner,
            options or {},
            content or [],
        )

    return role


def _register_roles(app):
    for name, (inventory, prefix) in app.config.schema_roles.items():
        app.add_role(name, _make_role(inventory, prefix))


def setup(app):
    app.add_config_value('schema_roles', {}, 'env')
    app.connect('builder-inited', _register_roles)
    return {'version': '1.0', 'parallel_read_safe': True}
