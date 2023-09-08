#!/bin/bash

set -u

FILECHECK="build/tools/test-cli/hermes-engine-prefix/src/hermes-engine-build/bin/FileCheck"
TESTCLI="build/tools/test-cli/test-cli"

for f in ./tests/js/*.js; do
    echo Running $f
    ${TESTCLI} $f | ${FILECHECK} --match-full-lines $f
done
