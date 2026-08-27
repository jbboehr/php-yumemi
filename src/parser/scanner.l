%top{
/*
 * Portions of these lexical definitions are derived from UDUNITS-2
 * lib/scanner.l, Copyright 2014 University Corporation for Atmospheric
 * Research and contributors.
 *
 * SPDX-License-Identifier: (AGPL-3.0-only WITH romic-exception) AND UCAR
 *
 * See docs/UDUNITS-COPYRIGHT for copying and redistribution conditions.
 */

#include "main/php.h"
#include "native_lexer.h"
}

%option reentrant bison-bridge bison-locations
%option 8bit noyywrap nounput noinput nodefault never-interactive nounistd
%option noyyalloc noyyrealloc noyyfree
%option prefix="yumemi_"
%option extra-type="yumemi_lexer_context *"
%x INVALID_UTF8

%{
#if defined(__GNUC__)
#undef yynoreturn
#define yynoreturn __attribute__((__noreturn__, __unused__))
#endif

#define YY_USER_ACTION                                                                                  \
    do {                                                                                                \
        yylloc->start = yyextra->offset;                                                               \
        yyextra->offset += (size_t)yyleng;                                                             \
        yylloc->end = yyextra->offset;                                                                 \
    } while (0);

#define YY_USER_INIT                                                                                   \
    do {                                                                                                \
        if (yyextra->invalid_utf8) {                                                                   \
            BEGIN(INVALID_UTF8);                                                                       \
        }                                                                                               \
    } while (0)

#define YUMEMI_TRIM_TOKEN(token_length)                                                                \
    do {                                                                                                \
        size_t yumemi_discarded_bytes = (size_t)yyleng - (token_length);                              \
        yyless((int)(token_length));                                                                   \
        yyextra->offset -= yumemi_discarded_bytes;                                                     \
        yylloc->end = yyextra->offset;                                                                 \
    } while (0)

#define YUMEMI_RETURN_TOKEN(token_type)                                                                \
    do {                                                                                                \
        yylval->text = yytext;                                                                         \
        yylval->length = (size_t)yyleng;                                                               \
        if (!yumemi_lexer_accept_token(yyextra, (token_type), yylloc->start, yylloc->end)) {          \
            return YUMEMI_TOKEN_ERROR;                                                                 \
        }                                                                                               \
        return (token_type);                                                                           \
    } while (0)

#define YY_FATAL_ERROR(message) zend_error_noreturn(E_ERROR, "Yumemi native lexer: %s", (message))
%}

ascii_space     [ \t\r\n\f\v]
digit           [0-9]
number          {digit}+(\.{digit}+)*([eE][+-]?{digit}+)?
identifier      [A-Za-z_][A-Za-z0-9_]*
high_byte       [\x80-\xff]
unicode_part    [A-Za-z0-9_.+-]|{high_byte}
unicode_chunk   {unicode_part}*{high_byte}{unicode_part}*

%%

<INVALID_UTF8>(.|\n)+ { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_IDENTIFIER); }

{ascii_space}+  { /* Whitespace does not consume the token budget. */ }

{unicode_chunk} {
                    yumemi_token_type token_type;
                    size_t token_length = yumemi_lexer_classify_unicode_chunk(
                        (const unsigned char *)yytext,
                        (size_t)yyleng,
                        &token_type
                    );

                    YUMEMI_TRIM_TOKEN(token_length);
                    if (token_type != YUMEMI_TOKEN_SKIP) {
                        YUMEMI_RETURN_TOKEN(token_type);
                    }
                }

{number}        {
                    size_t dot_count = 0;
                    size_t offset;
                    bool has_uppercase_exponent;

                    for (offset = 0; offset < (size_t)yyleng; ++offset) {
                        if (yytext[offset] == '.') {
                            ++dot_count;
                        }
                    }

                    has_uppercase_exponent = memchr(yytext, 'E', (size_t)yyleng) != NULL;
                    if (dot_count > 1) {
                        YUMEMI_RETURN_TOKEN(
                            has_uppercase_exponent ? YUMEMI_TOKEN_IDENTIFIER : YUMEMI_TOKEN_INVALID_NUMBER
                        );
                    }
                    if (dot_count == 1 || memchr(yytext, 'e', (size_t)yyleng) != NULL ||
                        memchr(yytext, 'E', (size_t)yyleng) != NULL) {
                        YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_FLOAT);
                    }
                    YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_INTEGER);
                }

{identifier}    { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_IDENTIFIER); }
"."             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_DOT); }
"*"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_MUL); }
"/"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_DIV); }
"^"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_POW); }
"-"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_SUB); }
"+"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_ADD); }
"("             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_LEFT_PAREN); }
")"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_RIGHT_PAREN); }
"@"             { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_AT); }
.               { YUMEMI_RETURN_TOKEN(YUMEMI_TOKEN_IDENTIFIER); }

<<EOF>>         {
                    yylloc->start = yyextra->offset;
                    yylloc->end = yyextra->offset;
                    return YUMEMI_TOKEN_EOF;
                }

%%

void *yyalloc(yy_size_t size, yyscan_t yyscanner)
{
    (void)yyscanner;
    return emalloc(size);
}

void *yyrealloc(void *pointer, yy_size_t size, yyscan_t yyscanner)
{
    (void)yyscanner;
    return erealloc(pointer, size);
}

void yyfree(void *pointer, yyscan_t yyscanner)
{
    (void)yyscanner;
    if (pointer != NULL) {
        efree(pointer);
    }
}
