# Release and compatibility policy

This policy defines the intended contract for the first tagged `php-yumemi` release. Until that tag exists, the
`develop` branch remains experimental and is not itself a supported release.

The extension is an optional companion to [yumemi.php](https://github.com/jbboehr/yumemi.php). Yumemi's method API and
generated PHP parser remain authoritative and available without native code.

## Compatibility boundaries

A tagged extension release supports:

- the `jbboehr/php-yumemi` PIE package identity;
- the `yumemi` PHP module and `ext-yumemi` Composer platform-package names;
- loading the module before Composer's autoloader and then using a compatible tagged `yumemi.php` release;
- the documented operator delegation and native-parser fallback behavior; and
- PHP 8.2 through 8.5 within the platform envelope below.

The following are cross-package implementation seams, not application APIs:

- `jbboehr\Yumemi\InternalQuantity` and its object-handler layout;
- the native lexer and parser classes, neutral AST arrays, and exception objects;
- `NativeParser::ABI_VERSION` and `NativeParser::supports()`, including the current ABI version `1`; and
- all C declarations and headers in this repository.

Applications should use `Quantity` and the public parser behavior exposed by yumemi.php. The two repositories may
revise an internal seam only through a coordinated change that preserves yumemi.php's method API, fallback parser, and
compatibility gate. No public C header or stable C ABI is promised. The internal base deliberately declares no abstract
arithmetic methods; public userland methods remain the semantic contract.

## Platform envelope

The initial release policy distinguishes supported package installations from narrower source-build evidence:

| Tier | Combinations | Release meaning |
| --- | --- | --- |
| Supported PIE envelope | x86_64 Linux, PHP 8.2–8.5, NTS and ZTS | Reported defects are supported. Every release runs all four NTS builds, ZTS/debug endpoint builds on 8.2 and 8.5, a clean PIE build/load check, and the real yumemi.php integration matrix. |
| Best-effort source builds | Intel macOS with PHP 8.2; Apple Silicon macOS with PHP 8.5 | These exact combinations run for release commits and tags. They qualify the ordinary source-build path but do not promise PIE installation or every PHP/architecture combination on macOS. |
| Best-effort source builds | x64 Windows with PHP 8.2, 8.4, and 8.5 NTS plus PHP 8.4 TS | These exact combinations run for release commits and tags. They qualify `config.w32` source builds but do not promise PIE installation or unlisted Windows combinations. |
| Unqualified | Other PHP versions, operating systems, architectures, SAPIs, and build modes | They may work, but no support or release-gate claim is made until they are added deliberately. |

The Linux PIE manifest necessarily describes NTS and ZTS across the complete PHP range. Endpoint ZTS/debug jobs test
the thread-safety boundary, while the NTS matrix tests each PHP minor. A failure in any supported combination is a
release blocker. Hosted-runner outages affecting a best-effort native job must be recorded rather than silently
reclassified as product success.

## Version coordination

`php-yumemi` and `yumemi.php` use independent Semantic Versioning. Neither package imposes the other as a hard runtime
dependency:

- yumemi.php must continue to install and pass its primary tests without `ext-yumemi`;
- the extension must not duplicate unit, conversion, registry, or arithmetic semantics;
- yumemi.php selects native parsing only when the advertised parser ABI and Unicode compatibility conditions both
  match; `NativeParser::supports()` is the preferred atomic gate for the coordinated adapter update;
- applications should upgrade both packages together while the extension seams remain provisional; and
- a release note that changes an operator or parser seam must name the compatible release or commit in the other
  repository.

Extension release notes own native implementation, build, platform, PIE installation, and ABI changes. Yumemi.php
release notes own public method semantics, parser behavior, PHPStan configuration, fallback behavior, and application
migration. A coordinated change belongs in both changelogs from those respective perspectives.

PHPStan cannot discover whether a deployment loads the native module. Applications opt into quantity-operator
inference by including yumemi.php's `yumemi-operators.neon`; this explicit declaration is the supported discovery
mechanism. An application that uses the operators should also require `ext-yumemi` in its root Composer project so
deployment checks fail when the module is absent; portable libraries should continue to depend only on yumemi.php.

The initial operator surface does not include quantity comparison syntax. Zend's comparison hook also controls implicit
engine consumers and cannot distinguish them from explicit operators, while yumemi.php comparisons may throw for
incompatible quantities. In the absence of a handler, PHP still applies its ordinary object-state comparison, which is
not a quantity relation and can disagree with named methods. Applications should use the named comparison methods
unless a future coordinated release explicitly adopts that broader runtime contract. Unary `+` and `-` are supported
through Zend's multiplication lowering and delegate to `mul(1)` and `mul(-1)` respectively.

## Prepare and verify a release

Releases are prepared on `develop`, merged without untested changes into `master`, and tagged from the exact verified
`master` commit.

1. Choose a version and complete `CHANGELOG.md`, including the compatible yumemi.php version or commit when a seam
   changed.
2. Verify generated scanner and parser sources are current.
3. Run the ordinary and strict source-build gates:

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

If publication fails after the tag is pushed, preserve the immutable tag and repair the missing service state. Publish
a new version only when the released code must change. PECL `package.xml` publication remains out of scope unless a
separate requirement is adopted.
