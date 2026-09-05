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
        yylval->node = NULL;                                                                           \
        if (!yumemi_lexer_accept_token(yyextra, (token_type), yylloc->start, yylloc->end)) {          \
            return YUMEMI_PARSER_error;                                                                \
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

<INVALID_UTF8>(.|\n)+ { YUMEMI_RETURN_TOKEN(T_IDENTIFIER); }

{ascii_space}+  { /* Whitespace does not consume the token budget. */ }

{unicode_chunk} {
                    yumemi_token_type token_type;
                    size_t token_length = yumemi_lexer_classify_unicode_chunk(
                        (const unsigned char *)yytext,
                        (size_t)yyleng,
                        &token_type
                    );

                    YUMEMI_TRIM_TOKEN(token_length);
                    if (token_type != T_SKIP) {
                        YUMEMI_RETURN_TOKEN(token_type);
                    }
                }

{number}        {
                    yumemi_token_type token_type;

                    (void)yumemi_lexer_classify_number(
                        (const unsigned char *)yytext,
                        (size_t)yyleng,
                        &token_type
                    );
                    YUMEMI_RETURN_TOKEN(token_type);
                }

{identifier}    { YUMEMI_RETURN_TOKEN(T_IDENTIFIER); }
"."             { YUMEMI_RETURN_TOKEN(T_DOT); }
"*"             { YUMEMI_RETURN_TOKEN(T_MUL); }
"/"             { YUMEMI_RETURN_TOKEN(T_DIV); }
"^"             { YUMEMI_RETURN_TOKEN(T_POW); }
"-"             { YUMEMI_RETURN_TOKEN(T_SUB); }
"+"             { YUMEMI_RETURN_TOKEN(T_ADD); }
"("             { YUMEMI_RETURN_TOKEN(T_LEFT_PAREN); }
")"             { YUMEMI_RETURN_TOKEN(T_RIGHT_PAREN); }
"@"             { YUMEMI_RETURN_TOKEN(T_AT); }
.               { YUMEMI_RETURN_TOKEN(T_IDENTIFIER); }

<<EOF>>         {
                    yylloc->start = yyextra->offset;
                    yylloc->end = yyextra->offset;
                    return YUMEMI_PARSER_EOF;
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
