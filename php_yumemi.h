/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifndef PHP_YUMEMI_H
#define PHP_YUMEMI_H

#include "main/php.h"

#define PHP_YUMEMI_NAME "yumemi"
#define PHP_YUMEMI_VERSION "0.1.0"
#define PHP_YUMEMI_AUTHORS "John Boehr <jbboehr@gmail.com> (lead)"

extern zend_module_entry yumemi_module_entry;
#define phpext_yumemi_ptr &yumemi_module_entry

#if defined(ZTS) && defined(COMPILE_DL_YUMEMI)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#endif /* PHP_YUMEMI_H */
