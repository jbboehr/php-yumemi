# php-yumemi

Experimental native PHP extension companion to
[yumemi.php](https://github.com/jbboehr/yumemi.php).

The module registers `jbboehr\Yumemi\InternalQuantity`, an abstract base class whose object handler delegates arithmetic
operators to public methods implemented by userland descendants. Arithmetic and unit semantics remain in yumemi.php. In
addition to admitting operator syntax, the extension contains an experimental native lexer for the Yumemi
unit-expression grammar and a native syntax parser that builds a neutral debug AST. Neither component resolves unit
names or constructs semantic unit expressions.

## Status

This is an early experiment, not a supported release. The initial build and PIE package target PHP 8.2 through 8.5 on
Linux NTS and ZTS builds. ZTS is qualified with debug-enabled PHP 8.2 and 8.5 builds on x86_64 Linux. Windows and other
Unix-like platforms remain unsupported or unverified.

## Operators

For descendants of `InternalQuantity`, the extension maps `+`, `-`, `*`, `/`, and `**` to `add()`, `sub()`, `mul()`,
`div()`, and `pow()` respectively. It passes the other operand to the selected method and returns the method's result
unchanged.

Multiplication is treated as commutative at the handler boundary, so both `$quantity * 2` and `2 * $quantity` delegate
to `$quantity->mul(2)`. PHP normalizes some `ZEND_MUL` expressions before invoking object handlers, making the original
operand order unavailable. Scalar-left division delegates `2 / $quantity` to `$quantity->rdiv(2)`. Scalar-left `+`,
`-`, and `**` remain unsupported.

The handler forwards the selected operand unchanged. The canonical yumemi.php `Quantity::rdiv(int|Rational)` signature
owns numerator validation, just as the other userland arithmetic methods own their operand rules.

## Install with PIE

The repository contains a [PIE](https://github.com/php/pie) package manifest. From a checkout, use PIE to build and
install the extension for the selected PHP installation:

```console
pie install
```

After a tagged release is registered with Packagist, the corresponding package command will be:

```console
pie install jbboehr/php-yumemi
```

The current manifest admits Linux NTS and ZTS builds on PHP 8.2 through 8.5. PIE's ZTS flag applies to that complete
envelope; the current ZTS qualification evidence is narrower—PHP 8.2 and 8.5 debug builds on x86_64 Linux. The manifest
does not claim Windows support. This project targets PIE rather than PECL, so it does not carry a `package.xml` manifest.

## Build from source

A PHP development package, a C compiler, Autoconf, and Make are required.

```console
phpize
./configure --enable-yumemi
make -j4
make test
```

To try the uninstalled extension:

```console
php -d extension="$PWD/modules/yumemi.so" -r 'var_dump(extension_loaded("yumemi"));'
```

The generated lexer and parser sources are committed, so Flex and Bison are not required for an ordinary build.
Maintainers can regenerate them with PHP, Flex, and Bison using:

```console
make generate-sources
make check-generated-sources
```

The corresponding individual targets are `generate-lexer`, `generate-parser`, `check-generated-lexer`, and
`check-generated-parser`. These targets are opt-in; the ordinary build always uses the committed generated sources.

`jbboehr\Yumemi\Parser\NativeLexer` is an internal integration seam. Its `tokenize()` method currently exposes token
text and zero-based, half-open byte spans for parity testing; applications should not depend on this experimental API.
Because yumemi.php classifies Unicode through its runtime PCRE, callers must check `isCompatible()` before selecting the
native path. `tokenize()` throws if the committed Unicode tables cannot guarantee parity with that PCRE version.
Resource failures from either native syntax seam throw the internal `NativeLimitException`, whose `limit`, `maximum`,
`observed`, `start`, and `end` properties allow yumemi.php to reproduce its public limit exception contract.

`jbboehr\Yumemi\Parser\NativeParser` is the corresponding internal parser seam. `parse()` returns nested arrays with a
node `kind`, a zero-based half-open byte `start`/`end` span, and either exact leaf `text` or `left`/`right` children.
Synthesized nodes have null spans. Syntax failures throw the internal `NativeParseException`, whose `input`, `start`,
`end`, `unexpected`, and `expected` properties support differential tests and translation to yumemi.php's public error
types without parsing Bison's diagnostic prose. Callers must check `isCompatible()` before selecting this experimental
path; the current parser ABI is exposed as `NativeParser::ABI_VERSION`.

## Version compatibility

yumemi.php treats the extension as optional. The method-based quantity API and generated PHP parser work without it.
The library selects the native parser only when the extension exposes the expected parser ABI and reports compatible
Unicode data; otherwise it falls back to PHP. `YUMEMI_NATIVE_PARSER=0` disables native parsing without unloading the
extension.

This ABI gate, rather than an exact matching package version, defines the current parser compatibility boundary.
Applications should nevertheless upgrade yumemi.php and `ext-yumemi` together while both packages remain experimental.
The extension package version does not replace yumemi.php's own Composer version requirement.

### Nix

The flake provides packages, checks, and development shells for PHP 8.2 through 8.5. PHP 8.2 is the default:

```console
nix develop
phpize
./configure --enable-yumemi
make -j4
make test
```

Select another supported PHP version by attribute, for example `nix develop .#php85` or `nix build .#php85`. Run the
PHPT suite against every supported PHP version and verify Nix formatting with `nix flake check`. Format Nix files with
`nix fmt`. On x86_64 Linux, that gate also exercises PHP 8.2 and 8.5 ZTS/debug builds and a Clang ASan/UBSan build. With
direnv installed, the checked-in `.envrc` enters the default shell automatically.

## Layout

- `config.m4` defines the `phpize` build.
- `composer.json` defines the PIE extension package and its supported PHP/platform envelope.
- `nix/derivation.nix` packages the extension for the flake's supported PHP versions.
- `php_yumemi.h` contains module metadata.
- `src/extension.c` registers the module and its `phpinfo()` output.
- `src/internal_quantity.c` registers the userland seam and operator handler.
- `src/parser/scanner.l` defines the reentrant Flex scanner.
- `src/parser/native_lexer.c` owns Unicode classification, resource limits, and the internal PHP lexer seam.
- `src/parser/parser.y` defines the pure reentrant Bison grammar.
- `src/parser/native_parser.c` owns the arena AST, structured failures, and internal PHP parser seam.
- `scripts/generate-lexer.sh` regenerates and verifies the committed scanner and Unicode tables.
- `scripts/generate-parser.sh` regenerates and verifies the committed C parser.
- `Makefile.frag` provides opt-in maintainer targets for regenerating and checking generated sources.
- `tests/` contains PHPT behavior tests.

## License

php-yumemi is licensed under `AGPL-3.0-only WITH romic-exception`. The native scanner contains portions derived from
UDUNITS2 under the UCAR license. See [LICENSE.md](LICENSE.md), [docs/LICENSE_EXCEPTION.md](docs/LICENSE_EXCEPTION.md),
and [docs/UDUNITS-COPYRIGHT](docs/UDUNITS-COPYRIGHT).
