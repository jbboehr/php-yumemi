# php-yumemi

Experimental native PHP extension for [yumemi.php](https://github.com/jbboehr/yumemi.php).

The extension adds two optional capabilities to the pure-PHP library:

- arithmetic operators for `Quantity` objects, forwarded to the existing public methods.
- a native unit-expression parser, with automatic fallback to yumemi.php's generated PHP parser.

yumemi.php still handles arithmetic, conversion, registries, and result construction. The extension does not contain a
second unit engine, and portable library code does not have to depend on it.

The integration has not yet shipped in a tagged yumemi.php release. Until it does, the installation below uses the
`develop` branch, which contains the internal quantity base and native-parser adapter.

## Status and platforms

This is an early experiment, not a supported release.

The [release and compatibility policy](docs/RELEASE.md) lists the platforms planned for the first release, the internal
interfaces shared with yumemi.php, and the checks required before tagging.

| Environment | Source-build CI coverage | Distribution status |
| --- | --- | --- |
| x86_64 Linux | PHP 8.2–8.5 NTS, PHP 8.2/8.5 ZTS+debug, PHP 8.5 ASan/UBSan | Published PIE package target |
| Intel macOS | PHP 8.2 | Source-build qualification only |
| Apple Silicon macOS | PHP 8.5 | Source-build qualification only |
| x64 Windows | PHP 8.2, 8.4, and 8.5 NTS, plus PHP 8.4 TS | Source-build qualification only, no DLL release yet |

The Composer manifest limits PIE installation to Linux NTS and ZTS builds on PHP 8.2 through 8.5. The macOS and Windows
jobs test source builds, but the project does not yet publish packages for those platforms.

## Install and use

Install the yumemi.php library in the application first:

```console
composer require jbboehr/yumemi:dev-develop
```

Tagged yumemi.php v0.1.1 predates `InternalQuantity`, the parser adapter, and `yumemi-operators.neon`. It cannot provide
the integration documented here.

### Install the extension with PIE

The extension is published on Packagist as
[`jbboehr/php-yumemi`](https://packagist.org/packages/jbboehr/php-yumemi). On Linux, install it for the selected PHP
installation with [PIE](https://github.com/php/pie):

```console
pie install jbboehr/php-yumemi:dev-develop
```

Packagist currently has development branches but no tagged release. PIE can also build from a repository checkout:

```console
pie install
```

Use the PHP binary for the installation that will load the extension, or select it with `--with-php-config`. The PIE
manifest accepts PHP 8.2 through 8.5 on Linux. This project targets PIE and does not include a PECL `package.xml`.

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

This command does not install the extension. It tells Composer to verify that the deployment PHP loads it. Reusable
packages should not require `ext-yumemi`, because yumemi.php's methods and generated parser work without it.

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

Runtime operators and PHPStan's operator model are configured separately. If the application configures yumemi.php
manually, load the operator model after the main PHPStan extension:

```neon
includes:
    - vendor/jbboehr/yumemi/extension.neon
    - vendor/jbboehr/yumemi/yumemi-operators.neon
```

When `phpstan/extension-installer` already loads `extension.neon`, include only `yumemi-operators.neon` yourself.
Including that file tells PHPStan that the analyzed application loads `ext-yumemi`. See yumemi.php's
[Optional Quantity Operators][yumemi-operator-docs] documentation for accepted operands and inferred result types.

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

The method receives the other operand unchanged. It validates that operand and either returns a result or throws.
Scalar multiplication works from either side. On the supported PHP versions, Zend lowers unary signs through
multiplication, so they follow the same `mul()` contract. Binary scalar-left `+`, `-`, and `**` remain unsupported.

PHP may reorder multiplication operands before invoking an internal object handler. The handler treats scalar
multiplication as commutative. yumemi.php also defines quantity-by-quantity products independently of the receiver Zend
chooses. Accepted products are canonical and retain the shared registry context. Rejected cross-context products expose
the same failure from either order.

Comparison operators are not overloaded. PHP still accepts them and compares object state, which can disagree with
quantity semantics even when two values use equivalent units. Zend has one callback for `==`, `!=`, `<`, `<=`, `>`,
`>=`, `<=>`, and implicit comparisons in functions such as `sort()`, `min()`, and `max()`. The extension cannot replace
explicit operator syntax without changing those functions too. Use yumemi.php methods such as `compareTo()`, `equals()`,
and `lessThan()` instead. The strict identity operators `===` and `!==` keep their PHP object-identity meaning and cannot
be overloaded.

## Native parser selection

The currently compatible yumemi.php `develop` branch selects the native parser automatically. Applications do not call
`NativeLexer` or `NativeParser` directly. Its adapter makes the ABI decision with `NativeParser::supports(1)`. If the
class is not already loaded, the method is missing, or the extension rejects ABI version 1, yumemi.php uses its
generated PHP parser. `ABI_VERSION` and `isCompatible()` remain available for older adapters. The yumemi.php integration
has not yet shipped in a tagged release.

Set `YUMEMI_NATIVE_PARSER` to `0`, `false`, `off`, `no`, or an empty string in the process environment to force the PHP
parser without unloading the extension. Leave it unset, or set it to `1`, `true`, `on`, or `yes`, for automatic native
selection. Values are case-insensitive. Any other value selects the PHP fallback. The generated parser continues to
work on its own. Applications should upgrade yumemi.php and `ext-yumemi` together while the integration is experimental.

The array schema, structured failure metadata, committed Unicode snapshot, and ABI contract are documented in
[Native Parser ABI](docs/NATIVE_PARSER_ABI.md).

## Build from source

### Unix-like systems

A PHP development package, C compiler, Autoconf, and Make are required. Flex and Bison are not required for a normal
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
[`php/php-windows-builder`](https://github.com/php/php-windows-builder) with PHP 8.2, 8.4, and 8.5 on x64. The exact
configuration is in [CI](.github/workflows/ci.yml). The project does not yet publish precompiled DLLs, so Windows is a
maintainer source-build target rather than a PIE installation target.

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
