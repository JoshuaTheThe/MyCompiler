
#include "lexer.h"
#include "error.h"

char lexer_getch(token_stream_t *fil)
{
        char Character   = fgetc(fil->fp);
        fil->_Column     = fil->Column;
        fil->_Line       = fil->Line;
        fil->_LineOffset = fil->LineOffset;
        if (Character == '\n') { fil->Line   += 1; fil->LineOffset = ftell(fil->fp); fil->Column = 0; }
        else                   { fil->Column += 1; }
        return Character;
}

void lexer_ungetch(token_stream_t *fil, char Character)
{
        fil->Column     = fil->_Column;
        fil->Line       = fil->_Line;
        fil->LineOffset = fil->_LineOffset;
        ungetc(Character, fil->fp);
}

char lexer_peek(token_stream_t *fil, long off)
{
        size_t _off = ftell(fil->fp);
        fseek(fil->fp, off, SEEK_CUR);
        char Character  = fgetc(fil->fp);
        fseek(fil->fp, _off, SEEK_SET);
        return Character;
}

token_t lexer_character(token_stream_t *fil, char First)
{
        token_t Token      = {0};
        char Character   = 0;
        Token.Class      = First == '\'' ? LEXER_TOKEN_CHAR_LITERAL : LEXER_TOKEN_STRING_LITERAL;
        Token.File       = fil;
        Token.Column     = fil->Column;
        Token.LineOffset = fil->LineOffset;
        Token.Line       = fil->Line;
        while ((Character = lexer_getch(fil)) != EOF && Character != First)
        {
                Token.Identifier[Token.Number++] = Character;
        }

        return Token;
}

token_t lexer_number(token_stream_t *fil, char First)
{
        token_t Token      = {0};
        char Character   = 0;
        Token.Class      = LEXER_TOKEN_INTEGER_LITERAL;
        Token.File       = fil;
        Token.Column     = fil->Column;
        Token.LineOffset = fil->LineOffset;
        Token.Line       = fil->Line;
        Token.Identifier[Token.Number++] = First;
        while ((Character = lexer_getch(fil)) != EOF && (isdigit(Character) || (Token.Class == LEXER_TOKEN_INTEGER_LITERAL && Character == '.')))
        {
                Token.Identifier[Token.Number++] = Character;
                if (Character == '.') Token.Class = LEXER_TOKEN_FLOAT_LITERAL;
        }

        lexer_ungetch(fil, Character);
        return Token;
}

// free the entire token tree
void lexer_destroy(token_t *Tokens)
{
        token_t *Last = NULL;
        while (Tokens)
        {
                if (Last)
                        free(Last);
                Last   = Tokens;
                Tokens = Tokens->Next;
        }

        if (Last)
                free(Last);
}

token_t *lexer_find_tail(token_t *Tokens)
{
        static token_t *CachedTail = NULL;
        static token_t *CachedBase = NULL;
        if (CachedTail && CachedTail->Next == NULL && CachedBase == Tokens)
                return CachedTail;
        while (Tokens->Next)
                Tokens = Tokens->Next;
        CachedTail = Tokens;
        CachedBase = Tokens;
        return Tokens;
}

// create next token for a file
token_t *lexer_construct_next(token_t **Tokens, token_stream_t *fil)
{
        token_t *NewToken = calloc(1, sizeof(*NewToken));
        token_t  Contents = lexer_next(fil);
        token_t *Tail     = NULL;
        if (!NewToken)
        {
                printf("Could not create Token\n");
                abort();
        }
        *NewToken = Contents;
        if (*Tokens == NULL)
        {
                *Tokens = NewToken;
                return NewToken;
        }
        Tail = lexer_find_tail(*Tokens);
        Tail->Next     = NewToken;
        NewToken->Prev = Tail;
        return NewToken;
}

// load the entire contents of a file then lex it.
token_t *lexer_analyse_file(token_stream_t *fil)
{
        token_t *Tokens = NULL;
        token_t *Last   = NULL;
        while ((Last = lexer_construct_next(&Tokens, fil))->Class != LEXER_TOKEN_EOF)
                ;
        lexer_index_lines(fil);
        fil->Tokens = Tokens;
        fil->Current = Tokens;
        return Tokens;
}

