#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repository_dir=$(dirname -- "$script_dir")
parser_source="src/parser/parser.y"
parser_c="src/parser/parser.c"
parser_h="src/parser/parser.h"

normalize_generated_source() {
    php -r '
        $path = $argv[1];
        $source = file_get_contents($path);
        if ($source === false) {
            fwrite(STDERR, "Unable to read {$path}.\n");
            exit(1);
        }
        $source = preg_replace("/[ \\t]+$/m", "", $source);
        if ($source === null || file_put_contents($path, rtrim($source, "\r\n") . "\n") === false) {
            fwrite(STDERR, "Unable to normalize {$path}.\n");
            exit(1);
        }
    ' "$1"
}

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
    normalize_generated_source "$output_dir/$parser_c"
    normalize_generated_source "$output_dir/$parser_h"
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
