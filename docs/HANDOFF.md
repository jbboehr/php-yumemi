# Extension work handoff

Snapshot date: 2026-08-29

This document records the implemented baseline in `php-yumemi`, the corresponding local work in `yumemi.php`, and the
next cross-repository slice. It assumes the current operator-handler changes are included; it is not a description of
the earlier load-only scaffold.

## Implemented baseline

| Item | Current value |
| --- | --- |
| Repository | `jbboehr/php-yumemi` |
| Extension/module name | `yumemi` |
| Shared library | `yumemi.so` on Unix-like platforms; `php_yumemi.dll` on Windows |
| Composer platform package | `ext-yumemi` |
| Development version | `0.1.0-dev` |
| Target PHP versions | 8.2 through 8.5 |
| Verified platforms | x86_64 Linux, x64 and arm64 macOS, and x64 Windows |
| License | `(AGPL-3.0-only WITH romic-exception) AND UCAR` |

The repository now contains:

- a Nix flake with PHP 8.2 through 8.5 packages, checks, development shells, and treefmt-nix formatting;
- GitHub Actions for the Linux PHP and Nix checks plus native macOS and Windows source-build matrices;
- a Linux-only PIE package manifest for PHP 8.2 through 8.5 NTS and ZTS, with a clean-source build/load check;
- the ordinary `phpize && ./configure && make && make test` build path;
- an internal abstract `jbboehr\Yumemi\InternalQuantity` base class;
- custom object creation, cloning, and arithmetic handlers for descendants of that base;
- a reentrant, length-aware Flex lexer for the Yumemi unit-expression token grammar, including Unicode classification,
  byte spans, and parser resource limits;
- a pure reentrant Bison parser with an arena-backed neutral AST, structured syntax errors, and an internal PHP debug
  seam for differential testing; and
- PHPT coverage for registration, operator dispatch, compound assignment, object lifecycle, failures, native token
  parity, and native parser behavior.

The local checkout at `tmp/yumemi.php` has the extension integration on branch `native-lexer-parser`. Local commits
`a6911e6` and `94dc34d` add the method/operator seam, compatible native-parser selection and translation, the PHP
fallback, an environment opt-out, differential integration coverage, and parser benchmarks. Those commits remain a
separate repository concern; do not commit yumemi.php files from this repository.

## Boundary to preserve

The extension is an optional syntax adapter, not a second unit engine.

1. yumemi.php remains the semantic authority for exact arithmetic, AST interpretation, normalization, conversion,
   registry-context safety, exceptions, and result construction.
2. Portable code continues to use `Quantity::add()`, `sub()`, `mul()`, `div()`, `pow()`, and `rdiv()`. Installing the
   extension must not be necessary to exchange quantities or call the method API.
3. The C handler delegates to public userland methods. Native parser components must remain syntax-only: they must not
   resolve unit names, compare registries, convert values, or construct `Quantity` objects themselves.
4. PHPStan support belongs under yumemi.php's existing `src/PHPStan` boundary and must model the same method semantics.

## Implemented operator contract

The internal base installs one stable object-handler table on userland descendants. Its `do_operation` mapping is:

| PHP operator | Zend opcode | Userland method |
| --- | --- | --- |
| `+` | `ZEND_ADD` | `add()` |
| `-` | `ZEND_SUB` | `sub()` |
| `*` | `ZEND_MUL` | `mul()` |
| `/` | `ZEND_DIV` | `div()` |
| `**` | `ZEND_POW` | `pow()` |

When the quantity is the right operand of `ZEND_DIV`, the handler instead delegates to `rdiv()` on that quantity and
forwards the left operand unchanged. The canonical yumemi.php `Quantity::rdiv(int|Rational)` signature owns numerator
validation; the C adapter does not duplicate that userland rule.

Receiver policy is deliberate:

