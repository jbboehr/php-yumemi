/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YUMEMI_PARSER_SRC_PARSER_PARSER_H_INCLUDED
# define YY_YUMEMI_PARSER_SRC_PARSER_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YUMEMI_PARSER_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define YUMEMI_PARSER_DEBUG 1
#  else
#   define YUMEMI_PARSER_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define YUMEMI_PARSER_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined YUMEMI_PARSER_DEBUG */
#if YUMEMI_PARSER_DEBUG
extern int yumemi_parser_debug;
#endif
/* "%code requires" blocks.  */

#include "parser_types.h"


/* Token kinds.  */
#ifndef YUMEMI_PARSER_TOKENTYPE
# define YUMEMI_PARSER_TOKENTYPE
  enum yumemi_parser_tokentype
  {
    YUMEMI_PARSER_EMPTY = -2,
    YUMEMI_PARSER_EOF = 0,         /* "end of file"  */
    YUMEMI_PARSER_error = 256,     /* error  */
    YUMEMI_PARSER_UNDEF = 273,     /* "invalid token"  */
    T_INTEGER = 258,               /* "integer"  */
    T_SUPERSCRIPT_INTEGER = 259,   /* "superscript integer"  */
    T_INVALID_SUPERSCRIPT = 260,   /* "superscript sign without digits"  */
    T_FLOAT = 261,                 /* "decimal number"  */
    T_DOT = 262,                   /* "."  */
    T_MUL = 263,                   /* "*"  */
    T_DIV = 264,                   /* "/"  */
    T_POW = 265,                   /* "^"  */
    T_SUB = 266,                   /* "-"  */
    T_ADD = 267,                   /* "+"  */
    T_IDENTIFIER = 268,            /* "identifier"  */
    T_LEFT_PAREN = 269,            /* "("  */
    T_RIGHT_PAREN = 270,           /* ")"  */
    T_AT = 271,                    /* "@"  */
    T_INVALID_NUMBER = 272,        /* "malformed number"  */
    T_SKIP = 274                   /* T_SKIP  */
  };
  typedef enum yumemi_parser_tokentype yumemi_parser_token_kind_t;
#endif

/* Value type.  */
#if ! defined YUMEMI_PARSER_STYPE && ! defined YUMEMI_PARSER_STYPE_IS_DECLARED
typedef yumemi_lexer_value YUMEMI_PARSER_STYPE;
# define YUMEMI_PARSER_STYPE_IS_TRIVIAL 1
# define YUMEMI_PARSER_STYPE_IS_DECLARED 1
#endif

/* Location type.  */
typedef yumemi_lexer_location YUMEMI_PARSER_LTYPE;




int yumemi_parser_parse (void *scanner, yumemi_parse_context *context);


#endif /* !YY_YUMEMI_PARSER_SRC_PARSER_PARSER_H_INCLUDED  */
