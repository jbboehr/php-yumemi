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

#include "native_lexer.h"
#include "scanner.h"

static zend_class_entry *yumemi_native_limit_exception_class;

void yumemi_lexer_throw_limit(const yumemi_lexer_error *error)
{
    zend_object *exception;

    zend_throw_exception_ex(yumemi_native_limit_exception_class,
                            0,
                            "Yumemi parser %s limit exceeded: limit %zu, observed %zu at bytes %zu..%zu",
                            yumemi_lexer_limit_name(error->category),
                            error->limit,
                            error->observed,
                            error->start,
                            error->end);
    exception = EG(exception);
    if (exception == NULL) {
        return;
    }

    zend_update_property_string(
        yumemi_native_limit_exception_class, exception, ZEND_STRL("limit"), yumemi_lexer_limit_name(error->category));
    zend_update_property_long(
        yumemi_native_limit_exception_class, exception, ZEND_STRL("maximum"), (zend_long)error->limit);
    zend_update_property_long(
        yumemi_native_limit_exception_class, exception, ZEND_STRL("observed"), (zend_long)error->observed);
    zend_update_property_long(
        yumemi_native_limit_exception_class, exception, ZEND_STRL("start"), (zend_long)error->start);
    zend_update_property_long(yumemi_native_limit_exception_class, exception, ZEND_STRL("end"), (zend_long)error->end);
}

void yumemi_declare_readonly_property(zend_class_entry *class_entry,
                                      const char *name,
                                      size_t name_length,
                                      uint32_t type_mask)
{
    zval default_value;
    zend_string *property_name = zend_string_init(name, name_length, true);

    ZVAL_UNDEF(&default_value);
    zend_declare_typed_property(class_entry,
                                property_name,
                                &default_value,
                                ZEND_ACC_PUBLIC | ZEND_ACC_READONLY,
                                NULL,
                                (zend_type)ZEND_TYPE_INIT_MASK(type_mask));
    zend_string_release(property_name);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_native_lexer_is_compatible, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(NativeLexer, isCompatible)
{
    ZEND_PARSE_PARAMETERS_NONE();

    RETURN_TRUE;
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_native_lexer_tokenize, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, input, IS_STRING, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(NativeLexer, tokenize)
{
    zend_string *input;
    yumemi_lexer_context context = { 0 };
    yyscan_t scanner = NULL;
    YY_BUFFER_STATE buffer = NULL;
    yumemi_lexer_value value;
    yumemi_lexer_location location;
    int token;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(input)
    ZEND_PARSE_PARAMETERS_END();

    yumemi_lexer_context_init(&context, (const unsigned char *)ZSTR_VAL(input), ZSTR_LEN(input));
    if (context.error.category != YUMEMI_LEXER_LIMIT_NONE) {
        yumemi_lexer_throw_limit(&context.error);
        RETURN_THROWS();
    }

    if (yumemi_lex_init_extra(&context, &scanner) != 0) {
        zend_throw_exception(spl_ce_RuntimeException, "Unable to initialize the Yumemi native lexer", 0);
        RETURN_THROWS();
    }

    buffer = yumemi__scan_bytes(ZSTR_VAL(input), (int)ZSTR_LEN(input), scanner);
    if (buffer == NULL) {
        yumemi_lex_destroy(scanner);
        zend_throw_exception(spl_ce_RuntimeException, "Unable to buffer input for the Yumemi native lexer", 0);
        RETURN_THROWS();
    }

    array_init(return_value);

    while ((token = yumemi_lex(&value, &location, scanner)) != YUMEMI_PARSER_EOF && token != YUMEMI_PARSER_error) {
        zval token_data;

        array_init_size(&token_data, 4);
        add_assoc_string(&token_data, "type", (char *)yumemi_lexer_token_name((yumemi_token_type)token));
        add_assoc_stringl(&token_data, "text", value.text, value.length);
        add_assoc_long(&token_data, "start", (zend_long)location.start);
        add_assoc_long(&token_data, "end", (zend_long)location.end);
        add_next_index_zval(return_value, &token_data);
    }

    yumemi__delete_buffer(buffer, scanner);
    yumemi_lex_destroy(scanner);

    if (token == YUMEMI_PARSER_error) {
        yumemi_lexer_throw_limit(&context.error);
        RETURN_THROWS();
    }
}

/* PHP_ME includes the initializer comma; keep one entry per line. */
/* clang-format off */
static const zend_function_entry yumemi_native_lexer_methods[] = {
    PHP_ME(NativeLexer, isCompatible, arginfo_native_lexer_is_compatible, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(NativeLexer, tokenize, arginfo_native_lexer_tokenize, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
/* clang-format on */

zend_result yumemi_register_native_lexer(void)
{
    zend_class_entry exception_entry;
    zend_class_entry lexer_entry;
    zend_class_entry *native_lexer_class_entry;

    INIT_NS_CLASS_ENTRY(exception_entry, "jbboehr\\Yumemi\\Parser", "NativeLimitException", NULL);
    yumemi_native_limit_exception_class = zend_register_internal_class_ex(&exception_entry, spl_ce_LengthException);
    yumemi_native_limit_exception_class->ce_flags |= ZEND_ACC_FINAL;
    yumemi_declare_readonly_property(yumemi_native_limit_exception_class, ZEND_STRL("limit"), MAY_BE_STRING);
    yumemi_declare_readonly_property(yumemi_native_limit_exception_class, ZEND_STRL("maximum"), MAY_BE_LONG);
    yumemi_declare_readonly_property(yumemi_native_limit_exception_class, ZEND_STRL("observed"), MAY_BE_LONG);
    yumemi_declare_readonly_property(yumemi_native_limit_exception_class, ZEND_STRL("start"), MAY_BE_LONG);
    yumemi_declare_readonly_property(yumemi_native_limit_exception_class, ZEND_STRL("end"), MAY_BE_LONG);

    INIT_NS_CLASS_ENTRY(lexer_entry, "jbboehr\\Yumemi\\Parser", "NativeLexer", yumemi_native_lexer_methods);
    native_lexer_class_entry = zend_register_internal_class(&lexer_entry);
    native_lexer_class_entry->ce_flags |= ZEND_ACC_FINAL;
    zend_declare_class_constant_string(
        native_lexer_class_entry, ZEND_STRL("UNICODE_PCRE_VERSION"), yumemi_lexer_unicode_pcre_version());

    return SUCCESS;
}
