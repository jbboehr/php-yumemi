# php-yumemi

Experimental native PHP extension companion to
[yumemi.php](https://github.com/jbboehr/yumemi.php).

The module registers `jbboehr\Yumemi\InternalQuantity`, an abstract base class whose object handler delegates arithmetic
operators to public methods implemented by userland descendants. Arithmetic semantics remain in yumemi.php; the
extension only admits operator syntax.

## Status

This is an early experiment, not a supported release. The initial build targets PHP 8.2 through 8.5 on non-thread-safe
builds. Packaging, Windows support, and ZTS support remain unfinished or unverified.

## Operators

For descendants of `InternalQuantity`, the extension maps `+`, `-`, `*`, `/`, and `**` to `add()`, `sub()`, `mul()`,
`div()`, and `pow()` respectively. It passes the other operand to the selected method and returns the method's result
unchanged.

Multiplication is treated as commutative at the handler boundary, so both `$quantity * 2` and `2 * $quantity` delegate
to `$quantity->mul(2)`. PHP normalizes some `ZEND_MUL` expressions before invoking object handlers, making the original
operand order unavailable. Scalar-left division delegates `2 / $quantity` to `$quantity->rdiv(2)`. Scalar-left `+`,
`-`, and `**` remain unsupported.

The handler forwards the selected operand unchanged. The canonical yumemi.php `Quantity::rdiv(int|Rational)` signature
owns numerator validation, just as the other userland arithmetic methods own their operand rules.

## Build

A PHP development package, a C compiler, Autoconf, and Make are required.

```console
phpize
./configure --enable-yumemi
make -j4
make test
```

To try the uninstalled extension:

```console
php -d extension="$PWD/modules/yumemi.so" -r 'var_dump(extension_loaded("yumemi"));'
```

### Nix

The flake provides packages, checks, and development shells for PHP 8.2 through 8.5. PHP 8.2 is the default:

```console
nix develop
phpize
./configure --enable-yumemi
make -j4
make test
```

Select another supported PHP version by attribute, for example `nix develop .#php85` or `nix build .#php85`. Run the
PHPT suite against every supported PHP version and verify Nix formatting with `nix flake check`. Format Nix files with
`nix fmt`. With direnv installed, the checked-in `.envrc` enters the default shell automatically.

## Layout

- `config.m4` defines the `phpize` build.
- `nix/derivation.nix` packages the extension for the flake's supported PHP versions.
- `php_yumemi.h` contains module metadata.
- `src/extension.c` registers the module and its `phpinfo()` output.
- `src/internal_quantity.c` registers the userland seam and operator handler.
- `tests/` contains PHPT behavior tests.

## License

php-yumemi is licensed under `AGPL-3.0-only WITH romic-exception`. See [LICENSE.md](LICENSE.md) and
[docs/LICENSE_EXCEPTION.md](docs/LICENSE_EXCEPTION.md).
