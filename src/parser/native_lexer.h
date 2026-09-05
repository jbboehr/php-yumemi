/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifndef PHP_YUMEMI_NATIVE_LEXER_H
#define PHP_YUMEMI_NATIVE_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "main/php.h"

#include "parser.h"

#define YUMEMI_LEXER_INPUT_BYTES_LIMIT 4096
#define YUMEMI_LEXER_TOKEN_COUNT_LIMIT 256
#define YUMEMI_LEXER_NESTING_DEPTH_LIMIT 64
#define YUMEMI_LEXER_TOKEN_BYTES_LIMIT 1024

typedef yumemi_parser_token_kind_t yumemi_token_type;

typedef enum
{
    YUMEMI_LEXER_LIMIT_NONE = 0,
    YUMEMI_LEXER_LIMIT_INPUT_BYTES,
    YUMEMI_LEXER_LIMIT_TOKEN_COUNT,
    YUMEMI_LEXER_LIMIT_NESTING_DEPTH,
    YUMEMI_LEXER_LIMIT_TOKEN_BYTES,
} yumemi_lexer_limit;

typedef struct
{
    yumemi_lexer_limit category;
    size_t limit;
    size_t observed;
    size_t start;
    size_t end;
} yumemi_lexer_error;

typedef struct
{
    size_t offset;
    size_t token_count;
    size_t nesting_depth;
    bool invalid_utf8;
    yumemi_lexer_error error;
} yumemi_lexer_context;

#ifndef YYSTYPE
#define YYSTYPE yumemi_lexer_value
#endif

#ifndef YYLTYPE
#define YYLTYPE yumemi_lexer_location
#endif

bool yumemi_lexer_accept_token(yumemi_lexer_context *context, yumemi_token_type type, size_t start, size_t end);
void yumemi_lexer_context_init(yumemi_lexer_context *context, const unsigned char *input, size_t length);
size_t yumemi_lexer_classify_number(const unsigned char *text, size_t length, yumemi_token_type *type);
size_t yumemi_lexer_classify_unicode_chunk(const unsigned char *text, size_t length, yumemi_token_type *type);
const char *yumemi_lexer_token_name(yumemi_token_type type);
const char *yumemi_lexer_limit_name(yumemi_lexer_limit limit);
void yumemi_declare_readonly_property(zend_class_entry *class_entry,
                                      const char *name,
                                      size_t name_length,
                                      uint32_t type_mask);
void yumemi_lexer_throw_limit(const yumemi_lexer_error *error);

zend_result yumemi_register_native_lexer(void);

#endif /* PHP_YUMEMI_NATIVE_LEXER_H */
