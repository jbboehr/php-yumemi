# Changelog

All notable changes to php-yumemi will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.1.0] - 2026-09-05

Initial release. Compatible with yumemi.php
[`e89eb4c`](https://github.com/jbboehr/yumemi.php/commit/e89eb4cf975e4b4752c5ed06ad9b220b438c0006),
from its planned 0.2.0 release. yumemi.php 0.1.x does not provide the native integration.

### Added

- Arithmetic operators for yumemi.php quantities: `+`, `-`, `*`, `/`, and `**`, including unary signs, scalar
  multiplication from either side, and scalar-left division. Quantity methods provide the arithmetic semantics.
- Optional native unit-expression parsing with Unicode support, structured diagnostics, resource limits, and an ABI
  compatibility check. yumemi.php retains its PHP parser fallback.
- PIE installation on x86_64 Linux with PHP 8.2–8.5 NTS and ZTS, plus best-effort macOS and Windows source builds.
  Generated lexer and parser sources are included, so normal builds do not require Flex or Bison.

[Unreleased]: https://github.com/jbboehr/php-yumemi/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/jbboehr/php-yumemi/releases/tag/v0.1.0
