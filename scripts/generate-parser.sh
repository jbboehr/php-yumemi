#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repository_dir=$(dirname -- "$script_dir")
parser_source="src/parser/parser.y"
parser_c="src/parser/parser.c"
parser_h="src/parser/parser.h"

generate_parser() {
    local output_dir=$1

    mkdir -p "$output_dir/src/parser"

    (
        cd "$output_dir"
        LC_ALL=C bison --no-lines \
            --defines="$parser_h" \
            --output="$parser_c" \
            "$repository_dir/$parser_source"
    )
}

cd "$repository_dir"

if [[ "${1:-}" == "--check" ]]; then
    generation_dir=$(mktemp -d)
    trap 'rm -rf -- "$generation_dir"' EXIT

    generate_parser "$generation_dir"
    cmp "$generation_dir/$parser_c" "$parser_c"
    cmp "$generation_dir/$parser_h" "$parser_h"
    exit 0
fi

generate_parser "$repository_dir"
