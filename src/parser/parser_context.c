/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "main/php.h"
#include "Zend/zend_smart_str.h"

#include "native_parser.h"

static void *yumemi_parser_arena_alloc(yumemi_parse_context *context, size_t size)
{
    return zend_arena_alloc(&context->arena, size);
}

static char *yumemi_parser_arena_string(yumemi_parse_context *context, const char *text, size_t length)
{
    char *copy = yumemi_parser_arena_alloc(context, length + 1);

    memcpy(copy, text, length);
    copy[length] = '\0';

    return copy;
}

void yumemi_parse_context_init(yumemi_parse_context *context)
{
    memset(context, 0, sizeof(*context));
    context->arena = zend_arena_create(4096);
}

void yumemi_parse_context_destroy(yumemi_parse_context *context)
{
    zend_arena_destroy(context->arena);
    memset(context, 0, sizeof(*context));
}

void yumemi_parse_context_set_error(yumemi_parse_context *context,
                                    const yumemi_lexer_location *location,
                                    const char *message)
{
    if (context->error_message != NULL) {
        return;
    }

    context->error_location = *location;
    context->error_message = yumemi_parser_arena_string(context, message, strlen(message));
}

static char *yumemi_parser_format_syntax_error(yumemi_parse_context *context,
                                               const char *unexpected_token,
                                               const char *const *expected_tokens,
                                               size_t expected_token_count)
{
    smart_str buffer = { 0 };
    size_t index;
    char *message;

    smart_str_appends(&buffer, "syntax error");
    if (unexpected_token != NULL) {
        smart_str_appends(&buffer, ", unexpected ");
        smart_str_appends(&buffer, unexpected_token);
        /* Preserve Bison's prior detailed-message behavior: one of its five arguments was the unexpected token. */
        if (expected_token_count > 0 && expected_token_count <= 4) {
            smart_str_appends(&buffer, ", expecting ");
            for (index = 0; index < expected_token_count; ++index) {
                if (index > 0) {
                    smart_str_appends(&buffer, " or ");
                }
                smart_str_appends(&buffer, expected_tokens[index]);
            }
        }
    }

    message = yumemi_parser_arena_string(context, ZSTR_VAL(buffer.s), ZSTR_LEN(buffer.s));
    smart_str_free(&buffer);

    return message;
}

void yumemi_parse_context_set_syntax_error(yumemi_parse_context *context,
                                           const yumemi_lexer_location *location,
                                           const char *unexpected_token,
                                           const char *const *expected_tokens,
                                           size_t expected_token_count)
{
    size_t index;

    if (context->error_message != NULL) {
        return;
    }

    context->error_location = *location;
    context->error_message =
        yumemi_parser_format_syntax_error(context, unexpected_token, expected_tokens, expected_token_count);
    if (unexpected_token != NULL) {
        context->unexpected_token = yumemi_parser_arena_string(context, unexpected_token, strlen(unexpected_token));
    }
    if (expected_token_count == 0) {
        return;
    }

    context->expected_tokens = yumemi_parser_arena_alloc(context, expected_token_count * sizeof(const char *));
    context->expected_token_count = expected_token_count;
    for (index = 0; index < expected_token_count; ++index) {
        context->expected_tokens[index] =
            yumemi_parser_arena_string(context, expected_tokens[index], strlen(expected_tokens[index]));
    }
}

static yumemi_ast_node *yumemi_ast_make_owned_leaf(yumemi_parse_context *context,
                                                   yumemi_ast_kind kind,
                                                   const char *text,
                                                   size_t length,
                                                   const yumemi_lexer_location *location)
{
    yumemi_ast_node *node = yumemi_parser_arena_alloc(context, sizeof(*node));

    node->kind = kind;
    node->has_location = location != NULL;
    node->location = location != NULL ? *location : (yumemi_lexer_location){ 0, 0 };
    node->value.leaf.text = text;
    node->value.leaf.length = length;

    return node;
}

yumemi_ast_node *yumemi_ast_make_leaf(yumemi_parse_context *context,
                                      yumemi_ast_kind kind,
                                      const char *text,
                                      size_t length,
                                      const yumemi_lexer_location *location)
{
    return yumemi_ast_make_owned_leaf(
        context, kind, yumemi_parser_arena_string(context, text, length), length, location);
}

yumemi_ast_node *yumemi_ast_make_binary(yumemi_parse_context *context,
                                        yumemi_ast_kind kind,
                                        yumemi_ast_node *left,
                                        yumemi_ast_node *right,
                                        const yumemi_lexer_location *location)
{
    yumemi_ast_node *node = yumemi_parser_arena_alloc(context, sizeof(*node));

    node->kind = kind;
    node->has_location = location != NULL;
    node->location = location != NULL ? *location : (yumemi_lexer_location){ 0, 0 };
    node->value.binary.left = left;
    node->value.binary.right = right;

    return node;
}

static char yumemi_superscript_to_ascii(const unsigned char *text, size_t length, size_t *width)
{
    if (length >= 2 && text[0] == 0xc2 && text[1] >= 0xb2 && text[1] <= 0xb3) {
        *width = 2;
        return (char)('2' + (text[1] - 0xb2));
    }
    if (length >= 2 && text[0] == 0xc2 && text[1] == 0xb9) {
        *width = 2;
        return '1';
    }
    if (length >= 3 && text[0] == 0xe2 && text[1] == 0x81) {
        *width = 3;
        if (text[2] == 0xb0) {
            return '0';
        }
        if (text[2] >= 0xb4 && text[2] <= 0xb9) {
            return (char)('4' + (text[2] - 0xb4));
        }
        if (text[2] == 0xba) {
            return '+';
        }
        if (text[2] == 0xbb) {
            return '-';
        }
    }

    *width = 1;
    return '?';
}

yumemi_ast_node *yumemi_ast_make_superscript_integer(yumemi_parse_context *context,
                                                     const char *text,
                                                     size_t length,
                                                     const yumemi_lexer_location *location)
{
    char *ascii = yumemi_parser_arena_alloc(context, length + 1);
    size_t input_offset = 0;
    size_t output_length = 0;

    while (input_offset < length) {
        size_t width;

        ascii[output_length++] =
            yumemi_superscript_to_ascii((const unsigned char *)text + input_offset, length - input_offset, &width);
        input_offset += width;
    }
    ascii[output_length] = '\0';

    return yumemi_ast_make_owned_leaf(context, YUMEMI_AST_INTEGER, ascii, output_length, location);
}

yumemi_ast_node *yumemi_ast_make_negation(yumemi_parse_context *context,
                                          yumemi_ast_node *node,
                                          const yumemi_lexer_location *location)
{
    if (node->kind == YUMEMI_AST_INTEGER || node->kind == YUMEMI_AST_FLOAT) {
        const char *text = node->value.leaf.text;
        size_t length = node->value.leaf.length;

        if (length > 0 && text[0] == '-') {
            return yumemi_ast_make_owned_leaf(context, node->kind, text + 1, length - 1, location);
        }

        char *negative = yumemi_parser_arena_alloc(context, length + 2);
        negative[0] = '-';
        memcpy(negative + 1, text, length);
        negative[length + 1] = '\0';

        return yumemi_ast_make_owned_leaf(context, node->kind, negative, length + 1, location);
    }

    return yumemi_ast_make_binary(
        context, YUMEMI_AST_MUL, yumemi_ast_make_leaf(context, YUMEMI_AST_INTEGER, "-1", 2, NULL), node, location);
}
