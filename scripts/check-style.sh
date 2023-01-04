#!/bin/bash
#
# Copyright (c) The Libre Solar Project Contributors
# SPDX-License-Identifier: Apache-2.0

DEFAULT_BRANCH="origin/main"

if [ -x "$(command -v clang-format-diff)" ]; then
CLANG_FORMAT_DIFF="clang-format-diff"
elif [ -x "$(command -v clang-format-diff-12)" ]; then
CLANG_FORMAT_DIFF="clang-format-diff-12"
elif [ -x "$(command -v /usr/share/clang/clang-format-diff.py)" ]; then
CLANG_FORMAT_DIFF="/usr/share/clang/clang-format-diff.py -v"
fi

echo "Style check for diff between branch $DEFAULT_BRANCH"

STYLE_ERROR=0

echo "Checking trailing whitespaces with git diff --check"
git diff --check --color=always $DEFAULT_BRANCH
if [[ $? -ne 0 ]]; then
    STYLE_ERROR=1
else
    echo "No trailing whitespaces found."
fi

echo "Checking coding style with clang-format"

# clang-format-diff returns 0 even for style differences, so we have to check the length of the
# response
CLANG_FORMAT_DIFF=`git diff $DEFAULT_BRANCH | $CLANG_FORMAT_DIFF -p1 | colordiff`
if [[ "$(echo -n $CLANG_FORMAT_DIFF | wc -c)" -ne 0 ]]; then
    echo "${CLANG_FORMAT_DIFF}"
    STYLE_ERROR=1
else
    echo "Coding style valid."
fi

exit $STYLE_ERROR
