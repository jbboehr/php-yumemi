%code top {
/*
 * Portions of this parser grammar are derived from UDUNITS-2 lib/parser.y,
 * Copyright 2020 University Corporation for Atmospheric Research and contributors.
 *
 * SPDX-License-Identifier: (AGPL-3.0-only WITH romic-exception) AND UCAR
 *
 * See docs/UDUNITS-COPYRIGHT for copying and redistribution conditions.
 */

#include "main/php.h"
#include "native_parser.h"
#include "scanner.h"

#undef YYSTYPE
#undef YYLTYPE

#define YYMALLOC emalloc
#define YYFREE efree

#define YYLLOC_DEFAULT(current, rhs, count)                                                           \
    do {                                                                                              \
        if ((count) > 0) {                                                                            \
            (current).start = YYRHSLOC((rhs), 1).start;                                               \
            (current).end = YYRHSLOC((rhs), (count)).end;                                             \
        } else {                                                                                      \
            (current).start = YYRHSLOC((rhs), 0).end;                                                 \
            (current).end = YYRHSLOC((rhs), 0).end;                                                   \
        }                                                                                             \
    } while (0)
}

%code requires {
#include "native_parser.h"
}

%code {
static int yumemi_parser_lex(yumemi_lexer_value *value, yumemi_lexer_location *location, void *scanner)
{
    return yumemi_lex(value, location, scanner);
}

static void yumemi_parser_error(yumemi_lexer_location *location,
                                void *scanner,
                                yumemi_parse_context *context,
                                const char *message)
{
    (void)scanner;
    yumemi_parse_context_set_error(context, location, message);
}
}

%define api.pure full
%define api.prefix {yumemi_parser_}
%define api.value.type {yumemi_lexer_value}
%define api.location.type {yumemi_lexer_location}
%define parse.error custom
%locations

%parse-param {void *scanner}
%parse-param {yumemi_parse_context *context}
%lex-param {void *scanner}

%token T_INTEGER 258 "integer"
%token T_SUPERSCRIPT_INTEGER 259 "superscript integer"
%token T_INVALID_SUPERSCRIPT 260 "superscript sign without digits"
%token T_FLOAT 261 "decimal number"
%token T_DOT 262 "."
%token T_MUL 263 "*"
%token T_DIV 264 "/"
%token T_POW 265 "^"
%token T_SUB 266 "-"
%token T_ADD 267 "+"
%token T_IDENTIFIER 268 "identifier"
%token T_LEFT_PAREN 269 "("
%token T_RIGHT_PAREN 270 ")"
%token T_AT 271 "@"
%token T_INVALID_NUMBER 272 "malformed number"

%%

start:
        exp                                     { context->root = $1.node; }
    ;

exp:
        additive_exp                            { $$.node = $1.node; }
    ;

additive_exp:
        product_exp                             { $$.node = $1.node; }
    |   additive_exp T_ADD product_exp          { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_ADD, $1.node, $3.node, &@$); }
    |   additive_exp T_SUB product_exp          { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_SUB, $1.node, $3.node, &@$); }
    ;

product_exp:
        unary_exp                               { $$.node = $1.node; }
    |   product_exp power_exp                   { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_MUL, $1.node, $2.node, &@$); }
    |   product_exp T_DOT unary_exp             { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_MUL, $1.node, $3.node, &@$); }
    |   product_exp T_MUL unary_exp             { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_MUL, $1.node, $3.node, &@$); }
    |   product_exp T_DIV unary_exp             { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_DIV, $1.node, $3.node, &@$); }
    ;

unary_exp:
        power_exp                               { $$.node = $1.node; }
    |   T_SUB unary_exp                         { $$.node = yumemi_ast_make_negation(context, $2.node, &@$); }
    ;

power_exp:
        simple                                  { $$.node = $1.node; }
    |   simple T_POW unary_exp                  { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_POW, $1.node, $3.node, &@$); }
    ;

simple:
        number                                  { $$.node = $1.node; }
    |   identifier                              { $$.node = $1.node; }
    |   identifier T_AT signed_number           { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_AT, $1.node, $3.node, &@$); }
    |   T_LEFT_PAREN exp T_RIGHT_PAREN          { $$.node = $2.node; }
    |   simple T_SUPERSCRIPT_INTEGER            { $$.node = yumemi_ast_make_binary(context, YUMEMI_AST_POW, $1.node, yumemi_ast_make_superscript_integer(context, $2.text, $2.length, &@2), &@$); }
    ;

number:
        T_INTEGER                               { $$.node = yumemi_ast_make_leaf(context, YUMEMI_AST_INTEGER, $1.text, $1.length, &@1); }
    |   T_FLOAT                                 { $$.node = yumemi_ast_make_leaf(context, YUMEMI_AST_FLOAT, $1.text, $1.length, &@1); }
    ;

signed_number:
        number                                  { $$.node = $1.node; }
    |   T_SUB number                            { $$.node = yumemi_ast_make_negation(context, $2.node, &@$); }
    ;

identifier:
        T_IDENTIFIER                            { $$.node = yumemi_ast_make_leaf(context, YUMEMI_AST_IDENTIFIER, $1.text, $1.length, &@1); }
    ;

%%

static int yyreport_syntax_error(const yypcontext_t *parser_context,
                                 void *scanner,
                                 yumemi_parse_context *context)
{
    yysymbol_kind_t expected_symbols[YYNTOKENS];
    const char *expected_names[YYNTOKENS];
    yysymbol_kind_t unexpected_symbol = yypcontext_token(parser_context);
    const yumemi_lexer_location *location = yypcontext_location(parser_context);
    int expected_count;
    int index;

    (void)scanner;
    expected_count = yypcontext_expected_tokens(parser_context, expected_symbols, YYNTOKENS);
    if (expected_count < 0) {
        return expected_count;
    }

    for (index = 0; index < expected_count; ++index) {
        expected_names[index] = yysymbol_name(expected_symbols[index]);
    }
    yumemi_parse_context_set_syntax_error(context,
                                          location,
                                          unexpected_symbol == YYSYMBOL_YYEMPTY
                                              ? NULL
                                              : yysymbol_name(unexpected_symbol),
                                          expected_names,
                                          (size_t)expected_count);

    return 0;
}
