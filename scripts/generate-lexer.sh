#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repository_dir=$(dirname -- "$script_dir")
scanner_source="src/parser/scanner.l"
scanner_c="src/parser/scanner.c"
scanner_h="src/parser/scanner.h"

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

cd "$repository_dir"

if [[ "${1:-}" == "--check" ]]; then
    generation_dir=$(mktemp -d)
    trap 'rm -rf -- "$generation_dir"' EXIT

    php "$script_dir/generate-unicode-ranges.php" --check
    LC_ALL=C flex --noline \
        --outfile="$generation_dir/scanner.c" \
        --header-file="$generation_dir/scanner.h" \
        "$scanner_source"
    normalize_generated_source "$generation_dir/scanner.c"
    normalize_generated_source "$generation_dir/scanner.h"
    cmp "$generation_dir/scanner.c" "$scanner_c"
    cmp "$generation_dir/scanner.h" "$scanner_h"
    exit 0
fi

php "$script_dir/generate-unicode-ranges.php"
LC_ALL=C flex --noline \
    --outfile="$scanner_c" \
    --header-file="$scanner_h" \
    "$scanner_source"
normalize_generated_source "$scanner_c"
normalize_generated_source "$scanner_h"
