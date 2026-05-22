#!/bin/sh
set -e

if [ -d .git ]; then
    git submodule update --init third_party/STC third_party/ast

    if [ -f third_party/ast/bootstrap.local ]; then
        (cd third_party/ast && ./bootstrap.local)
    fi
fi

autoreconf --install --force
