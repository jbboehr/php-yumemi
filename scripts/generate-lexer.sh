#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repository_dir=$(dirname -- "$script_dir")
scanner_source="src/parser/scanner.l"
scanner_c="src/parser/scanner.c"
scanner_h="src/parser/scanner.h"

cd "$repository_dir"

if [[ "${1:-}" == "--check" ]]; then
    generation_dir=$(mktemp -d)
    trap 'rm -rf -- "$generation_dir"' EXIT

    php "$script_dir/generate-unicode-ranges.php" --check
    LC_ALL=C flex --noline \
        --outfile="$generation_dir/scanner.c" \
        --header-file="$generation_dir/scanner.h" \
        "$scanner_source"
    cmp "$generation_dir/scanner.c" "$scanner_c"
    cmp "$generation_dir/scanner.h" "$scanner_h"
    exit 0
fi

php "$script_dir/generate-unicode-ranges.php"
LC_ALL=C flex --noline \
    --outfile="$scanner_c" \
    --header-file="$scanner_h" \
    "$scanner_source"
