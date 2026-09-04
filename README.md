![php-yumemi — operator overloading for yumemi.php](docs/assets/banner.png)

# php-yumemi

[![Build](https://github.com/jbboehr/php-yumemi/actions/workflows/ci.yml/badge.svg)](https://github.com/jbboehr/php-yumemi/actions/workflows/ci.yml)
[![Built with Nix](https://img.shields.io/badge/built%20with-Nix-5277C3?logo=nixos&logoColor=white)](flake.nix)
[![License: AGPL-3.0-only WITH romic-exception](https://img.shields.io/badge/license-AGPL--3.0--only%20WITH%20romic--exception-blue.svg)](LICENSE.md)
[![AI burn](https://img.shields.io/endpoint?url=https%3A%2F%2Fgist.githubusercontent.com%2Fjbboehr%2F82f2ceb23f4d50a491e05da7da08317e%2Fraw%2Fagent-badge.json&cacheSeconds=300)](https://github.com/arlegotin/agent-badge)

php-yumemi is an experimental native PHP extension for
[yumemi.php](https://github.com/jbboehr/yumemi.php). It adds arithmetic operator syntax to `Quantity` objects and an
optional native unit-expression parser. yumemi.php continues to work without the extension.

## Requirements

- PHP 8.2 through 8.5.
- Linux for installation through PIE. macOS and Windows are currently source-build targets, and no precompiled Windows
  DLLs are published.

This project is an early experiment and does not have a stable release.

## Installation

Install yumemi.php first:

```console
composer require jbboehr/yumemi
```

Install the extension with [PIE](https://github.com/php/pie):

```console
pie install jbboehr/php-yumemi
```

If more than one PHP installation is present, run PIE with the PHP binary or `php-config` for the installation that
will load the extension.

Applications that depend on operator syntax should also record the runtime requirement:

```console
composer require 'ext-yumemi:*'
```

Reusable packages should not require `ext-yumemi`, because consumers can use yumemi.php without the extension.

The extension must load before Composer autoloads yumemi.php. Confirm that the active PHP installation sees it:

```console
php --ri yumemi
```

## Using quantity operators

Existing yumemi.php quantities support arithmetic operators after the extension is loaded:

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

The operators follow the corresponding public yumemi.php methods:

| PHP syntax | Method |
| --- | --- |
| `$quantity + $other` | `$quantity->add($other)` |
| `$quantity - $other` | `$quantity->sub($other)` |
| `$quantity * $other` | `$quantity->mul($other)` |
| `$quantity / $other` | `$quantity->div($other)` |
| `$quantity ** $power` | `$quantity->pow($power)` |
| `$numerator / $quantity` | `$quantity->rdiv($numerator)` |
| `+$quantity` | `$quantity->mul(1)` |
| `-$quantity` | `$quantity->mul(-1)` |

Scalar multiplication works from either side. Scalar-left `+`, `-`, and `**` are not supported.

Comparison operators are not overloaded. Use yumemi.php methods such as `compareTo()`, `equals()`, and `lessThan()` for
quantity comparisons. The `===` and `!==` operators retain their normal PHP object-identity meaning.

Library code can call `add()`, `sub()`, `mul()`, `div()`, `pow()`, and `rdiv()` directly when it needs to work with or
without the extension.

## PHPStan

Runtime operators and PHPStan support are configured separately. If the application loads yumemi.php's PHPStan
extension manually, include the operator model after it:

```neon
includes:
    - vendor/jbboehr/yumemi/extension.neon
    - vendor/jbboehr/yumemi/yumemi-operators.neon
```

When `phpstan/extension-installer` already loads `extension.neon`, include only `yumemi-operators.neon`. See
[Optional Quantity Operators][yumemi-operator-docs] for supported operands and inferred result types.

## Native parser

yumemi.php uses the native parser automatically when the extension is loaded and falls back to its PHP parser when the
native parser is unavailable.

Set `YUMEMI_NATIVE_PARSER=0` in the process environment to force the PHP parser without unloading the extension. The
values `false`, `off`, `no`, and an empty string have the same effect. Leave the variable unset for automatic selection.

Upgrade yumemi.php and `ext-yumemi` together while the native integration is experimental.

## Building from source

A PHP development package, C compiler, Autoconf, and Make are required. The generated lexer and parser sources are
included, so a normal build does not require Flex or Bison.

```console
phpize
./configure --enable-yumemi
make -j4
make test
make install
```

After installation, add `extension=yumemi` to the appropriate `php.ini` and verify it with `php --ri yumemi`.

## License

Project-authored code is licensed under `AGPL-3.0-only WITH romic-exception`. The native scanner and grammar contain
portions derived from UDUNITS2 under the UCAR license.

See [LICENSE.md](LICENSE.md), [docs/LICENSE_EXCEPTION.md](docs/LICENSE_EXCEPTION.md), and
[docs/UDUNITS-COPYRIGHT](docs/UDUNITS-COPYRIGHT) for the complete terms and notices.

[yumemi-operator-docs]: https://github.com/jbboehr/yumemi.php/blob/develop/docs/pages/reference/phpstan.md#optional-quantity-operators
