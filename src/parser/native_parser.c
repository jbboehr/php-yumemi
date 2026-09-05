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
#include "Zend/zend_smart_str.h"
#include "ext/spl/spl_exceptions.h"

#include "native_parser.h"
#include "parser.h"
#include "scanner.h"

static zend_class_entry *yumemi_native_parse_exception_class;

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
    size_t start = context->error_message != NULL ? context->error_location.start : fallback_offset;
    size_t end = context->error_message != NULL ? context->error_location.end : fallback_offset;
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
        zend_update_property_string(
            yumemi_native_parse_exception_class, exception, ZEND_STRL("unexpected"), context->unexpected_token);
    } else {
        zend_update_property_null(yumemi_native_parse_exception_class, exception, ZEND_STRL("unexpected"));
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

    RETURN_TRUE;
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_native_parser_supports, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, abiVersion, IS_LONG, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(NativeParser, supports)
{
    zend_long abi_version;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(abi_version)
    ZEND_PARSE_PARAMETERS_END();

    RETURN_BOOL(abi_version == YUMEMI_NATIVE_PARSER_ABI_VERSION);
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

    yumemi_lexer_context_init(&lexer_context, (const unsigned char *)ZSTR_VAL(input), ZSTR_LEN(input));
    if (lexer_context.error.category != YUMEMI_LEXER_LIMIT_NONE) {
        yumemi_lexer_throw_limit(&lexer_context.error);
        RETURN_THROWS();
    }

    yumemi_parse_context_init(&parse_context);

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

    if (parse_result != 0 || parse_context.root == NULL || parse_context.error_message != NULL) {
        yumemi_native_parser_throw_syntax_error(input, &parse_context, ZSTR_LEN(input));
        yumemi_parse_context_destroy(&parse_context);
        RETURN_THROWS();
    }

    yumemi_ast_to_zval(parse_context.root, return_value);
    yumemi_parse_context_destroy(&parse_context);
}

/* PHP_ME includes the initializer comma; keep one entry per line. */
/* clang-format off */
static const zend_function_entry yumemi_native_parser_methods[] = {
    PHP_ME(NativeParser, isCompatible, arginfo_native_parser_is_compatible, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(NativeParser, supports, arginfo_native_parser_supports, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(NativeParser, parse, arginfo_native_parser_parse, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
/* clang-format on */

zend_result yumemi_register_native_parser(void)
{
    zend_class_entry exception_entry;
    zend_class_entry parser_entry;
    zend_class_entry *native_parser_class;

    INIT_NS_CLASS_ENTRY(exception_entry, "jbboehr\\Yumemi\\Parser", "NativeParseException", NULL);
    yumemi_native_parse_exception_class = zend_register_internal_class_ex(&exception_entry, spl_ce_RuntimeException);
    yumemi_native_parse_exception_class->ce_flags |= ZEND_ACC_FINAL;
    yumemi_declare_readonly_property(yumemi_native_parse_exception_class, ZEND_STRL("input"), MAY_BE_STRING);
    yumemi_declare_readonly_property(yumemi_native_parse_exception_class, ZEND_STRL("start"), MAY_BE_LONG);
    yumemi_declare_readonly_property(yumemi_native_parse_exception_class, ZEND_STRL("end"), MAY_BE_LONG);
    yumemi_declare_readonly_property(
        yumemi_native_parse_exception_class, ZEND_STRL("unexpected"), MAY_BE_STRING | MAY_BE_NULL);
    yumemi_declare_readonly_property(yumemi_native_parse_exception_class, ZEND_STRL("expected"), MAY_BE_ARRAY);

    INIT_NS_CLASS_ENTRY(parser_entry, "jbboehr\\Yumemi\\Parser", "NativeParser", yumemi_native_parser_methods);
    native_parser_class = zend_register_internal_class(&parser_entry);
    native_parser_class->ce_flags |= ZEND_ACC_FINAL;
    zend_declare_class_constant_long(native_parser_class, ZEND_STRL("ABI_VERSION"), YUMEMI_NATIVE_PARSER_ABI_VERSION);

    return SUCCESS;
}
