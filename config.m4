PHP_ARG_ENABLE([yumemi], [whether to enable yumemi],
    [AS_HELP_STRING([--enable-yumemi], [Enable the yumemi extension])])

if test "$PHP_YUMEMI" != "no"; then
    PHP_ADD_BUILD_DIR([src])
    PHP_ADD_BUILD_DIR([src/parser])
    PHP_NEW_EXTENSION([yumemi], [src/extension.c src/internal_quantity.c src/parser/native_lexer.c src/parser/native_parser.c src/parser/parser.c src/parser/scanner.c], [$ext_shared],,
        [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1])
    PHP_ADD_MAKEFILE_FRAGMENT
fi
