# Release and compatibility policy

This policy describes releases prepared from the current source. `develop` is experimental and unsupported.

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

Supported package installations and source-build coverage are qualified separately:

| Tier | Combinations | Release meaning |
| --- | --- | --- |
| Supported PIE envelope | x86_64 Linux, PHP 8.2–8.5, NTS and ZTS | Reported defects are supported. Every release runs all four NTS builds, ZTS/debug endpoint builds on 8.2 and 8.5, a clean PIE build/load check, and the real yumemi.php integration matrix. |
| macOS PIE packages | Apple Silicon, macOS 15 or later, PHP 8.2–8.5, NTS and ZTS | All eight combinations build, run the PHPT suite, and load the packaged module on macOS 15. PIE selects matching release ZIPs and falls back to source when none exists. |
| Best-effort source builds | Intel macOS with PHP 8.2 NTS | This combination runs for release commits and tags. No Intel macOS binaries are published. |
| Windows PIE packages | x64 Windows, PHP 8.2–8.5, NTS and TS | All eight combinations build and run the PHPT suite for branches and tags. PIE requires the matching ZIP on a published GitHub Release. Version 0.1.0 predates these packages. |
| Unqualified | Other PHP versions, operating systems, architectures, SAPIs, and build modes | They may work, but they are unsupported until this policy includes them. |

The PIE manifest covers Linux, Windows, and macOS NTS and ZTS across the full PHP range. Linux endpoint ZTS/debug jobs test
thread safety, and the NTS matrix tests each PHP minor. A failure in any supported combination blocks the release. If a
hosted runner prevents a best-effort native job from running, record the outage instead of counting the job as a pass.

## Version coordination

`php-yumemi` and `yumemi.php` use independent Semantic Versioning. Neither package is a hard runtime dependency of the
other:

- yumemi.php must continue to install and pass its primary tests without `ext-yumemi`.
- the extension must not duplicate unit, conversion, registry, or arithmetic semantics.
- yumemi.php selects native parsing only when `NativeParser::supports()` accepts the requested parser ABI.
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
   changed. Set `PHP_YUMEMI_VERSION` in `php_yumemi.h` and `version` in `nix/derivation.nix` to the tag version without
   its `v` prefix. Update the expected version string and its length in `tests/001-extension-loads.phpt` to match.
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

   The Nix test gate compares each built module's version with its package metadata. Also compare the local module and
   Nix metadata with the planned tag; matching development versions alone do not qualify a release. Set
   `YUMEMI_RELEASE_VERSION` below to the intended tag without its `v` prefix:

   ```sh
   YUMEMI_RELEASE_VERSION=0.1.0
   YUMEMI_PACKAGE_VERSION=$(nix eval --raw .#packages.x86_64-linux.php82.version) &&
   php -n -d extension=modules/yumemi.so -r '
       foreach (["module" => phpversion("yumemi"), "Nix package" => $argv[2]] as $source => $actual) {
           if ($actual !== $argv[1]) {
               fwrite(STDERR, "$source: expected {$argv[1]}, got " . var_export($actual, true) . PHP_EOL);
               exit(1);
           }
       }
   ' "$YUMEMI_RELEASE_VERSION" "$YUMEMI_PACKAGE_VERSION"
   ```

   Both commands must succeed. Use the PHP runtime that built `modules/yumemi.so`.

5. Update yumemi.php's locked `php-yumemi` flake input to the release candidate and run its PHP 8.2–8.5 extension
   integration checks plus `nix flake check --keep-going -L`.
6. Merge the verified commit to `master`. Confirm the GitHub Actions run for that exact commit passes, including the
   listed macOS and Windows qualification jobs.
7. Create and verify a signed annotated `vX.Y.Z` tag without moving or replacing an existing tag.
8. Confirm the tag's GitHub Actions run and its automatically published GitHub Release with eight Windows and eight
   macOS ARM64 ZIP assets.
   Add the matching changelog section as release notes and verify that Packagist indexes the tag for the
   already-registered PIE package.
