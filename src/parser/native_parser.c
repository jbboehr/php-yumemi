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
#include "Zend/zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"

#include "native_parser.h"
#include "parser.h"
#include "scanner.h"

typedef struct yumemi_parser_allocation
{
    struct yumemi_parser_allocation *next;
} yumemi_parser_allocation;

static zend_class_entry *yumemi_native_parse_exception_class;

static void *yumemi_parser_arena_alloc(yumemi_parse_context *context, size_t size)
{
    yumemi_parser_allocation *allocation = emalloc(sizeof(*allocation) + size);

    allocation->next = context->allocations;
    context->allocations = allocation;

    return allocation + 1;
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
}

void yumemi_parse_context_destroy(yumemi_parse_context *context)
{
    yumemi_parser_allocation *allocation = context->allocations;

    while (allocation != NULL) {
        yumemi_parser_allocation *next = allocation->next;

        efree(allocation);
        allocation = next;
    }

    memset(context, 0, sizeof(*context));
}

void yumemi_parse_context_set_error(yumemi_parse_context *context,
                                    const yumemi_lexer_location *location,
                                    const char *message)
{
    if (context->has_error) {
        return;
    }

    context->has_error = true;
    context->error_location = *location;
    context->error_message = yumemi_parser_arena_string(context, message, strlen(message));
}

static char *yumemi_parser_format_syntax_error(yumemi_parse_context *context,
                                               const char *unexpected_token,
                                               const char *const *expected_tokens,
                                               size_t expected_token_count)
{
    static const char syntax_error[] = "syntax error";
    static const char unexpected_prefix[] = ", unexpected ";
    static const char expecting_prefix[] = ", expecting ";
    static const char alternative_separator[] = " or ";
    /* Preserve Bison's prior detailed-message behavior: one of its five arguments was the unexpected token. */
    bool include_expected = expected_token_count <= 4;
    size_t length = sizeof(syntax_error) - 1;
    size_t index;
    char *message;
    char *cursor;

    if (unexpected_token == NULL) {
        return yumemi_parser_arena_string(context, syntax_error, length);
    }

    length += sizeof(unexpected_prefix) - 1 + strlen(unexpected_token);
    if (include_expected && expected_token_count > 0) {
        length += sizeof(expecting_prefix) - 1;
        for (index = 0; index < expected_token_count; ++index) {
            if (index > 0) {
                length += sizeof(alternative_separator) - 1;
            }
            length += strlen(expected_tokens[index]);
        }
    }

    message = yumemi_parser_arena_alloc(context, length + 1);
    cursor = message;
#define YUMEMI_APPEND_LITERAL(literal)                                                                \
    do {                                                                                              \
        memcpy(cursor, (literal), sizeof(literal) - 1);                                               \
        cursor += sizeof(literal) - 1;                                                                \
    } while (0)
    YUMEMI_APPEND_LITERAL(syntax_error);
    YUMEMI_APPEND_LITERAL(unexpected_prefix);
    memcpy(cursor, unexpected_token, strlen(unexpected_token));
    cursor += strlen(unexpected_token);
    if (include_expected && expected_token_count > 0) {
        YUMEMI_APPEND_LITERAL(expecting_prefix);
        for (index = 0; index < expected_token_count; ++index) {
            if (index > 0) {
                YUMEMI_APPEND_LITERAL(alternative_separator);
            }
            memcpy(cursor, expected_tokens[index], strlen(expected_tokens[index]));
            cursor += strlen(expected_tokens[index]);
        }
    }
#undef YUMEMI_APPEND_LITERAL
    *cursor = '\0';

    return message;
}

