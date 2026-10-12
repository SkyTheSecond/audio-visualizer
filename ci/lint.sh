#!/usr/bin/env bash
# Usage: ci/lint.sh [build-dir]
#
# Checks our own code (src/ and tests/, never third_party/) in two steps:
#   1. clang-format: every file must already be formatted per .clang-format.
#   2. clang-tidy: every .cpp must pass .clang-tidy. Needs a configured build directory
#      (default: build) for its compile_commands.json.
# Set CLANG_FORMAT / CLANG_TIDY to pick a specific version, e.g. clang-tidy-19.
set -euo pipefail

build_dir=${1:-build}
clang_format=${CLANG_FORMAT:-clang-format}
clang_tidy=${CLANG_TIDY:-clang-tidy}
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

cd "$(git rev-parse --show-toplevel)"

echo "== clang-format ($("$clang_format" --version))"
if ! git ls-files -z 'src/*.cpp' 'src/*.hpp' 'tests/*.cpp' 'tests/*.hpp' |
    xargs -0 "$clang_format" --dry-run --Werror; then
    echo "Formatting differs. Fix with: git ls-files 'src/*.[ch]pp' 'tests/*.[ch]pp' | xargs $clang_format -i"
    exit 1
fi
echo "OK"

if [[ ! -f "$build_dir/compile_commands.json" ]]; then
    echo "No $build_dir/compile_commands.json: configure first (cmake -B $build_dir)." >&2
    exit 1
fi

echo "== clang-tidy ($("$clang_tidy" --version | grep -o 'version [0-9.]*'))"
# Headers are checked through the .cpp files that include them. The "N warnings
# generated" lines count findings in library headers, which are filtered out anyway.
if ! git ls-files -z 'src/*.cpp' 'tests/*.cpp' |
    xargs -0 -n 1 -P "$jobs" "$clang_tidy" --quiet -p "$build_dir" 2>&1 |
    { grep -v ' warnings\{0,1\} generated\.$' || true; }; then
    echo "clang-tidy found problems (above)."
    exit 1
fi
echo "OK"