9. From a clean machine or temporary environment, run `pie install jbboehr/php-yumemi:X.Y.Z`, load the module, and use
   it with the named compatible yumemi.php release.

If publication fails after pushing the tag, keep the tag and repair the failed publication step. Publish a new version
only if the code must change. A PECL `package.xml` is out of scope unless the project later chooses to publish through
PECL.

## Binary release packages

After all tag CI jobs pass, `release` creates a draft release if necessary, attaches the Windows and macOS ARM64 ZIPs,
and publishes after every upload succeeds. One job uploads both platforms before publication so immutable releases
include the complete asset set. Branch and pull-request builds retain CI artifacts without touching releases.

Download a run's packages with:

```console
gh run download RUN_ID --pattern 'php_yumemi-*.zip' --dir binary-packages
```

Each Windows artifact contains the distributable ZIP at its root and a separate `logs/` ZIP. macOS artifacts contain
only the distributable ZIP. Only root ZIPs belong on the release. To repair an upload, rerun the failed tag job;
it publishes only after a successful upload. Immutable releases cannot accept replacement assets.

### Windows

The CI workflow uses the official [PHP Windows builder](https://github.com/php/php-windows-builder) to produce
[PIE-compatible Windows packages](https://github.com/php/pie/blob/1.5.x/docs/extension-maintainers.md#windows-support).
The ZIP and its extension DLL share the name
`php_yumemi-{tag}-{php-version}-{nts|ts}-{compiler}-x86_64`, with their respective suffixes. Preserve the tag's `v`
prefix. PHP 8.2 and 8.3 use `vs16`; PHP 8.4 and 8.5 use `vs17`.

For a local Windows rebuild, install the matching Visual Studio toolchain and the builder's `BuildPhpExtension`
PowerShell module, then run from the tagged source checkout:

```powershell
Get-Content LICENSE.md, docs/LICENSE_EXCEPTION.md, docs/UDUNITS-COPYRIGHT |
    Set-Content -Encoding utf8 LICENSE
Invoke-PhpBuildExtension -ExtensionRef vX.Y.Z -PhpVersion 8.4 -Arch x64 -Ts nts -Args '--enable-yumemi'
```

Repeat for each PHP version and thread-safety mode. The output is under `artifacts/`. Verify the ZIP contains its
matching DLL and `LICENSE`, then run a clean Windows PIE install and module-load check after the release is published.

### macOS ARM64

The macOS job packages its tested `yumemi.so` using PIE's
[pre-packaged binary format](https://github.com/php/pie/blob/1.5.x/docs/extension-maintainers.md#pre-packaged-binary):
`php_yumemi-{tag}_php{php-version}-arm64-darwin-bsdlibc-{nts|zts}.zip`. The archive contains `yumemi.so`, `LICENSE.md`,
`LICENSE_EXCEPTION.md`, and `UDUNITS-COPYRIGHT` at its root. Branch builds substitute the commit SHA for the tag.

The packages target macOS 15 or later and non-debug PHP builds. CI rejects libraries outside `/usr/lib` and
`/System/Library` so packages do not depend on Homebrew paths from the runner. It also verifies the module's ARM64
architecture and loads the module extracted from each ZIP.

To rebuild locally on Apple Silicon, select the required PHP version and NTS or ZTS mode, then run:

```console
export MACOSX_DEPLOYMENT_TARGET=15.0
phpize
./configure --enable-yumemi
make -j4
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
otool -L modules/yumemi.so
zip -j php_yumemi-vX.Y.Z_php8.4-arm64-darwin-bsdlibc-nts.zip modules/yumemi.so LICENSE.md docs/LICENSE_EXCEPTION.md docs/UDUNITS-COPYRIGHT
```

Adjust the filename to match the tag, PHP minor version, and thread-safety mode. Verify the extracted module loads
with that PHP installation. After publication, run a clean PIE install to check release discovery and installation.
The manifest prefers prebuilt binaries, with source fallback for Linux, Intel macOS, debug builds, and missing assets.
Windows continues to use PIE's Windows DLL download path.
