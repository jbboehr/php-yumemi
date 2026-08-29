# Changelog

All notable changes to php-yumemi will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Add the loadable `yumemi` extension and the abstract `jbboehr\Yumemi\InternalQuantity` seam used by yumemi.php.
- Delegate `+`, `-`, `*`, `/`, and `**` to userland quantity methods, including scalar multiplication from either side
  and scalar-left division through `rdiv()`.
- Add a reentrant native lexer for Yumemi unit expressions with version-gated Unicode parity, byte spans, and resource
  limits.
- Add a pure reentrant native parser with an arena-backed neutral AST, structured syntax failures, byte spans, and an
  internal seam for differential testing against yumemi.php.
- Expose machine-readable unexpected and expected tokens plus resource-limit metadata from native parser failures.
- Add opt-in Make targets for regenerating and checking the committed Flex and Bison sources.
- Add a PIE package manifest for PHP 8.2 through 8.5 on Linux NTS and ZTS and a Nix check that validates, builds, and
  loads the packaged extension with PIE.
- Publish the development package on Packagist for installation with
  `pie install jbboehr/php-yumemi:dev-develop` until the first tagged release.
- Qualify PHP 8.2 and 8.5 ZTS/debug builds and a PHP 8.5 Clang ASan/UBSan build on x86_64 Linux.
- Add `config.w32` and qualify source builds on Intel and Apple Silicon macOS and on x64 Windows NTS and TS builds.
- Run native-platform matrices on direct `develop` pushes and on their `darwin/` and `windows/` qualification branches.
- Add the release and compatibility policy, including the supported Linux PIE envelope, best-effort native source-build
  combinations, coordinated versioning, and release-tag qualification.

### Changed

- Make native parser exception metadata natively typed and readonly, matching the documented integration ABI.

### Fixed

- Initialize Zend's thread-local cache for each request so dynamic ZTS builds can safely use operator dispatch and the
  native parser.
- Prevent ordinary source and release-archive builds from invoking implicit Flex or Bison regeneration based on file
  timestamps.
