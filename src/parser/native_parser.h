/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifndef PHP_YUMEMI_NATIVE_PARSER_H
#define PHP_YUMEMI_NATIVE_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "main/php.h"

#include "native_lexer.h"

#define YUMEMI_NATIVE_PARSER_ABI_VERSION 1

typedef enum
{
    YUMEMI_AST_INTEGER = 0,
    YUMEMI_AST_FLOAT,
    YUMEMI_AST_IDENTIFIER,
    YUMEMI_AST_ADD,
    YUMEMI_AST_SUB,
    YUMEMI_AST_MUL,
    YUMEMI_AST_DIV,
    YUMEMI_AST_POW,
    YUMEMI_AST_AT,
} yumemi_ast_kind;

typedef struct yumemi_ast_node
{
    yumemi_ast_kind kind;
    bool has_location;
    yumemi_lexer_location location;
    union
    {
        struct
        {
            const char *text;
            size_t length;
        } leaf;
        struct
        {
            struct yumemi_ast_node *left;
            struct yumemi_ast_node *right;
        } binary;
    } value;
} yumemi_ast_node;

struct yumemi_parser_allocation;

typedef struct
{
    struct yumemi_parser_allocation *allocations;
    yumemi_ast_node *root;
    bool has_error;
    yumemi_lexer_location error_location;
    const char *error_message;
} yumemi_parse_context;

void yumemi_parse_context_init(yumemi_parse_context *context);
void yumemi_parse_context_destroy(yumemi_parse_context *context);
void yumemi_parse_context_set_error(yumemi_parse_context *context,
                                    const yumemi_lexer_location *location,
                                    const char *message);

yumemi_ast_node *yumemi_ast_make_leaf(yumemi_parse_context *context,
                                      yumemi_ast_kind kind,
                                      const char *text,
                                      size_t length,
                                      const yumemi_lexer_location *location);
yumemi_ast_node *yumemi_ast_make_superscript_integer(yumemi_parse_context *context,
                                                     const char *text,
                                                     size_t length,
                                                     const yumemi_lexer_location *location);
yumemi_ast_node *yumemi_ast_make_binary(yumemi_parse_context *context,
                                        yumemi_ast_kind kind,
                                        yumemi_ast_node *left,
                                        yumemi_ast_node *right,
                                        const yumemi_lexer_location *location);
yumemi_ast_node *yumemi_ast_make_negation(yumemi_parse_context *context,
                                          yumemi_ast_node *node,
                                          const yumemi_lexer_location *location);

zend_result yumemi_register_native_parser(void);

#endif /* PHP_YUMEMI_NATIVE_PARSER_H */