void yumemi_parse_context_set_syntax_error(yumemi_parse_context *context,
                                           const yumemi_lexer_location *location,
                                           const char *unexpected_token,
                                           const char *const *expected_tokens,
                                           size_t expected_token_count)
{
    size_t index;

    if (context->has_error) {
        return;
    }

    context->has_error = true;
    context->error_location = *location;
    context->error_message =
        yumemi_parser_format_syntax_error(context, unexpected_token, expected_tokens, expected_token_count);
    if (unexpected_token != NULL) {
        context->unexpected_token =
            yumemi_parser_arena_string(context, unexpected_token, strlen(unexpected_token));
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

yumemi_ast_node *yumemi_ast_make_leaf(yumemi_parse_context *context,
                                      yumemi_ast_kind kind,
                                      const char *text,
                                      size_t length,
                                      const yumemi_lexer_location *location)
{
    yumemi_ast_node *node = yumemi_parser_arena_alloc(context, sizeof(*node));

    node->kind = kind;
    node->has_location = location != NULL;
    node->location = location != NULL ? *location : (yumemi_lexer_location){ 0, 0 };
    node->value.leaf.text = yumemi_parser_arena_string(context, text, length);
    node->value.leaf.length = length;

    return node;
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

    return yumemi_ast_make_leaf(context, YUMEMI_AST_INTEGER, ascii, output_length, location);
}

yumemi_ast_node *yumemi_ast_make_negation(yumemi_parse_context *context,
                                          yumemi_ast_node *node,
                                          const yumemi_lexer_location *location)
{
    if (node->kind == YUMEMI_AST_INTEGER || node->kind == YUMEMI_AST_FLOAT) {
        const char *text = node->value.leaf.text;
        size_t length = node->value.leaf.length;

        if (length > 0 && text[0] == '-') {
            return yumemi_ast_make_leaf(context, node->kind, text + 1, length - 1, location);
        }

        char *negative = yumemi_parser_arena_alloc(context, length + 2);
        negative[0] = '-';
        memcpy(negative + 1, text, length);
        negative[length + 1] = '\0';

        return yumemi_ast_make_leaf(context, node->kind, negative, length + 1, location);
    }

    return yumemi_ast_make_binary(
        context, YUMEMI_AST_MUL, yumemi_ast_make_leaf(context, YUMEMI_AST_INTEGER, "-1", 2, NULL), node, location);
}

static const char *yumemi_ast_kind_name(yumemi_ast_kind kind)
{
    switch (kind) {
        case YUMEMI_AST_INTEGER:
            return "integer";
        case YUMEMI_AST_FLOAT:
            return "decimal-number";
        case YUMEMI_AST_IDENTIFIER:
            return "identifier";
        case YUMEMI_AST_ADD:
            return "add";
        case YUMEMI_AST_SUB:
            return "sub";
        case YUMEMI_AST_MUL:
            return "mul";
        case YUMEMI_AST_DIV:
            return "div";
        case YUMEMI_AST_POW:
            return "pow";
        case YUMEMI_AST_AT:
            return "at";
        default:
            return "unknown";
    }
}

static bool yumemi_ast_is_leaf(const yumemi_ast_node *node)
{
    return node->kind == YUMEMI_AST_INTEGER || node->kind == YUMEMI_AST_FLOAT || node->kind == YUMEMI_AST_IDENTIFIER;
}

static void yumemi_ast_to_zval(const yumemi_ast_node *node, zval *output)
{
    array_init_size(output, yumemi_ast_is_leaf(node) ? 4 : 5);
    add_assoc_string(output, "kind", (char *)yumemi_ast_kind_name(node->kind));
    if (node->has_location) {
        add_assoc_long(output, "start", (zend_long)node->location.start);
        add_assoc_long(output, "end", (zend_long)node->location.end);
    } else {
        add_assoc_null(output, "start");
        add_assoc_null(output, "end");
    }

    if (yumemi_ast_is_leaf(node)) {
        add_assoc_stringl(output, "text", node->value.leaf.text, node->value.leaf.length);
    } else {
        zval left;
        zval right;

        yumemi_ast_to_zval(node->value.binary.left, &left);
        yumemi_ast_to_zval(node->value.binary.right, &right);
        add_assoc_zval(output, "left", &left);
        add_assoc_zval(output, "right", &right);
    }
}

static void yumemi_native_parser_throw_syntax_error(zend_string *input,
                                                    const yumemi_parse_context *context,
                                                    size_t fallback_offset)
{
    const char *message = context->error_message != NULL ? context->error_message : "syntax error";
    size_t start = context->has_error ? context->error_location.start : fallback_offset;
    size_t end = context->has_error ? context->error_location.end : fallback_offset;
    zend_object *exception;
    zval expected;
    size_t index;

    zend_throw_exception_ex(yumemi_native_parse_exception_class, 0, "%s at bytes %zu..%zu", message, start, end);
    exception = EG(exception);
    if (exception == NULL) {
        return;
    }

    zend_update_property_str(yumemi_native_parse_exception_class, exception, ZEND_STRL("input"), input);
    zend_update_property_long(yumemi_native_parse_exception_class, exception, ZEND_STRL("start"), (zend_long)start);
    zend_update_property_long(yumemi_native_parse_exception_class, exception, ZEND_STRL("end"), (zend_long)end);
    if (context->unexpected_token != NULL) {
        zend_update_property_string(yumemi_native_parse_exception_class,
                                    exception,
                                    ZEND_STRL("unexpected"),
                                    context->unexpected_token);
    }

    array_init_size(&expected, context->expected_token_count);
    for (index = 0; index < context->expected_token_count; ++index) {
        add_next_index_string(&expected, context->expected_tokens[index]);
    }
    zend_update_property(yumemi_native_parse_exception_class, exception, ZEND_STRL("expected"), &expected);
    zval_ptr_dtor(&expected);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_native_parser_is_compatible, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(NativeParser, isCompatible)
{
    ZEND_PARSE_PARAMETERS_NONE();

    RETURN_BOOL(yumemi_lexer_is_compatible());
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_native_parser_parse, 0, 1, IS_ARRAY, 0)
ZEND_ARG_TYPE_INFO(0, input, IS_STRING, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(NativeParser, parse)
{
    zend_string *input;
    yumemi_lexer_context lexer_context = { 0 };
    yumemi_parse_context parse_context;
    yyscan_t scanner = NULL;
    YY_BUFFER_STATE buffer = NULL;
    int parse_result;

    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_STR(input)
    ZEND_PARSE_PARAMETERS_END();

    if (!yumemi_lexer_is_compatible()) {
        yumemi_lexer_throw_incompatible_pcre();
        RETURN_THROWS();
    }

    if (ZSTR_LEN(input) > YUMEMI_LEXER_INPUT_BYTES_LIMIT) {
        lexer_context.error = (yumemi_lexer_error){
            YUMEMI_LEXER_LIMIT_INPUT_BYTES, YUMEMI_LEXER_INPUT_BYTES_LIMIT, ZSTR_LEN(input), 0, ZSTR_LEN(input),
        };
        yumemi_lexer_throw_limit(&lexer_context.error);
        RETURN_THROWS();
    }

    yumemi_parse_context_init(&parse_context);
    yumemi_lexer_context_init(&lexer_context, (const unsigned char *)ZSTR_VAL(input), ZSTR_LEN(input));

    if (yumemi_lex_init_extra(&lexer_context, &scanner) != 0) {
        yumemi_parse_context_destroy(&parse_context);
        zend_throw_exception(spl_ce_RuntimeException, "Unable to initialize the Yumemi native parser lexer", 0);
        RETURN_THROWS();
    }

    buffer = yumemi__scan_bytes(ZSTR_VAL(input), (int)ZSTR_LEN(input), scanner);
    if (buffer == NULL) {
        yumemi_lex_destroy(scanner);
        yumemi_parse_context_destroy(&parse_context);
        zend_throw_exception(spl_ce_RuntimeException, "Unable to buffer input for the Yumemi native parser", 0);
        RETURN_THROWS();
    }

    parse_result = yumemi_parser_parse(scanner, &parse_context);

    yumemi__delete_buffer(buffer, scanner);
    yumemi_lex_destroy(scanner);

    if (lexer_context.error.category != YUMEMI_LEXER_LIMIT_NONE) {
        yumemi_lexer_throw_limit(&lexer_context.error);
        yumemi_parse_context_destroy(&parse_context);
        RETURN_THROWS();
    }

    if (parse_result != 0 || parse_context.root == NULL || parse_context.has_error) {
        yumemi_native_parser_throw_syntax_error(input, &parse_context, ZSTR_LEN(input));
        yumemi_parse_context_destroy(&parse_context);
        RETURN_THROWS();
    }

    yumemi_ast_to_zval(parse_context.root, return_value);
    yumemi_parse_context_destroy(&parse_context);
}

static const zend_function_entry yumemi_native_parser_methods[] = {
    PHP_ME(NativeParser, isCompatible, arginfo_native_parser_is_compatible, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        PHP_ME(NativeParser, parse, arginfo_native_parser_parse, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC) PHP_FE_END
};

zend_result yumemi_register_native_parser(void)
{
    zend_class_entry exception_entry;
    zend_class_entry parser_entry;
    zend_class_entry *native_parser_class;

    INIT_NS_CLASS_ENTRY(exception_entry, "jbboehr\\Yumemi\\Parser", "NativeParseException", NULL);
    yumemi_native_parse_exception_class = zend_register_internal_class_ex(&exception_entry, spl_ce_RuntimeException);
    yumemi_native_parse_exception_class->ce_flags |= ZEND_ACC_FINAL;
    zend_declare_property_null(yumemi_native_parse_exception_class, ZEND_STRL("input"), ZEND_ACC_PUBLIC);
    zend_declare_property_null(yumemi_native_parse_exception_class, ZEND_STRL("start"), ZEND_ACC_PUBLIC);
    zend_declare_property_null(yumemi_native_parse_exception_class, ZEND_STRL("end"), ZEND_ACC_PUBLIC);
    zend_declare_property_null(yumemi_native_parse_exception_class, ZEND_STRL("unexpected"), ZEND_ACC_PUBLIC);
    zend_declare_property_null(yumemi_native_parse_exception_class, ZEND_STRL("expected"), ZEND_ACC_PUBLIC);

    INIT_NS_CLASS_ENTRY(parser_entry, "jbboehr\\Yumemi\\Parser", "NativeParser", yumemi_native_parser_methods);
    native_parser_class = zend_register_internal_class(&parser_entry);
    native_parser_class->ce_flags |= ZEND_ACC_FINAL;
    zend_declare_class_constant_long(native_parser_class, ZEND_STRL("ABI_VERSION"), YUMEMI_NATIVE_PARSER_ABI_VERSION);

    return SUCCESS;
}
