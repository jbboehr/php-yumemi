# Changelog

All notable changes to php-yumemi will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Initial loadable PHP extension scaffold and PHPT smoke test.
- Delegate exponentiation and scalar-left division to userland `pow()` and `rdiv()` methods.
- Add the abstract `jbboehr\Yumemi\InternalQuantity` seam and delegate arithmetic operators to userland methods.
- Add a reentrant native lexer for Yumemi unit expressions with version-gated Unicode parity, byte spans, and resource
  limits.
- Add a pure reentrant native parser with an arena-backed neutral AST, structured syntax failures, byte spans, and an
  internal debug API for differential testing against yumemi.php.
- Expose machine-readable unexpected/expected token lists and resource-limit metadata from native parser failures.
- Add opt-in Make targets for regenerating and checking the committed Flex and Bison sources.
- Add a PIE package manifest for PHP 8.2 through 8.5 on Linux NTS and ZTS and a Nix check that validates, builds, and
  loads the packaged extension with PIE.
- Qualify PHP 8.2 and 8.5 ZTS/debug builds and a PHP 8.5 Clang ASan/UBSan build on x86_64 Linux.

### Fixed

- Initialize Zend's thread-local cache for each request so dynamic ZTS builds can safely use operator dispatch and the
  native parser.
- Prevent ordinary source and release-archive builds from invoking implicit Flex or Bison regeneration based on file
  timestamps.
