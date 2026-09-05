/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <string.h>

#include "main/php.h"
#include "Zend/zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"

#include "native_lexer.h"
#include "scanner.h"
#include "unicode_ranges.h"

static zend_class_entry *yumemi_native_limit_exception_class;

static bool yumemi_unicode_in_ranges(uint32_t code_point, const yumemi_unicode_range *ranges, size_t range_count)
{
    size_t lower = 0;
    size_t upper = range_count;

    while (lower < upper) {
        size_t middle = lower + (upper - lower) / 2;
        const yumemi_unicode_range *range = &ranges[middle];

        if (code_point < range->first) {
            upper = middle;
        } else if (code_point > range->last) {
            lower = middle + 1;
        } else {
            return true;
        }
    }

    return false;
}

static bool yumemi_unicode_is_identifier(uint32_t code_point)
{
    return code_point == '_' || code_point == 0x00b0 ||
           yumemi_unicode_in_ranges(
               code_point, yumemi_unicode_identifier_ranges, YUMEMI_UNICODE_IDENTIFIER_RANGE_COUNT);
}

static bool yumemi_unicode_is_decimal_digit(uint32_t code_point)
{
    return yumemi_unicode_in_ranges(
        code_point, yumemi_unicode_decimal_digit_ranges, YUMEMI_UNICODE_DECIMAL_DIGIT_RANGE_COUNT);
}

static bool yumemi_unicode_is_whitespace(uint32_t code_point)
{
    return yumemi_unicode_in_ranges(
        code_point, yumemi_unicode_whitespace_ranges, YUMEMI_UNICODE_WHITESPACE_RANGE_COUNT);
}

static bool yumemi_utf8_decode(const unsigned char *text, size_t length, uint32_t *code_point, size_t *width)
{
    unsigned char first;

    if (length == 0) {
        return false;
    }

    first = text[0];
    if (first < 0x80) {
        *code_point = first;
        *width = 1;
        return true;
    }

    if (first >= 0xc2 && first <= 0xdf && length >= 2 && (text[1] & 0xc0) == 0x80) {
        *code_point = ((uint32_t)(first & 0x1f) << 6) | (uint32_t)(text[1] & 0x3f);
        *width = 2;
        return true;
    }

    if (first >= 0xe0 && first <= 0xef && length >= 3 && (text[1] & 0xc0) == 0x80 && (text[2] & 0xc0) == 0x80 &&
        !(first == 0xe0 && text[1] < 0xa0) && !(first == 0xed && text[1] >= 0xa0)) {
        *code_point = ((uint32_t)(first & 0x0f) << 12) | ((uint32_t)(text[1] & 0x3f) << 6) | (uint32_t)(text[2] & 0x3f);
        *width = 3;
        return true;
    }

    if (first >= 0xf0 && first <= 0xf4 && length >= 4 && (text[1] & 0xc0) == 0x80 && (text[2] & 0xc0) == 0x80 &&
        (text[3] & 0xc0) == 0x80 && !(first == 0xf0 && text[1] < 0x90) && !(first == 0xf4 && text[1] >= 0x90)) {
        *code_point = ((uint32_t)(first & 0x07) << 18) | ((uint32_t)(text[1] & 0x3f) << 12) |
                      ((uint32_t)(text[2] & 0x3f) << 6) | (uint32_t)(text[3] & 0x3f);
        *width = 4;
        return true;
    }

    *code_point = first;
    *width = 1;
    return false;
}

void yumemi_lexer_context_init(yumemi_lexer_context *context, const unsigned char *input, size_t length)
{
    size_t offset = 0;

    memset(context, 0, sizeof(*context));
    if (length > YUMEMI_LEXER_INPUT_BYTES_LIMIT) {
        context->error = (yumemi_lexer_error){
            YUMEMI_LEXER_LIMIT_INPUT_BYTES, YUMEMI_LEXER_INPUT_BYTES_LIMIT, length, 0, length,
        };
        return;
    }

    while (offset < length) {
        uint32_t code_point;
        size_t width;

        if (!yumemi_utf8_decode(input + offset, length - offset, &code_point, &width)) {
            context->invalid_utf8 = true;
            return;
        }
        offset += width;
    }
}

static bool yumemi_unicode_is_superscript_digit(uint32_t code_point)
{
    return code_point == 0x00b9 || code_point == 0x00b2 || code_point == 0x00b3 || code_point == 0x2070 ||
           (code_point >= 0x2074 && code_point <= 0x2079);
}