// this routine is slow and bad and stinky but oh well we only do it once per file
void lexer_index_lines(token_stream_t *fil)
{
        if (fil->LineOffsets)
        {
                printf("File already indexed\n");
                abort();
        }

        fseek(fil->fp, 0, SEEK_SET);
        fil->LineOffsets = malloc(sizeof(long) * 1024);
        fil->LineCapacity = 1024;
        fil->LineCount = 1;
        fil->LineOffsets[0] = 0;
        int c;
        long pos = 0;
        while ((c = fgetc(fil->fp)) != EOF)
        {
                pos++;
                if (c == '\n')
                {
                        if (fil->LineCount >= fil->LineCapacity)
                        {
                                fil->LineCapacity *= 2;
                                fil->LineOffsets = realloc(fil->LineOffsets,
                                                           sizeof(long) * fil->LineCapacity);
                        }

                        fil->LineOffsets[fil->LineCount++] = pos;
                }
        }
}

token_t lexer_identifier(token_stream_t *fil, char First)
{
        token_t Token      = {0};
        char Character   = 0;
        Token.Class      = LEXER_TOKEN_IDENTIFIER;
        Token.File       = fil;
        Token.Column     = fil->Column;
        Token.LineOffset = fil->LineOffset;
        Token.Line       = fil->Line;
        Token.Identifier[Token.Number++] = First;
        while ((Character = lexer_getch(fil)) != EOF && (isalpha(Character) || Character == '_' || isalnum(Character)))
        {
                Token.Identifier[Token.Number++] = Character;
        }

        lexer_ungetch(fil, Character);
        return Token;
}

token_t lexer_operator(token_stream_t *fil, char First)
{
        token_t Token = {0};
        static const char MultiClassText[][4] =
        {
                "==",
                "&&",
                "||",
                "+=",
                "-=",
                "*=",
                "/=",
                "&=",
                "|=",
                "^=",
                "%=",
                "!=",
                "<=",
                ">=",
                "<<",
                ">>",
                "->",
                "++",
                "--",
                "..",
                "<<=",
                ">>=",
                "<<<",
                ">>>",
                "...",
        };

        static const token_kind_t MultiClass[] = {
                LEXER_TOKEN_EQUALS,             // ==
                LEXER_TOKEN_DOUBLE_AMPERSAND,   // &&
                LEXER_TOKEN_DOUBLE_PIPE,        // ||
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
                LEXER_TOKEN_SHIFTLEFTSET,       // <<=
                LEXER_TOKEN_SHIFTRIGHTSET,      // >>=
                LEXER_TOKEN_ROLLLEFT,           // <<<
                LEXER_TOKEN_ROLLRIGHT,          // >>>
        };

        // TODO - check for triple and double symbol before single
        static const token_kind_t SingleClass[256] =
        {
                ['+'] = LEXER_TOKEN_PLUS,
                ['-'] = LEXER_TOKEN_MINUS,
                ['*'] = LEXER_TOKEN_ASTERISK,
                ['/'] = LEXER_TOKEN_SLASH,
                ['%'] = LEXER_TOKEN_PERCENT,
                ['^'] = LEXER_TOKEN_CARET,
                ['&'] = LEXER_TOKEN_AMPERSAND,
                ['|'] = LEXER_TOKEN_PIPE,
                ['~'] = LEXER_TOKEN_TILDE,
                ['!'] = LEXER_TOKEN_BANG,
                [','] = LEXER_TOKEN_COMMA,
                ['.'] = LEXER_TOKEN_DOT,
                [';'] = LEXER_TOKEN_SEMICOLON,
                [':'] = LEXER_TOKEN_COLON,
                ['?'] = LEXER_TOKEN_QUESTION,
                ['('] = LEXER_TOKEN_LPAREN,
                [')'] = LEXER_TOKEN_RPAREN,
                ['{'] = LEXER_TOKEN_LBRACKET,
                ['}'] = LEXER_TOKEN_RBRACKET,
                ['['] = LEXER_TOKEN_LSBRACKET,
                [']'] = LEXER_TOKEN_RSBRACKET,
                ['>'] = LEXER_TOKEN_GREATER,
                ['<'] = LEXER_TOKEN_LESS,
                ['='] = LEXER_TOKEN_EQUAL,
        };

        char Peek[4] = {First,0,0,0};
        size_t Len = 1;
        Peek[1] = lexer_peek(fil, 0);
        Peek[2] = lexer_peek(fil, 1);
        for (size_t i = 0; i < sizeof(MultiClassText)/4; i++)
        {
                if (strlen(MultiClassText[i]) == 3 &&
                        memcmp(Peek, MultiClassText[i], 3) == 0)
                {
                        Token.Class = MultiClass[i];
                        Len = 3;
                        goto found;
                }
        }

        for (size_t i = 0; i < sizeof(MultiClassText)/4; i++)
        {
                if (strlen(MultiClassText[i]) == 2 &&
                        memcmp(Peek, MultiClassText[i], 2) == 0)
                {
                        Token.Class = MultiClass[i];
                        Len = 2;
                        goto found;
                }
        }

        Token.Class = SingleClass[(unsigned char)First];
        Len = 1;
found:  Token.File = fil;
        Token.Column = fil->Column;
        Token.LineOffset = fil->LineOffset;
        Token.Line = fil->Line;
        for (size_t i = 1; i < Len; i++)
        {
                lexer_getch(fil);
                Token.Identifier[Token.Number++] = Peek[i];
        }

        return Token;
}

