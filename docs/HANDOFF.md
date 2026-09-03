# Cross-repository work handoff

Snapshot date: 2026-09-02

This note tracks work shared by php-yumemi and [yumemi.php](https://github.com/jbboehr/yumemi.php). Design and
maintenance details live in
[Architecture](ARCHITECTURE.md), [Native Parser ABI](NATIVE_PARSER_ABI.md), [Development](DEVELOPMENT.md), and the
[release and compatibility policy](RELEASE.md).

## Repository state

- php-yumemi `develop` baseline:
  [`bc241d6`](https://github.com/jbboehr/php-yumemi/commit/bc241d676aa736fc563df288e6053b67361dc9d0)
- yumemi.php `develop` baseline, including atomic native-parser selection and native comparison diagnostics:
  [`5e4798a`](https://github.com/jbboehr/yumemi.php/commit/5e4798a29af11e1b69ed3e773f99859df37e0b46)

An ignored checkout may exist at `tmp/yumemi.php` as a local convenience. It is a separate Git repository: never add
its files to php-yumemi, and use the remote branch and commit links above when handing work to someone who does not have
that checkout.

## Project boundary

php-yumemi is an optional syntax adapter, not a second unit engine. It supplies Zend operator handlers and a neutral
unit-expression parser result. yumemi.php remains responsible for arithmetic semantics, registry-context safety,
operand validation, AST interpretation, exceptions, and result construction. The public method API and generated PHP
parser must continue to work when the extension is absent or disabled.

The operator and parser contracts are in [Architecture](ARCHITECTURE.md) and
[Native Parser ABI](NATIVE_PARSER_ABI.md). Changes to these interfaces need matching tests in both repositories.

## Resolved multiplication contract

PHP may swap `ZEND_MUL` operands before invoking object handlers, so the extension cannot always recover the
source-level receiver. yumemi.php defines quantity multiplication independently of receiver identity. Its real
extension matrix covers PHP 8.2 through 8.5 with reversed operands, variables, helper-return and expression temporaries,
compound assignment, symbolic-factor ordering, and registry contexts.

Accepted products are canonical and retain the shared context. Rejected cross-context products expose the same
exception class, message, and ordered process-local context IDs from either operand order. php-yumemi does not need to
recover the source-level receiver.

## Verification snapshot

The current php-yumemi baseline passed all 11 jobs in
[GitHub Actions run 33597704663](https://github.com/jbboehr/php-yumemi/actions/runs/33597704663), including the Linux,
macOS, and Windows matrices described in [Development](DEVELOPMENT.md#native-ci-matrix).

During native-parser integration, 520 valid expressions and ten invalid expressions matched the generated PHP parser's
ASTs and error spans. A later deterministic 30,000-input differential probe found no mismatches. Focused yumemi.php
tests cover selection without autoloading, the atomic ABI check, forced fallback, cache isolation, AST validation, exact
lexemes, structured failures, previous-exception chaining, and fallback behavior.

A clean Linux rehearsal resolved php-yumemi `bc241d6` and yumemi.php `5e4798a` from Packagist, built the extension with
PIE 1.4.10, satisfied an application-level `ext-yumemi` Composer requirement, and produced `1.5 meter` through both the
native parser and the process-level PHP fallback.

These results are a dated snapshot, not a replacement for running both repositories' current gates after a change.

## Deferred comparison operators

Native comparison delegation was evaluated and left out of `develop`. Zend provides one callback for every non-strict
operator and for implicit comparisons. Adding quantity comparison syntax would also change `sort()`, `min()`, `max()`,
and similar functions. Incompatible quantities would then throw from all of those paths, and `>` and `>=` use a swapped
receiver order.

The evaluated extension-only implementation is preserved on the remote php-yumemi branch
[`comparison/operators`](https://github.com/jbboehr/php-yumemi/tree/comparison/operators) at commit
[`4e3f160`](https://github.com/jbboehr/php-yumemi/commit/4e3f160ec305fd034ca71407b1a78015d2193d15)
for future reference, but it is not part of the current contract. Use yumemi.php's named comparison methods instead.
Its `yumemi.nativeQuantityComparison` PHPStan diagnostic rejects non-strict comparison operators on quantities. Without
the native handler, PHP compares object state, which may disagree with the named methods even when two quantities are
physically equivalent.

## Consumer integration state

The yumemi.php adapter calls `NativeParser::supports(1)` and falls back when the method is missing or returns `false`.
php-yumemi keeps `ABI_VERSION` and `isCompatible()` for older adapters, and uses its committed Unicode tables on every
runtime. yumemi.php also rejects native quantity comparisons in PHPStan and tells operator users to require
`ext-yumemi` in application projects.

One consumer-side item remains: teach the opt-in operator model that unary `+` and `-` preserve a quantity's branded
type. That work belongs in yumemi.php and must not change runtime behavior without the extension.

## Release qualification

The initial policy decisions are recorded in [Release and compatibility policy](RELEASE.md):

- x86_64 Linux with PHP 8.2 through 8.5 NTS and ZTS is the intended supported PIE envelope for the first tag.
- the listed macOS and Windows combinations are source-build qualifications, not package-distribution promises.
- `InternalQuantity`, native parser ABI version 1, the `NativeParser::supports()` helper, neutral syntax objects,
  and C declarations are internal cross-package interfaces rather than application APIs.
- the native lexer uses its committed Unicode snapshot without requiring PHP's runtime PCRE version to match.
- the internal base remains signature-free, unary signs delegate through `mul()`, and binary scalar-left `+` and `-`
  remain unsupported.
- comparison operators remain unsupported, so applications use yumemi.php's named comparison methods.
- `yumemi-operators.neon` tells PHPStan that an application enables operator-bearing code.
- each repository owns release notes for its own public boundary and cross-references the compatible counterpart when a
  coordinated interface changes.

The PIE package is registered on Packagist with its development branches. Before the first tag, finish the remaining
yumemi.php PHPStan item, choose compatible versions, run both release gates, and repeat the clean paired installation
with those exact release candidates. PECL `package.xml` work is out of scope unless the project later chooses to publish
through PECL.