- when the handler receives a quantity as its left operand, it uses that quantity as the receiver;
- variable-by-variable quantity multiplication preserves the written left receiver;
- scalar multiplication is supported from either side and calls the quantity receiver's `mul()` method;
- scalar-left division calls the right quantity's `rdiv()` method, whose signature defines the accepted numerator types;
- scalar-left `+`, `-`, and exponentiation remain unsupported and follow PHP's normal `TypeError` path; and
- unsupported opcodes return `FAILURE` so PHP retains its normal failure behavior.

PHP may swap any `ZEND_MUL` operands before invoking object handlers. Consequently, `$quantity * 2` and literal
`2 * $quantity` can be indistinguishable at the handler boundary. The same applies to two quantities when, for example,
one operand is a temporary and the other is a variable. Treating scalar multiplication as commutative gives consistent
behavior for literals and variables while still selecting the quantity's unit and registry context, but the extension
cannot universally recover the source-level left receiver for quantity-by-quantity multiplication. End-to-end testing
must determine whether this affects yumemi.php's observable symbolic-unit contract before release.

The handler also preserves:

- public instance-method dispatch and userland argument type checks;
- delegated return values and exceptions;
- compound-assignment aliasing when the VM result reuses the left zval;
- declared and readonly userland properties;
- the custom handler table after cloning; and
- ordinary garbage collection for cyclic object graphs.

The internal base intentionally declares no abstract arithmetic methods. This avoids imposing new signature-variance
constraints on the pure-PHP library while the cross-repository API remains experimental.

## PHPT coverage

| Test | Contract |
| --- | --- |
| `001-extension-loads.phpt` | Module loading and version metadata |
| `002-internal-quantity.phpt` | Internal, abstract, non-instantiable, subclassable base class |
| `003-operator-delegation.phpt` | Arithmetic, power, reverse-division mappings, and quantity multiplication order |
| `004-object-lifecycle.phpt` | Compound assignment, clone handlers, declared/readonly properties, and GC |
| `005-operator-failures.phpt` | Missing methods, unsupported operators, invalid arguments, scalar policy, and exceptions |
| `006-native-lexer.phpt` | Token kinds, exact text, Unicode behavior, and byte spans |
| `007-native-lexer-limits.phpt` | Input, token-count, nesting, and token-size limits |
| `008-native-lexer-compatibility.phpt` | Shared fail-closed lexer/parser runtime PCRE compatibility gate |
| `009-native-parser.phpt` | Parser ABI, AST kinds, precedence, exact lexemes, and byte spans |
| `010-native-parser-failures.phpt` | Structured syntax failures and inherited lexer resource limits |
| `011-native-error-metadata.phpt` | Machine-readable unexpected/expected tokens and resource-limit metadata |
| `012-build-qualification.phpt` | Requested ZTS and debug runtime modes in qualification builds |

## Verification

The ordinary local gate is:

```console
phpize
./configure --enable-yumemi
make -j4
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
```

The strict development gate is:

```console
make clean
make -j4 CFLAGS='-g -O2 -Wall -Wextra -Werror -Wno-unused-parameter'
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
```

The supported Nix matrix, formatting, generated-source, and PIE packaging gate is:

```console
nix flake check --keep-going -L
```

Maintainers can regenerate or verify both committed Flex/Bison outputs after configuring the extension with
`make generate-sources` or `make check-generated-sources`. The ordinary build never regenerates them implicitly.

