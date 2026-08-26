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

## Layout

- `config.m4` defines the `phpize` build.
- `php_yumemi.h` contains module metadata.
- `src/extension.c` registers the module and its `phpinfo()` output.
- `tests/` contains PHPT smoke tests.

## License

php-yumemi is licensed under `AGPL-3.0-only WITH romic-exception`. See [LICENSE.md](LICENSE.md) and
[docs/LICENSE_EXCEPTION.md](docs/LICENSE_EXCEPTION.md).
