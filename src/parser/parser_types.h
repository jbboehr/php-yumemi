/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifndef PHP_YUMEMI_PARSER_TYPES_H
#define PHP_YUMEMI_PARSER_TYPES_H

#include <stddef.h>

struct yumemi_ast_node;
typedef struct yumemi_parse_context yumemi_parse_context;

typedef struct
{
    const char *text;
    size_t length;
    struct yumemi_ast_node *node;
} yumemi_lexer_value;

typedef struct
{
    size_t start;
    size_t end;
} yumemi_lexer_location;

#endif /* PHP_YUMEMI_PARSER_TYPES_H */