static size_t yumemi_scan_identifier(const unsigned char *text, size_t length)
{
    size_t offset = 0;

    while (offset < length) {
        uint32_t code_point;
        size_t width;

        if (!yumemi_utf8_decode(text + offset, length - offset, &code_point, &width) ||
            !yumemi_unicode_is_identifier(code_point)) {
            break;
        }

        offset += width;
    }

    return offset;
}

static size_t yumemi_scan_digits(const unsigned char *text, size_t length, bool *ascii_only)
{
    size_t offset = 0;

    while (offset < length) {
        uint32_t code_point;
        size_t width;

        if (!yumemi_utf8_decode(text + offset, length - offset, &code_point, &width) ||
            !yumemi_unicode_is_decimal_digit(code_point)) {
            break;
        }

        if (code_point < '0' || code_point > '9') {
            *ascii_only = false;
        }
        offset += width;
    }

    return offset;
}

size_t yumemi_lexer_classify_number(const unsigned char *text, size_t length, yumemi_token_type *type)
{
    size_t offset;
    size_t digit_length;
    size_t dot_count = 0;
    bool ascii_only = true;
    bool has_exponent = false;
    bool has_uppercase_exponent = false;

    offset = yumemi_scan_digits(text, length, &ascii_only);

    while (offset < length && text[offset] == '.') {
        bool segment_ascii_only = true;

        digit_length = yumemi_scan_digits(text + offset + 1, length - offset - 1, &segment_ascii_only);
        if (digit_length == 0) {
            break;
        }

        ascii_only = ascii_only && segment_ascii_only;
        ++dot_count;
        offset += 1 + digit_length;
    }

    if (offset < length && (text[offset] == 'e' || text[offset] == 'E')) {
        size_t exponent_start = offset + 1;
        bool exponent_ascii_only = true;

        if (exponent_start < length && (text[exponent_start] == '+' || text[exponent_start] == '-')) {
            ++exponent_start;
        }

        digit_length = yumemi_scan_digits(text + exponent_start, length - exponent_start, &exponent_ascii_only);
        if (digit_length > 0) {
            ascii_only = ascii_only && exponent_ascii_only;
            has_exponent = true;
            has_uppercase_exponent = text[offset] == 'E';
            offset = exponent_start + digit_length;
        }
    }

    if (!ascii_only) {
        *type = T_IDENTIFIER;
    } else if (dot_count > 1) {
        *type = has_uppercase_exponent ? T_IDENTIFIER : T_INVALID_NUMBER;
    } else if (dot_count == 1 || has_exponent) {
        *type = T_FLOAT;
    } else {
        *type = T_INTEGER;
    }

    return offset;
}

bool yumemi_lexer_accept_token(yumemi_lexer_context *context, yumemi_token_type type, size_t start, size_t end)
{
    size_t token_length = end - start;

    ++context->token_count;
    if (context->token_count > YUMEMI_LEXER_TOKEN_COUNT_LIMIT) {
        context->error = (yumemi_lexer_error){
            YUMEMI_LEXER_LIMIT_TOKEN_COUNT, YUMEMI_LEXER_TOKEN_COUNT_LIMIT, context->token_count, start, end,
        };
        return false;
    }

    if (token_length > YUMEMI_LEXER_TOKEN_BYTES_LIMIT &&
        (type == T_IDENTIFIER || type == T_INTEGER || type == T_FLOAT || type == T_SUPERSCRIPT_INTEGER ||
         type == T_INVALID_NUMBER)) {
        context->error = (yumemi_lexer_error){
            YUMEMI_LEXER_LIMIT_TOKEN_BYTES, YUMEMI_LEXER_TOKEN_BYTES_LIMIT, token_length, start, end,
        };
        return false;
    }

    if (type == T_LEFT_PAREN) {
        ++context->nesting_depth;
        if (context->nesting_depth > YUMEMI_LEXER_NESTING_DEPTH_LIMIT) {
            context->error = (yumemi_lexer_error){
                YUMEMI_LEXER_LIMIT_NESTING_DEPTH, YUMEMI_LEXER_NESTING_DEPTH_LIMIT, context->nesting_depth, start, end,
            };
            return false;
        }
    } else if (type == T_RIGHT_PAREN && context->nesting_depth > 0) {
        --context->nesting_depth;
    }

    return true;
}

