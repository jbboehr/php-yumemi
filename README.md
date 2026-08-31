# php-yumemi

Experimental native PHP extension companion to
[yumemi.php](https://github.com/jbboehr/yumemi.php).

The extension adds two optional capabilities to the pure-PHP library:

- `Quantity` objects can use PHP arithmetic operators, which delegate to the existing public method API; and
- compatible yumemi.php versions can parse unit expressions with the native lexer and parser, falling back to the PHP
  parser automatically when the native path is unavailable.

Arithmetic, conversion, registry handling, and result construction remain in yumemi.php. Installing this extension does
not create a second unit engine or make the extension a dependency of portable library code.

The integration has not yet shipped in a tagged yumemi.php release. Until it does, the installation below uses the
`develop` branch, which contains both the operator seam and native-parser adapter.

## Status and platforms

This is an early experiment, not a supported release.

The [release and compatibility policy](docs/RELEASE.md) defines the intended first-release Linux envelope, best-effort
native source-build combinations, provisional cross-package seams, and exact release gate.

| Environment | Source-build CI coverage | Distribution status |
| --- | --- | --- |
| x86_64 Linux | PHP 8.2–8.5 NTS; PHP 8.2/8.5 ZTS+debug; PHP 8.5 ASan/UBSan | Published PIE package target |
| Intel macOS | PHP 8.2 | Source-build qualification only |
| Apple Silicon macOS | PHP 8.5 | Source-build qualification only |
| x64 Windows | PHP 8.2, 8.4, and 8.5 NTS; PHP 8.4 TS | Source-build qualification only; no DLL release yet |

The Composer manifest intentionally limits PIE installation to Linux NTS and ZTS builds on PHP 8.2 through 8.5.
Passing source-build jobs on macOS and Windows do not yet constitute a published installation path for those platforms.

## Install and use

Install the yumemi.php library in the application first:

```console
composer require jbboehr/yumemi:dev-develop
```

Tagged yumemi.php v0.1.1 predates `InternalQuantity`, the parser adapter, and `yumemi-operators.neon`; it cannot provide
the integration documented here.

### Install the extension with PIE

The extension is published on Packagist as
[`jbboehr/php-yumemi`](https://packagist.org/packages/jbboehr/php-yumemi). On Linux, install it for the selected PHP
installation with [PIE](https://github.com/php/pie):

```console
pie install jbboehr/php-yumemi:dev-develop
```

The Packagist package currently exposes development branches; the extension does not yet have a tagged release. PIE
can also build directly from a repository checkout:

```console
pie install
```

Use the PHP binary or `--with-php-config` option appropriate for the PHP installation that should load the extension.
The current PIE manifest admits PHP 8.2 through 8.5 on Linux only. This project targets PIE rather than PECL and does
not carry a `package.xml` manifest.

### Load it before yumemi.php

The extension must be loaded before Composer autoloads yumemi.php's `Quantity` class. A normal PIE installation enables
the module for its target PHP installation. For a temporary source-build test, pass it on the PHP command line:

```console
php -d extension=/path/to/yumemi.so application.php
```

Confirm the active PHP installation sees the module with:

```console
php --ri yumemi
```

Applications that enable operator syntax should also record the runtime dependency in their root Composer project:

```console
composer require 'ext-yumemi:*'
```

This does not install the extension; it makes Composer verify that the deployment PHP has it loaded. Reusable packages
should not require `ext-yumemi`, because yumemi.php's public methods and generated parser remain portable without it.

Once loaded, existing yumemi.php quantities gain operator syntax:

```php
<?php

require 'vendor/autoload.php';

use jbboehr\Yumemi\Units;

$units = Units::default();
$length = $units->quantity(1, 'meter');
$extra = $units->quantity(50, 'centimeter');

$total = $length + $extra;

echo $total->exactDecimalValueIn('meter'), ' ', $total->unitToString(), PHP_EOL;
// 1.5 meter
```

Reusable libraries should continue to prefer `add()`, `sub()`, `mul()`, `div()`, `pow()`, and `rdiv()`, because those
methods work whether or not an application installs the extension.

### Enable PHPStan's operator model

Runtime operator support and PHPStan support are separate opt-ins. If the application configures yumemi.php manually,
load the operator model after its primary PHPStan extension:

```neon
includes:
    - vendor/jbboehr/yumemi/extension.neon
    - vendor/jbboehr/yumemi/yumemi-operators.neon
```

When `phpstan/extension-installer` already loads `extension.neon`, include only `yumemi-operators.neon` yourself.
Enabling that file is the application's declaration that analyzed operator-bearing code runs with `ext-yumemi` loaded.
See yumemi.php's [Optional Quantity Operators][yumemi-operator-docs] documentation for the operand and result-type
contract.

## Operators

For descendants of `jbboehr\Yumemi\InternalQuantity`, the extension delegates:

| PHP syntax | Public method |
| --- | --- |
| `$quantity + $other` | `$quantity->add($other)` |
| `$quantity - $other` | `$quantity->sub($other)` |
| `$quantity * $other` | `$quantity->mul($other)` |
| `$quantity / $other` | `$quantity->div($other)` |
| `$quantity ** $power` | `$quantity->pow($power)` |
| `$numerator / $quantity` | `$quantity->rdiv($numerator)` |
| `+$quantity` | `$quantity->mul(1)` |
| `-$quantity` | `$quantity->mul(-1)` |

The selected method receives the other operand unchanged, owns its validation, and supplies the result or exception.
Scalar multiplication works from either side. On the supported PHP versions, Zend lowers unary signs through
multiplication, so they follow the same `mul()` contract. Binary scalar-left `+`, `-`, and `**` remain unsupported.

PHP may reorder multiplication operands before invoking an internal object handler. Scalar multiplication is therefore
treated as commutative at the handler boundary. yumemi.php also defines quantity-by-quantity products independently of
which operand Zend presents as the receiver: accepted products are canonical and retain the shared registry context,
while rejected cross-context products expose the same failure from either order.

Comparison operators are intentionally not overloaded for now. These expressions are not rejected: PHP applies its
ordinary object-state comparison, whose result can disagree with quantity semantics even for equivalent values written
in different units. Zend exposes only one comparison callback shared by `==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>`, and
implicit consumers such as `sort()`, `min()`, and `max()`, so the extension cannot limit a replacement to explicit
operator syntax. Use yumemi.php's named methods such as `compareTo()`, `equals()`, and `lessThan()` instead. Strict
identity operators `===` and `!==` retain their ordinary PHP object-identity meaning and cannot be overloaded.

## Native parser selection

The currently compatible yumemi.php `develop` branch selects the native parser automatically. Applications do not call
`NativeLexer` or `NativeParser` directly. Its current adapter checks `ABI_VERSION` and `isCompatible()` separately, then
uses the generated PHP parser if either check fails. This extension now exposes `NativeParser::supports()` as the
preferred atomic check for a coordinated adapter update. The yumemi.php integration has not yet shipped in a tagged
release.

Set `YUMEMI_NATIVE_PARSER` to `0`, `false`, `off`, `no`, or an empty string in the process environment to force the PHP
parser without unloading the extension. Leave it unset, or set it to `1`, `true`, `on`, or `yes`, for automatic native
selection; values are case-insensitive, and any other explicit value fails closed to the PHP fallback. The fallback
parser remains authoritative and independently supported. Applications should upgrade yumemi.php and `ext-yumemi`
together while the integration remains experimental.

The array schema, structured failure metadata, Unicode gate, and ABI contract are documented in
[Native Parser ABI](docs/NATIVE_PARSER_ABI.md).

## Build from source

### Unix-like systems

A PHP development package, C compiler, Autoconf, and Make are required. Flex and Bison are not required for an ordinary
build because their generated C sources are committed.

```console
phpize
./configure --enable-yumemi
make -j4
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
```

Try the uninstalled module with the same PHP installation used to build it:

```console
php -d extension="$PWD/modules/yumemi.so" --ri yumemi
```

### Windows

`config.w32` defines the Windows extension build. The qualified matrix uses
[`php/php-windows-builder`](https://github.com/php/php-windows-builder) with PHP 8.2, 8.4, and 8.5 on x64; the exact
configuration is in [CI](.github/workflows/ci.yml). The project does not yet publish the precompiled DLL packages needed
for normal PIE installation on Windows, so this is currently a maintainer source-build path rather than an end-user
installation promise.

### Nix development

The flake provides packages, checks, and development shells for PHP 8.2 through 8.5. PHP 8.2 is the default:

```console
nix develop
phpize
./configure --enable-yumemi
make -j4
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test
```

Select another PHP version with an attribute such as `nix develop .#php85` or `nix build .#php85`. Run the complete
Linux build, PHPT, formatting, generated-source, sanitizer, ZTS/debug, and PIE packaging gate with:

```console
nix flake check --keep-going -L
```

Format supported files with `nix fmt`. With direnv installed, the checked-in `.envrc` enters the default shell
automatically.

## Maintainer documentation

- [Architecture](docs/ARCHITECTURE.md) defines the extension/library boundary and operator handler contract.
- [Native Parser ABI](docs/NATIVE_PARSER_ABI.md) records the internal neutral-AST and failure interface.
- [Development](docs/DEVELOPMENT.md) covers generated sources, verification, CI, tests, and repository layout.
- [Release and compatibility policy](docs/RELEASE.md) defines the initial support envelope and release procedure.
- [Cross-repository handoff](docs/HANDOFF.md) records current yumemi.php integration and release-qualification state.
- [Changelog](CHANGELOG.md) summarizes release-facing changes.

## License

Project-authored code is licensed under `AGPL-3.0-only WITH romic-exception`. The native scanner and grammar contain
portions derived from UDUNITS2 under the UCAR license, so the package expression is
`(AGPL-3.0-only WITH romic-exception) AND UCAR`.

See [LICENSE.md](LICENSE.md), [docs/LICENSE_EXCEPTION.md](docs/LICENSE_EXCEPTION.md), and
[docs/UDUNITS-COPYRIGHT](docs/UDUNITS-COPYRIGHT) for the complete terms and notices.

[yumemi-operator-docs]: https://github.com/jbboehr/yumemi.php/blob/develop/docs/pages/reference/phpstan.md#optional-quantity-operators