The twelve-test PHPT collection runs on PHP 8.2, 8.3, 8.4, and 8.5 NTS builds on x86_64 Linux: all eleven behavior tests
pass and the build-mode-only test skips. All twelve pass on debug-enabled ZTS builds at the PHP 8.2 and 8.5 endpoints.
A PHP 8.5 Clang ASan/UBSan build also passes all eleven behavior tests with Zend's allocator disabled. The real
yumemi.php extension integration suite passes under both qualified ZTS/debug endpoints. Native source-build CI also
passes on Intel macOS with PHP 8.2, Apple Silicon macOS with PHP 8.5, and x64 Windows with PHP 8.2, 8.4, and 8.5 NTS
plus PHP 8.4 TS. Those native matrices run on direct `develop` pushes and on `darwin/**` or `windows/**` qualification
branches; the platform-prefixed branches skip the unrelated jobs. The parser extends the generated-source check to
Bison output. The PIE check validates the Linux-only Composer manifest, performs a clean PHP 8.2 PIE build without Flex
or Bison, and loads the resulting module. When new files are still untracked, the default Git-backed flake source omits
them; stage them first or verify with a `path:` flake source containing the intended tree.

## Implemented native syntax parser

The lexer remains usable independently so its compatibility boundary can be tested directly. `NativeParser::parse()`
now consumes the same reentrant scanner and returns a nested neutral AST through the internal PHP seam:

- leaf nodes contain `kind`, `start`, `end`, and exact source `text`;
- binary nodes contain `kind`, `start`, `end`, `left`, and `right`;
- synthesized nodes such as the `-1` used for unary negation have null spans;
- syntax failures throw internal `NativeParseException` objects with `input`, `start`, `end`, `unexpected`, and
  `expected`;
- resource failures throw internal `NativeLimitException` objects with `limit`, `maximum`, `observed`, `start`, and
  `end`; and
- `NativeParser::ABI_VERSION` is `1`, while `NativeParser::isCompatible()` shares the lexer's fail-closed PCRE gate.

The grammar performs no registry or unit lookup. A deterministic differential corpus of 520 valid expressions and ten
invalid expressions matched the current yumemi.php parser's ASTs and error spans during implementation.

## Implemented yumemi.php native-parser selection

The local yumemi.php branch now adapts the neutral native arrays into its existing AST classes. It selects the native
path only when the class is already loaded, `ABI_VERSION === 1`, `isCompatible()` is true, and
`YUMEMI_NATIVE_PARSER` is not `0`. It translates structured native syntax and limit failures into the existing public
exception contract and rejects malformed neutral ASTs. Missing, disabled, incompatible, or future-ABI extensions use
the generated PHP parser unchanged.

Focused adapter and real-extension suites cover selection without autoloading, ABI and Unicode gates, cache isolation,
input limits before native dispatch, AST translation, exact lexemes, error metadata, previous-exception chaining, and
fallback behavior. A deterministic 30,000-input differential probe found no mismatches. On one local PHP 8.2 NTS
system, the benchmark measured roughly 9.2--11 times faster syntax-only parsing and 2.4--3.8 times faster fresh
parse-and-resolve work; treat these as development measurements rather than release guarantees.

The PHP fallback remains authoritative and independently supported. Native parser selection is an optimization, not a
new semantic dependency.

## Later slices

The next slice is release-policy qualification:

- decide whether to expand the Linux-only PIE envelope using the separate macOS and Windows source-build evidence;
- register the tagged PIE package with Packagist once the supported envelope is settled;
- coordinate release notes between the extension and yumemi.php; and
- decide whether the experimental native parser ABI needs a longer-lived compatibility policy before a stable release.

The extension targets PIE, so PECL `package.xml` work is not planned unless a separate PECL publication requirement is
introduced.

## Open release questions

Do not answer these accidentally inside an unrelated implementation patch:

- Is `InternalQuantity` the final published class name and compatibility classification?
- Should a future version declare abstract arithmetic signatures on the internal base?
- Must quantity-by-quantity multiplication preserve the source-level left receiver even when Zend has reordered the
  operands, or is an equivalent commutative result sufficient?
- Should scalar-left `+` or `-` ever gain semantics? They are currently deliberately unsupported.
- How does PHPStan discover that operator syntax is available in a particular application environment?
- Which of the qualified macOS and Windows combinations are release requirements, and which are best-effort CI?
- Does the extension need a public C header/API, or can native declarations remain private?
- Which repository owns coordinated release notes and installation documentation?