size_t yumemi_lexer_classify_unicode_chunk(const unsigned char *text, size_t length, yumemi_token_type *type)
{
    uint32_t code_point;
    size_t width;

    if (length == 0) {
        *type = T_IDENTIFIER;
        return 0;
    }

    if (!yumemi_utf8_decode(text, length, &code_point, &width)) {
        *type = T_IDENTIFIER;
        return width;
    }

    switch (code_point) {
        case '.':
            *type = T_DOT;
            return width;
        case '*':
            *type = T_MUL;
            return width;
        case '/':
            *type = T_DIV;
            return width;
        case '^':
            *type = T_POW;
            return width;
        case '-':
            *type = T_SUB;
            return width;
        case '+':
            *type = T_ADD;
            return width;
        case '(':
            *type = T_LEFT_PAREN;
            return width;
        case ')':
            *type = T_RIGHT_PAREN;
            return width;
        case '@':
            *type = T_AT;
            return width;
    }

    if (yumemi_unicode_is_whitespace(code_point)) {
        size_t offset = width;

        while (offset < length) {
            uint32_t next_code_point;
            size_t next_width;

            if (!yumemi_utf8_decode(text + offset, length - offset, &next_code_point, &next_width) ||
                !yumemi_unicode_is_whitespace(next_code_point)) {
                break;
            }
            offset += next_width;
        }

        *type = T_SKIP;
        return offset;
    }

    if (yumemi_unicode_is_decimal_digit(code_point)) {
        return yumemi_lexer_classify_number(text, length, type);
    }

    if (yumemi_unicode_is_identifier(code_point)) {
        *type = T_IDENTIFIER;
        return yumemi_scan_identifier(text, length);
    }

    if (code_point == 0x00b7) {
        *type = T_MUL;
        return width;
    }

    if (code_point == 0x207a || code_point == 0x207b) {
        uint32_t next_code_point;
        size_t next_width;
        size_t offset = width;

        if (offset >= length || !yumemi_utf8_decode(text + offset, length - offset, &next_code_point, &next_width) ||
            !yumemi_unicode_is_superscript_digit(next_code_point)) {
            *type = T_INVALID_SUPERSCRIPT;
            return width;
        }

        offset += next_width;
        while (offset < length && yumemi_utf8_decode(text + offset, length - offset, &next_code_point, &next_width) &&
               yumemi_unicode_is_superscript_digit(next_code_point)) {
            offset += next_width;
        }

        *type = T_SUPERSCRIPT_INTEGER;
        return offset;
    }

    if (yumemi_unicode_is_superscript_digit(code_point)) {
        size_t offset = width;

        while (offset < length) {
            uint32_t next_code_point;
            size_t next_width;

            if (!yumemi_utf8_decode(text + offset, length - offset, &next_code_point, &next_width) ||
                !yumemi_unicode_is_superscript_digit(next_code_point)) {
                break;
            }
            offset += next_width;
        }

        *type = T_SUPERSCRIPT_INTEGER;
        return offset;
    }

    *type = T_IDENTIFIER;
    return width;
}

const char *yumemi_lexer_token_name(yumemi_token_type type)
{
    switch (type) {
        case T_INTEGER:
            return "integer";
        case T_SUPERSCRIPT_INTEGER:
            return "superscript-integer";
        case T_INVALID_SUPERSCRIPT:
            return "invalid-superscript";
        case T_FLOAT:
            return "decimal-number";
        case T_DOT:
            return "dot";
        case T_MUL:
            return "mul";
        case T_DIV:
            return "div";
        case T_POW:
            return "pow";
        case T_SUB:
            return "sub";
        case T_ADD:
            return "add";
        case T_IDENTIFIER:
            return "identifier";
        case T_LEFT_PAREN:
            return "left-paren";
        case T_RIGHT_PAREN:
            return "right-paren";
        case T_AT:
            return "at";
        case T_INVALID_NUMBER:
            return "invalid-number";
        default:
            return "unknown";
    }
}

const char *yumemi_lexer_limit_name(yumemi_lexer_limit limit)
{
    switch (limit) {
        case YUMEMI_LEXER_LIMIT_INPUT_BYTES:
            return "input-bytes";
        case YUMEMI_LEXER_LIMIT_TOKEN_COUNT:
            return "token-count";
        case YUMEMI_LEXER_LIMIT_NESTING_DEPTH:
            return "nesting-depth";
        case YUMEMI_LEXER_LIMIT_TOKEN_BYTES:
            return "token-bytes";
        default:
            return "unknown";
    }
}

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
        native_lexer_class_entry, ZEND_STRL("UNICODE_PCRE_VERSION"), YUMEMI_UNICODE_PCRE_VERSION);

    return SUCCESS;
}
