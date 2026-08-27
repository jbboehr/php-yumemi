# Extension work handoff

Snapshot date: 2026-08-26

This document records the implemented baseline in `php-yumemi`, the corresponding local work in `yumemi.php`, and the
next cross-repository slice. It assumes the current operator-handler changes are included; it is not a description of
the earlier load-only scaffold.

## Implemented baseline

| Item | Current value |
| --- | --- |
| Repository | `jbboehr/php-yumemi` |
| Extension/module name | `yumemi` |
| Shared library | `yumemi.so` |
| Composer platform package | `ext-yumemi` |
| Development version | `0.1.0-dev` |
| Target PHP versions | 8.2 through 8.5 |
| Verified platform | x86_64 Linux, non-thread-safe PHP builds |
| License | `AGPL-3.0-only WITH romic-exception` |

The repository now contains:

- a Nix flake with PHP 8.2 through 8.5 packages, checks, development shells, and treefmt-nix formatting;
- GitHub Actions for the PHP and Nix checks;
- the ordinary `phpize && ./configure && make && make test` build path;
- an internal abstract `jbboehr\Yumemi\InternalQuantity` base class;
- custom object creation, cloning, and arithmetic handlers for descendants of that base; and
- PHPT coverage for registration, operator dispatch, compound assignment, object lifecycle, and failures.

The local checkout at `tmp/yumemi.php` contains the separate method-only library seam: an empty PHP fallback
`InternalQuantity`, `Quantity extends InternalQuantity`, and consumer/runtime coverage. Those library changes remain a
separate worktree concern and should be reviewed and committed in the yumemi.php repository, not from this repository.

## Boundary to preserve

The extension is an optional syntax adapter, not a second unit engine.

1. yumemi.php remains the semantic authority for exact arithmetic, unit parsing, normalization, conversion,
   registry-context safety, exceptions, and result construction.
2. Portable code continues to use `Quantity::add()`, `sub()`, `mul()`, and `div()`. Installing the extension must not be
   necessary to exchange quantities or call the method API.
3. The C handler delegates to public userland methods. It must not parse units, compare registries, convert values, or
   construct `Quantity` objects itself.
4. PHPStan support belongs under yumemi.php's existing `src/PHPStan` boundary and must model the same method semantics.

## Implemented operator contract

The internal base installs one stable object-handler table on userland descendants. Its `do_operation` mapping is:

| PHP operator | Zend opcode | Userland method |
| --- | --- | --- |
| `+` | `ZEND_ADD` | `add()` |
| `-` | `ZEND_SUB` | `sub()` |
| `*` | `ZEND_MUL` | `mul()` |
| `/` | `ZEND_DIV` | `div()` |

Receiver policy is deliberate:

- when the handler receives a quantity as its left operand, it uses that quantity as the receiver;
- variable-by-variable quantity multiplication preserves the written left receiver;
- scalar multiplication is supported from either side and calls the quantity receiver's `mul()` method;
- scalar-left `+`, `-`, and `/` remain unsupported and follow PHP's normal `TypeError` path; and
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
| `003-operator-delegation.phpt` | Four method mappings and left-receiver order for two quantity variables |
| `004-object-lifecycle.phpt` | Compound assignment, clone handlers, declared/readonly properties, and GC |
| `005-operator-failures.phpt` | Missing methods, unsupported operators, invalid arguments, scalar policy, and exceptions |

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

The supported Nix matrix and formatting gate is:

```console
nix flake check --keep-going -L
```

The handler slice has passed all five PHPTs on PHP 8.2, 8.3, 8.4, and 8.5 NTS builds on x86_64 Linux, plus the strict
PHP 8.2 build and formatting checks. When new files are still untracked, the default Git-backed flake source omits them;
stage them first or verify from a clean source snapshot containing the complete intended change.

## Next slice: end-to-end yumemi.php behavior

The next work belongs primarily in the yumemi.php checkout and should use the locally built extension without moving
semantic logic into C.

1. Load the local `yumemi.so` before Composer autoloading and verify that the internal base suppresses fallback-class
   autoloading without declaration collisions.
2. Add focused integration fixtures comparing each operator result with the corresponding `Quantity` method result.
3. Cover compatible-unit conversion, quantity multiplication/division, integer and `Rational` scaling, and
   cross-registry rejection.
4. Compare variable and temporary quantity multiplication forms and decide whether Zend's commutative operand swapping
   changes an observable symbolic-unit result or only an equivalent representation.
5. Verify the exact exception categories and messages remain those produced by the userland methods.
6. Exercise normal and optimized Composer autoloading with the extension present, plus the existing extension-absent
   fallback path.
7. Recheck cloning, serialization, garbage collection, and consumer behavior using the real `Quantity` class.
8. Run yumemi.php's focused checks, `composer check`, and the appropriate supported-PHP Nix matrix.

Method-call conformance fixtures remain authoritative. Operator fixtures should prove syntax adaptation and equivalence,
not encode a parallel model of unit arithmetic.

## Later slices

After end-to-end runtime behavior is stable:

- teach yumemi.php's PHPStan adapter the exact optional operator surface;
- add extension-present and extension-absent static-analysis consumer fixtures;
- settle Composer `suggest` metadata and the version relationship between yumemi.php and ext-yumemi; and
- add `package.xml`, release metadata, installation documentation, and additional platform jobs as support is proven.

## Open release questions

Do not answer these accidentally inside an unrelated implementation patch:

- Is `InternalQuantity` the final published class name and compatibility classification?
- Should a future version declare abstract arithmetic signatures on the internal base?
- Must quantity-by-quantity multiplication preserve the source-level left receiver even when Zend has reordered the
  operands, or is an equivalent commutative result sufficient?
- Should scalar-left `+`, `-`, or `/` ever gain semantics? They are currently deliberately unsupported.
- How does PHPStan discover that operator syntax is available in a particular application environment?
- Which OS, ZTS, sanitizer, and debug-build combinations are release requirements?
- Does the extension need a public C header/API, or can native declarations remain private?
- What version relationship must hold between `jbboehr/yumemi` and `ext-yumemi`?
- Which repository owns coordinated release notes and installation documentation?
