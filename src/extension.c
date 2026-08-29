/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main/php.h"

#include "Zend/zend_modules.h"
#include "ext/standard/info.h"

#include "../php_yumemi.h"
#include "internal_quantity.h"
#include "parser/native_lexer.h"
#include "parser/native_parser.h"

#if PHP_VERSION_ID < 80200
#error php-yumemi requires PHP 8.2 or newer
#endif

static PHP_MINIT_FUNCTION(yumemi)
{
    if (yumemi_register_internal_quantity() == FAILURE) {
        return FAILURE;
    }

    if (yumemi_register_native_lexer() == FAILURE) {
        return FAILURE;
    }

    return yumemi_register_native_parser();
}

static PHP_RINIT_FUNCTION(yumemi)
{
#if defined(ZTS) && defined(COMPILE_DL_YUMEMI)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}

static PHP_MINFO_FUNCTION(yumemi)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "yumemi extension", "enabled");
    php_info_print_table_row(2, "Version", PHP_YUMEMI_VERSION);
    php_info_print_table_row(2, "Authors", PHP_YUMEMI_AUTHORS);
    php_info_print_table_end();
}

zend_module_entry yumemi_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_YUMEMI_NAME,
    NULL,
    PHP_MINIT(yumemi),
    NULL,
    PHP_RINIT(yumemi),
    NULL,
    PHP_MINFO(yumemi),
    PHP_YUMEMI_VERSION,
    STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_YUMEMI
#if defined(ZTS)
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(yumemi)
#endif
