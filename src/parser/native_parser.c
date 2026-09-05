/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main/php.h"
#include "Zend/zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"

#include "native_parser.h"
#include "parser.h"
#include "scanner.h"

static zend_class_entry *yumemi_native_parse_exception_class;

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
