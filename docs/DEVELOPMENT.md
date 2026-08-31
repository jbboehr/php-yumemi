# Development

This document covers local extension development, generated sources, tests, and CI. See [Architecture](ARCHITECTURE.md)
for component ownership and [Native Parser ABI](NATIVE_PARSER_ABI.md) for the internal parser interface.

## Unix-like build

For a normal development build, run:

```console
phpize
./configure --enable-yumemi
make -j4
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
```

To treat compiler warnings as errors after configuring:

```console
make clean
make -j4 CFLAGS='-g -O2 -Wall -Wextra -Werror -Wno-unused-parameter'
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
```

Use the same PHP build to load the resulting module. A module compiled for NTS, ZTS, or a different PHP module API will
not load safely into another runtime.

## Generated lexer and parser

The repository commits Flex and Bison output, so source builds and release archives do not require either tool.
Generation runs only when requested. File timestamps never trigger it as a side effect of `make`.

After configuring the extension, regenerate or verify both outputs with:

```console
make generate-sources
make check-generated-sources
```

Individual targets are:

- `generate-lexer` and `check-generated-lexer`.
- `generate-parser` and `check-generated-parser`.

Lexer generation also rebuilds the committed Unicode classification tables from pinned Unicode data. The generated
source checks fail if the maintainer tools produce files that differ from the committed copies.

## Nix gate

The flake provides PHP 8.2 through 8.5 packages and development shells. Run the complete x86_64 Linux gate with:

```console
nix flake check --keep-going -L
```

It covers:

- PHP 8.2, 8.3, 8.4, and 8.5 NTS builds and PHPTs.
- PHP 8.2 and 8.5 ZTS/debug endpoint builds.
- a PHP 8.5 Clang ASan/UBSan build.
- treefmt-nix formatting and Actionlint.
- committed Flex/Bison output.
- Composer manifest validation.
- a clean PHP 8.2 PIE build and module load without Flex or Bison.

The default flake source comes from Git, so it excludes untracked files. Stage new files before using the default check,
or use a `path:` flake reference while developing them.

## Native CI matrix

Direct pushes to `develop` and `master`, plus `v*` tag pushes, run every job below. Qualification branches beginning
with `darwin/` or `windows/` run only their matching native-platform matrix.

| Platform | Matrix |
| --- | --- |
| Linux x86_64 | PHP 8.2, 8.3, 8.4, and 8.5 NTS plus the Nix gate above |
| Intel macOS | PHP 8.2 x64 |
| Apple Silicon macOS | PHP 8.5 arm64 |
| Windows Server 2022 x64 | PHP 8.2 NTS, PHP 8.4 NTS and TS, PHP 8.5 NTS |

The macOS jobs use the standard `phpize` build. Windows uses `config.w32` through
[`php/php-windows-builder`](https://github.com/php/php-windows-builder). The Windows action receives the commit SHA as
its extension ref because it embeds that value in filenames and branch refs containing `/` are not path-safe.

## PHPT suite

| Test | Contract |
| --- | --- |
| `001-extension-loads.phpt` | Module loading and version metadata |
| `002-internal-quantity.phpt` | Abstract, non-instantiable, subclassable internal base |
| `003-operator-delegation.phpt` | Arithmetic, power, reverse division, unary signs, and multiplication order |
| `004-object-lifecycle.phpt` | Compound assignment, cloning, properties, and garbage collection |
| `005-operator-failures.phpt` | Missing methods, invalid arguments, unsupported operators, and exceptions |
| `006-native-lexer.phpt` | Token kinds, exact text, Unicode behavior, and byte spans |
| `007-native-lexer-limits.phpt` | Input, token-count, nesting, and token-size limits |
| `008-native-lexer-compatibility.phpt` | Fail-closed lexer/parser PCRE gate and atomic parser ABI selection |
| `009-native-parser.phpt` | Parser ABI, AST kinds, precedence, lexemes, and spans |
| `010-native-parser-failures.phpt` | Structured syntax failures and inherited lexer limits |
| `011-native-error-metadata.phpt` | Machine-readable syntax and resource metadata |
| `012-build-qualification.phpt` | Requested ZTS/debug modes in qualification builds |

The build-mode test skips in normal NTS runs. Qualification builds pass the expected mode to the test.

## Repository layout

- `config.m4` defines the Unix-like `phpize` build.
- `config.w32` defines the Windows build.
- `composer.json` defines the PIE extension package and Linux install envelope.
- `nix/derivation.nix` packages the extension for the flake's PHP versions.
- `php_yumemi.h` contains module metadata.
- `src/extension.c` registers the module and `phpinfo()` output.
- `src/internal_quantity.c` registers the quantity base and operator handler.
- `src/parser/scanner.l` defines the reentrant Flex scanner.
- `src/parser/native_lexer.c` owns Unicode classification, limits, and the PHP lexer interface.
- `src/parser/parser.y` defines the pure reentrant Bison grammar.
- `src/parser/native_parser.c` owns the arena AST, failures, and PHP parser interface.
- `scripts/generate-lexer.sh` regenerates the scanner and Unicode tables.
- `scripts/generate-parser.sh` regenerates the C parser.
- `Makefile.frag` supplies opt-in generated-source targets.
- `tests/` contains the PHPT suite.
