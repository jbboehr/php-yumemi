# Release and compatibility policy

This policy describes the first tagged `php-yumemi` release. Until that tag exists, `develop` is experimental and
unsupported.

The extension is optional. [yumemi.php](https://github.com/jbboehr/yumemi.php) keeps its method API and generated parser
when no native code is installed.

## Compatibility boundaries

A tagged extension release supports:

- the `jbboehr/php-yumemi` PIE package identity.
- the `yumemi` PHP module and `ext-yumemi` Composer platform-package names.
- loading the module before Composer's autoloader and then using a compatible tagged `yumemi.php` release.
- the documented operator delegation and native-parser fallback behavior.
- PHP 8.2 through 8.5 within the platform envelope below.

The following are internal interfaces between the two packages, not application APIs:

- `jbboehr\Yumemi\InternalQuantity` and its object-handler layout.
- the native lexer and parser classes, neutral AST arrays, and exception objects.
- `NativeParser::ABI_VERSION` and `NativeParser::supports()`, including the current ABI version `1`.
- all C declarations and headers in this repository.

Applications should use `Quantity` and yumemi.php's public parser behavior. Changes to an internal interface must be
coordinated across both repositories and must preserve yumemi.php's methods, fallback parser, and compatibility check.
The project does not promise a public C header or stable C ABI. The internal base declares no abstract arithmetic
methods. Public userland methods define the arithmetic contract.

## Platform envelope

The first release separates supported package installations from source-build coverage:

| Tier | Combinations | Release meaning |
| --- | --- | --- |
| Supported PIE envelope | x86_64 Linux, PHP 8.2–8.5, NTS and ZTS | Reported defects are supported. Every release runs all four NTS builds, ZTS/debug endpoint builds on 8.2 and 8.5, a clean PIE build/load check, and the real yumemi.php integration matrix. |
| Best-effort source builds | Intel macOS with PHP 8.2 and Apple Silicon macOS with PHP 8.5 | These exact combinations run for release commits and tags. They qualify the source-build path but do not promise PIE installation or every PHP/architecture combination on macOS. |
| Best-effort source builds | x64 Windows with PHP 8.2, 8.4, and 8.5 NTS plus PHP 8.4 TS | These exact combinations run for release commits and tags. They qualify `config.w32` source builds but do not promise PIE installation or unlisted Windows combinations. |
| Unqualified | Other PHP versions, operating systems, architectures, SAPIs, and build modes | They may work, but they are unsupported until this policy includes them. |

The Linux PIE manifest covers NTS and ZTS across the full PHP range. Endpoint ZTS/debug jobs test thread safety, and the
NTS matrix tests each PHP minor. A failure in any supported combination blocks the release. If a hosted runner prevents
a best-effort native job from running, record the outage instead of counting the job as a pass.

## Version coordination

`php-yumemi` and `yumemi.php` use independent Semantic Versioning. Neither package is a hard runtime dependency of the
other:

- yumemi.php must continue to install and pass its primary tests without `ext-yumemi`.
- the extension must not duplicate unit, conversion, registry, or arithmetic semantics.
- yumemi.php selects native parsing only when the advertised parser ABI and Unicode compatibility conditions both
  match. `NativeParser::supports()` is the preferred atomic gate for the coordinated adapter update.
- applications should upgrade both packages together while the internal interfaces remain provisional.
- a release note that changes an operator or parser interface must name the compatible release or commit in the other
  repository.

php-yumemi release notes cover native implementation, builds, platforms, PIE installation, and ABI changes. yumemi.php
release notes cover public methods, parser behavior, PHPStan configuration, fallback behavior, and application
migration. Changes shared by both packages need an entry in each changelog.

PHPStan cannot detect whether a deployment loads the native module. Applications enable quantity-operator inference by
including yumemi.php's `yumemi-operators.neon`. Applications that use operators should also require `ext-yumemi` in
their root Composer project so deployment checks fail when the module is absent. Portable libraries should depend only
on yumemi.php.

The first release does not overload quantity comparisons. Zend uses the same comparison hook for operators and implicit
comparisons, but yumemi.php comparisons may throw for incompatible quantities. Without a handler, PHP compares object
state, not quantity values, and may disagree with the named methods. Applications should use those methods unless a
future release adopts the wider runtime behavior. Unary `+` and `-` are supported through Zend's multiplication
lowering and delegate to `mul(1)` and `mul(-1)`.

## Prepare and verify a release

Prepare releases on `develop`, merge the tested commit into `master`, and tag that exact `master` commit.

1. Choose a version and complete `CHANGELOG.md`, including the compatible yumemi.php version or commit when an interface
   changed.
2. Verify generated scanner and parser sources are current.
3. Run the normal and strict source-build gates:

   ```console
   phpize
   ./configure --enable-yumemi
   make -j4
   NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
   make clean
   make -j4 CFLAGS='-g -O2 -Wall -Wextra -Werror -Wno-unused-parameter'
   NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
   ```

4. Run the reproducible package, generated-source, ZTS/debug, sanitizer, and PIE checks:

   ```console
   nix flake check --keep-going -L
   ```

5. Update yumemi.php's locked `php-yumemi` flake input to the release candidate and run its PHP 8.2–8.5 extension
   integration checks plus `nix flake check --keep-going -L`.
6. Merge the verified commit to `master`. Confirm the GitHub Actions run for that exact commit passes, including the
   listed macOS and Windows qualification jobs.
7. Create and verify a signed annotated `vX.Y.Z` tag without moving or replacing an existing tag.
8. Confirm the tag's GitHub Actions run, create the GitHub Release from the matching changelog section, and verify that
   Packagist indexes the tag for the already-registered PIE package.
9. From a clean machine or temporary environment, run `pie install jbboehr/php-yumemi:X.Y.Z`, load the module, and use
   it with the named compatible yumemi.php release.

If publication fails after pushing the tag, keep the tag and repair the failed publication step. Publish a new version
only if the code must change. A PECL `package.xml` is out of scope unless the project later chooses to publish through
PECL.
