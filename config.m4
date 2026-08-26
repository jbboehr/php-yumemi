PHP_ARG_ENABLE([yumemi], [whether to enable yumemi],
    [AS_HELP_STRING([--enable-yumemi], [Enable the yumemi extension])])

if test "$PHP_YUMEMI" != "no"; then
    PHP_ADD_BUILD_DIR([src])
    PHP_NEW_EXTENSION([yumemi], [src/extension.c], [$ext_shared],,
        [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1])
fi