token_stream_t lexer_create_stream(const char *Path)
{
        token_stream_t fil     = {0};
        fil.fp         = fopen(Path, "r");
        fil.Column     = 0;
        fil.Line       = 1;
        fil.LineOffset = 0;
        strncpy(fil.Identifier, Path, IDENTIFIER_SIZE - 1); // ewww
        if (!fil.fp)
        {
                printf("Could not open file %s\n", Path);
                abort();
        }

        return fil;
}

void lexer_close_stream(token_stream_t fil)
{
        free(fil.LineOffsets);
        fclose(fil.fp);
}

void lexer_remove_token(token_t *Token)
{
        if (Token->Prev)
                Token->Prev->Next = Token->Next;
        if (Token->Next)
                Token->Next->Prev = Token->Prev;
}

token_t *lexer_consume(token_stream_t *stream)
{
        token_t **Token = &stream->Current;
        if (!Token)
                return NULL;
        token_t *const Tok = *Token;
        *Token = (*Token)->Next;
        return Tok;
}

token_t *lexer_expect(token_stream_t *stream, token_kind_t Class)
{
        token_t *Token = stream->Current;
        if (lexer_consume(stream)->Class != Class)
        {
                comperror(stream, *Token, "Expected token of class %ld when provided %ld", Class, Token->Class);
        }

        return Token;
}

bool lexer_accept(token_stream_t *stream, token_kind_t Class)
{
        token_t **Token = &stream->Current;
        if ((*Token)->Class == Class)
        {
                lexer_consume(stream);
                return true;
        }

        return false;
}

token_t *lexer_unconsume(token_stream_t *stream)
{
        token_t **Token = &stream->Current;
        if (!Token)
                return NULL;
        token_t *const Tok = *Token;
        *Token = (*Token)->Prev;
        return Tok;
}

// returns the next raw token
token_t lexer_next(token_stream_t *fil)
{
        char Character = 0, Saved = 0;
        // Skip WhiteSpace
        while ((Character = lexer_getch(fil)) != EOF && isspace(Character))
            ;
        token_t none = {0};
        none.File = fil;
        none.Column = fil->Column;
        none.LineOffset = fil->LineOffset;
        none.Line = fil->Line;
        if (Character == '/')
        {
                Saved = lexer_getch(fil);

                if (Saved == '/')
                {
                        while ((Character = lexer_getch(fil)) != EOF && Character != '\n')
                                ;
                        return lexer_next(fil);
                }
                else if (Saved == '*')
                {
                        char Prev = 0;
                        while ((Character = lexer_getch(fil)) != EOF)
                        {
                                if (Prev == '*' && Character == '/')
                                        break;
                                Prev = Character;
                        }
                        if (Character == EOF)
                        {
                                return none;
                        }
                        return lexer_next(fil);
                }
                else
                {
                        if (Saved != EOF)
                                lexer_ungetch(fil, Saved);
                        return lexer_operator(fil, '/');
                }
        }

        if (isdigit(Character))
                return lexer_number(fil, Character);
        else if (isalpha(Character) || Character == '_' || isalnum(Character))
                return lexer_identifier(fil, Character);
        else if (Character == '\'' || Character == '"')
                return lexer_character(fil, Character);
        else
        {
                token_t Tok = lexer_operator(fil, Character);
                if (Tok.Class == LEXER_TOKEN_COMMENT)
                {
                        while (Character != '\n')
                                Character = lexer_getch(fil);
                        lexer_getch(fil);
                        return lexer_next(fil);
                }
                else if (Tok.Class == LEXER_TOKEN_LMULTICOMMENT)
                {
                        while (Tok.Class != LEXER_TOKEN_RMULTICOMMENT)
                        {
                                Tok = lexer_next(fil);
                        }

                        return lexer_next(fil);
                }
                return Tok;
        }
        return none;
}
