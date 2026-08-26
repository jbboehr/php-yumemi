# php-yumemi

Scaffold for an experimental native PHP extension companion to
[yumemi.php](https://github.com/jbboehr/yumemi.php).

This repository currently establishes the extension's build, test, formatting, and licensing conventions. The module
loads as `yumemi` and reports its development version, but it does not yet expose userland classes, functions, or
constants.

## Status

This is an early scaffold, not a supported release. The initial build targets PHP 8.2 through 8.5 on non-thread-safe
builds. It does not yet integrate with yumemi.php, publish packages, or claim Windows support.

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
- `tests/` contains PHPT smoke tests.

## License

php-yumemi is licensed under `AGPL-3.0-only WITH romic-exception`. See [LICENSE.md](LICENSE.md) and
[docs/LICENSE_EXCEPTION.md](docs/LICENSE_EXCEPTION.md).
