# Cross-repository work handoff

Snapshot date: 2026-08-29

This is a coordination note for work that spans php-yumemi and
[yumemi.php](https://github.com/jbboehr/yumemi.php). Stable design and maintenance information lives in
[Architecture](ARCHITECTURE.md), [Native Parser ABI](NATIVE_PARSER_ABI.md), [Development](DEVELOPMENT.md), and the
[release and compatibility policy](RELEASE.md).

## Repository state

- php-yumemi behavior baseline:
  [`c326537`](https://github.com/jbboehr/php-yumemi/commit/c326537e7939bef490b4834cc0b733d6af8e189c)
- yumemi.php `develop` baseline, including native parsing, operator hardening, and receiver-independent multiplication:
  [`a5afea2`](https://github.com/jbboehr/yumemi.php/commit/a5afea2a9e50f199683c75fca16805708ac8b65e)

An ignored checkout may exist at `tmp/yumemi.php` as a local convenience. It is a separate Git repository: never add
its files to php-yumemi, and use the remote branch and commit links above when handing work to someone who does not have
that checkout.

## Boundary to preserve

php-yumemi is an optional syntax adapter, not a second unit engine. It supplies Zend operator handlers and a neutral
unit-expression parser result. yumemi.php remains responsible for arithmetic semantics, registry-context safety,
operand validation, AST interpretation, exceptions, and result construction. The public method API and generated PHP
parser must continue to work when the extension is absent or disabled.

The detailed operator and parser contracts are in [Architecture](ARCHITECTURE.md) and
[Native Parser ABI](NATIVE_PARSER_ABI.md). Changes that cross this boundary should have matching coverage in both
repositories.

## Resolved multiplication contract

PHP may swap `ZEND_MUL` operands before invoking object handlers, so the extension cannot universally recover the
source-level receiver. yumemi.php now defines quantity multiplication independently of receiver identity. Its real
extension matrix covers PHP 8.2 through 8.5 with reversed operands, variables, helper-return and expression temporaries,
compound assignment, symbolic-factor ordering, and registry contexts.

Accepted products are canonical and retain the shared context. Rejected cross-context products expose the same
exception class, message, and canonically ordered process-local context IDs from either operand order. Source-level
receiver recovery is therefore not required in php-yumemi.

## Verification snapshot

The php-yumemi behavior baseline passed all 11 jobs in
[GitHub Actions run 33235772245](https://github.com/jbboehr/php-yumemi/actions/runs/33235772245), including the Linux,
macOS, and Windows matrices described in [Development](DEVELOPMENT.md#native-ci-matrix).

During native-parser integration, 520 valid expressions and ten invalid expressions matched the generated PHP parser's
ASTs and error spans. A later deterministic 30,000-input differential probe found no mismatches. Focused yumemi.php
tests cover selection without autoloading, the ABI and Unicode gates, explicit opt-out, cache isolation, AST validation,
exact lexemes, structured failures, previous-exception chaining, and fallback behavior.

These results are a dated snapshot, not a replacement for running both repositories' current gates after a change.

## Deferred comparison operators

Native comparison delegation was evaluated and is intentionally excluded from `develop` for now. Zend provides one
comparison callback for every non-strict operator and for implicit comparison consumers, so it cannot offer explicit
quantity syntax without also changing `sort()`, `min()`, `max()`, and similar engine behavior. Incompatible quantities
would then throw from all of those paths, and `>`/`>=` use a swapped receiver order.

The evaluated extension-only implementation is preserved on the remote php-yumemi branch
[`comparison/operators`](https://github.com/jbboehr/php-yumemi/tree/comparison/operators) at commit
[`4e3f160`](https://github.com/jbboehr/php-yumemi/commit/4e3f160ec305fd034ca71407b1a78015d2193d15)
for future reference, but it is not part of the current integration contract. yumemi.php's named comparison methods
remain authoritative. Its `yumemi.invalidQuantityComparison` PHPStan diagnostic covers incompatible named method calls;
it does not enable or validate unsupported comparison-operator syntax.

## Release qualification

The initial policy decisions are recorded in [Release and compatibility policy](RELEASE.md):

- x86_64 Linux with PHP 8.2 through 8.5 NTS and ZTS is the intended supported PIE envelope for the first tag;
- the listed macOS and Windows combinations remain best-effort source-build qualifications rather than broader
  distribution promises;
- `InternalQuantity`, native parser ABI version 1, neutral syntax objects, and C declarations remain internal
  cross-package seams rather than application APIs;
- the internal base remains signature-free, and scalar-left `+` and `-` remain unsupported;
- comparison operators remain unsupported; applications use yumemi.php's named comparison methods;
- `yumemi-operators.neon` is the explicit PHPStan declaration that an application enables operator-bearing code; and
- each repository owns release notes for its own public boundary and cross-references the compatible counterpart when a
  coordinated seam changes.

The PIE package is already registered on Packagist and exposes its development branches. The remaining operational work
is to choose the compatible tagged yumemi.php release, verify and tag the first php-yumemi version, confirm Packagist
indexes that tag, and prove a clean paired installation. PECL `package.xml` work is not planned unless a separate PECL
publication requirement appears.
