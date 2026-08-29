# Cross-repository work handoff

Snapshot date: 2026-08-29

This is a coordination note for work that spans php-yumemi and
[yumemi.php](https://github.com/jbboehr/yumemi.php). Stable design and maintenance information lives in
[Architecture](ARCHITECTURE.md), [Native Parser ABI](NATIVE_PARSER_ABI.md), and [Development](DEVELOPMENT.md).

## Repository state

- php-yumemi `develop` baseline:
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

The php-yumemi baseline passed all 11 jobs in
[GitHub Actions run 33235772245](https://github.com/jbboehr/php-yumemi/actions/runs/33235772245), including the Linux,
macOS, and Windows matrices described in [Development](DEVELOPMENT.md#native-ci-matrix).

During native-parser integration, 520 valid expressions and ten invalid expressions matched the generated PHP parser's
ASTs and error spans. A later deterministic 30,000-input differential probe found no mismatches. Focused yumemi.php
tests cover selection without autoloading, the ABI and Unicode gates, explicit opt-out, cache isolation, AST validation,
exact lexemes, structured failures, previous-exception chaining, and fallback behavior.

These results are a dated snapshot, not a replacement for running both repositories' current gates after a change.

## Next cross-repository work

- Decide whether macOS and Windows remain source-build qualification or become supported distribution targets.
- Tag the first release of the existing Packagist PIE package once the platform envelope is settled.
- Coordinate release notes and installation guidance between both repositories.
- Decide how long stable releases must retain older native parser ABI versions.

PECL `package.xml` work is not planned unless a separate PECL publication requirement appears.

## Open release decisions

- Is `InternalQuantity` the final published class name and compatibility classification?
- Should a future version declare abstract arithmetic signatures on the internal base?
- Should scalar-left `+` or `-` ever gain semantics?
- Which qualified macOS and Windows combinations are release requirements rather than best-effort CI?
- Does the extension need a public C header/API, or can native declarations remain private?
- Which repository owns coordinated release notes and installation documentation?
