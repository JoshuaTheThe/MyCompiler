
#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define IDENTIFIER_SIZE 256

typedef enum
{
        LEXER_TOKEN_EOF,
        LEXER_TOKEN_IDENTIFIER,
        LEXER_TOKEN_INTEGER_LITERAL,
        LEXER_TOKEN_FLOAT_LITERAL,
        LEXER_TOKEN_CHAR_LITERAL,
        LEXER_TOKEN_STRING_LITERAL,
        LEXER_TOKEN_SINGLE_SYMBOL_START,
        LEXER_TOKEN_PLUS=LEXER_TOKEN_SINGLE_SYMBOL_START,
        LEXER_TOKEN_MINUS,
        LEXER_TOKEN_ASTERISK,
        LEXER_TOKEN_SLASH,
        LEXER_TOKEN_PERCENT,
        LEXER_TOKEN_CARET,
        LEXER_TOKEN_AMPERSAND,
        LEXER_TOKEN_PIPE,
        LEXER_TOKEN_TILDE,
        LEXER_TOKEN_BANG,
        LEXER_TOKEN_COMMA,
        LEXER_TOKEN_DOT,
        LEXER_TOKEN_SEMICOLON,
        LEXER_TOKEN_COLON,
        LEXER_TOKEN_QUESTION,
        LEXER_TOKEN_LPAREN,
        LEXER_TOKEN_RPAREN,
        LEXER_TOKEN_LBRACKET,
        LEXER_TOKEN_RBRACKET,
        LEXER_TOKEN_LSBRACKET,
        LEXER_TOKEN_RSBRACKET,
        LEXER_TOKEN_GREATER,
        LEXER_TOKEN_LESS,
        LEXER_TOKEN_EQUAL,

        LEXER_TOKEN_DOUBLE_SYMBOL_START,
        LEXER_TOKEN_EQUALS=LEXER_TOKEN_DOUBLE_SYMBOL_START,                // ==
        LEXER_TOKEN_DOUBLE_AMPERSAND,                // &&
        LEXER_TOKEN_DOUBLE_PIPE,                 // ||
        LEXER_TOKEN_ADDSET,             // +=
        LEXER_TOKEN_SUBSET,             // -=
        LEXER_TOKEN_MULSET,             // *=
        LEXER_TOKEN_DIVSET,             // /=
        LEXER_TOKEN_ANDSET,             // &=
        LEXER_TOKEN_ORSET,              // |=
        LEXER_TOKEN_XORSET,             // ^=
        LEXER_TOKEN_MODSET,             // %=
        LEXER_TOKEN_NOTEQ,              // !=
        LEXER_TOKEN_LESSEQ,             // <=
        LEXER_TOKEN_GREATEREQ,          // >=
        LEXER_TOKEN_SHIFTLEFT,          // <<
        LEXER_TOKEN_SHIFTRIGHT,         // >>
        LEXER_TOKEN_ARROW,              // ->
        LEXER_TOKEN_INC,                // ++
        LEXER_TOKEN_DEC,                // --
        LEXER_TOKEN_COMMENT,            // //
        LEXER_TOKEN_LMULTICOMMENT,      // /*
        LEXER_TOKEN_RMULTICOMMENT,      // */

        LEXER_TOKEN_TRIPLE_SYMBOL_START,
        LEXER_TOKEN_SHIFTLEFTSET=LEXER_TOKEN_TRIPLE_SYMBOL_START,       // <<=
        LEXER_TOKEN_SHIFTRIGHTSET,      // >>=
        LEXER_TOKEN_ROLLLEFT,           // <<<
        LEXER_TOKEN_ROLLRIGHT,          // >>>
} token_kind_t;

struct _TOKEN;
typedef struct _TOKEN token_t;

typedef struct
{
        FILE   *fp;
        size_t  Line,Column,LineOffset;
        size_t  _Line,_Column,_LineOffset;
        char    Identifier[IDENTIFIER_SIZE];
        size_t *LineOffsets;
        size_t  LineCapacity;
        size_t  LineCount;
        token_t *Tokens;
        token_t *Current;
} token_stream_t;

typedef struct _TOKEN
{
        token_stream_t *File;
        token_kind_t Class;
        char    Identifier[IDENTIFIER_SIZE];
        int     Number;
        size_t  Line,Column,LineOffset;
        struct _TOKEN *Next;
        struct _TOKEN *Prev;
} token_t;

// Caller owns, returns root
token_t *lexer_analyse_file(token_stream_t *fp);
void   lexer_destroy(token_t *Tokens);
token_t *lexer_construct_next(token_t **Tokens, token_stream_t *fil);
token_t  lexer_next(token_stream_t *fil);
token_stream_t lexer_create_stream(const char *path);
void   lexer_close_stream(token_stream_t fil);
void lexer_index_lines(token_stream_t *fil);
void lexer_remove_token(token_t *Token);
char lexer_getch(token_stream_t *fil);
void lexer_ungetc(token_stream_t *fil, char Character);
char lexer_peek(token_stream_t *fil, long off);
token_t *lexer_unconsume(token_stream_t *stream);
token_t *lexer_expect(token_stream_t *stream, token_kind_t Class);
bool lexer_accept(token_stream_t *stream, token_kind_t Class);
token_t *lexer_consume(token_stream_t *stream);
token_t lexer_number(token_stream_t *fil, char First);
token_t lexer_character(token_stream_t *fil, char First);

#endif
