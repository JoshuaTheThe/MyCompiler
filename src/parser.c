
#include "parser.h"
#include "lexer.h"
#include "error.h"
#include "sym.h"
#include <error.h>

node_t *new_node(token_t token, node_kind_t kind)
{
        node_t *new = calloc(1, sizeof(*new));
        if (!new)
        {
                abort();
        }

        new->kind  = kind;
        new->token = token;
        return new;
}

node_t *parse_keyword(token_stream_t *stream)
{
        token_t token = *stream->Current;
        if (stream->Current->Next->Class == LEXER_TOKEN_SEMICOLON)
        {
                lexer_expect(stream, LEXER_TOKEN_IDENTIFIER);
                lexer_expect(stream, LEXER_TOKEN_SEMICOLON);
                node_t *node  = new_node(token, NODE_DECLARATION);
                return node;
        }

        return NULL;
}

void append_node(node_t *root, node_t *n)
{
        for (;root->next;root=root->next);
        root->next = n;
        n->next  = NULL;
}

void clean_nodes(node_t *root)
{
        node_t *prev = NULL;
        while (root)
        {
                if (prev)
                {
                        clean_nodes(prev->left);
                        clean_nodes(prev->right);
                        free(prev);
                }

                prev = root;
                root = root->next;
        }

        if (prev)
        {
                clean_nodes(prev->left);
                clean_nodes(prev->right);
                free(prev);
        }
}

node_t *parse_stmt(token_stream_t *stream)
{
        token_t *token = stream->Current;
        node_t *node = NULL;
        switch (token->Class)
        {
                case LEXER_TOKEN_IDENTIFIER:
                        if ((node = parse_keyword(stream)))
                                break;
                        goto expr;
                default:
                expr:
                        node = parse_expr(stream);
                        lexer_expect(stream, LEXER_TOKEN_SEMICOLON);
                        break;
        }

        return node;
}

node_t *parse_assignment(token_stream_t *stream)
{
        node_t *left = parse_equivilance(stream);

        if (lexer_accept(stream, LEXER_TOKEN_EQUAL))
        {
                token_t tok = *stream->Current->Prev;
                node_t *right = parse_assignment(stream);
                node_t *node = new_node(tok, NODE_ASSIGN);
                node->left = left;
                node->right = right;
                return node;
        }

        return left;
}

node_t *parse_equivilance(token_stream_t *stream)
{
        node_t *left = parse_relational(stream);

        while (lexer_accept(stream, LEXER_TOKEN_EQUALS) ||
               lexer_accept(stream, LEXER_TOKEN_NOTEQ))
        {
                token_t tok = *stream->Current->Prev;
                node_t *right = parse_relational(stream);
                node_t *node = new_node(tok, NODE_BINOP);
                node->left = left;
                node->right = right;
                left = node;
        }

        return left;
}

node_t *parse_relational(token_stream_t *stream)
{
        node_t *left = parse_additive(stream);

        while (lexer_accept(stream, LEXER_TOKEN_LESS) ||
               lexer_accept(stream, LEXER_TOKEN_GREATER) ||
               lexer_accept(stream, LEXER_TOKEN_LESSEQ) ||
               lexer_accept(stream, LEXER_TOKEN_GREATEREQ))
        {
                token_t tok = *stream->Current->Prev;
                node_t *right = parse_additive(stream);
                node_t *node = new_node(tok, NODE_BINOP);
                node->left = left;
                node->right = right;
                left = node;
        }

        return left;
}

node_t *parse_additive(token_stream_t *stream)
{
        node_t *left = parse_multiplicitive(stream);

        while (lexer_accept(stream, LEXER_TOKEN_PLUS) ||
               lexer_accept(stream, LEXER_TOKEN_MINUS))
        {
                token_t tok = *stream->Current->Prev;
                node_t *right = parse_multiplicitive(stream);
                node_t *node = new_node(tok, NODE_BINOP);
                node->left = left;
                node->right = right;
                left = node;
        }

        return left;
}

node_t *parse_multiplicitive(token_stream_t *stream)
{
        node_t *left = parse_prefix(stream);

        while (lexer_accept(stream, LEXER_TOKEN_ASTERISK) ||
               lexer_accept(stream, LEXER_TOKEN_PERCENT) ||
               lexer_accept(stream, LEXER_TOKEN_SLASH))
        {
                token_t tok = *stream->Current->Prev;
                node_t *right = parse_prefix(stream);
                node_t *node = new_node(tok, NODE_BINOP);
                node->left = left;
                node->right = right;
                left = node;
        }

        return left;
}

node_t *parse_prefix(token_stream_t *stream)
{
        size_t deref = 0;
        while (lexer_accept(stream, LEXER_TOKEN_ASTERISK))
                deref++;

        node_t *node = parse_primary(stream);

        while (deref--)
        {
                token_t tok = {.Class = LEXER_TOKEN_ASTERISK};
                node_t *deref_node = new_node(tok, NODE_PREFIX);
                deref_node->left = node;
                deref_node->right = NULL;
                node = deref_node;
        }

        return node;
}

node_t *parse_primary(token_stream_t *stream)
{
        token_t token = *lexer_consume(stream);
        node_t *node = NULL;

        switch (token.Class)
        {
                case LEXER_TOKEN_IDENTIFIER:
                {
                        node = new_node(token, NODE_IDENTIFIER);
                        symbol_t *sym = sym_find(stream, token.Identifier);
                        break;
                }

                case LEXER_TOKEN_INTEGER_LITERAL:
                        node = new_node(token, NODE_INTEGER);
                        break;

                case LEXER_TOKEN_LPAREN:
                {
                        node = parse_expr(stream);
                        lexer_expect(stream, LEXER_TOKEN_RPAREN);
                        node_t *group = new_node(token, NODE_GROUP);
                        group->left = node;
                        node = group;
                        break;
                }

                case LEXER_TOKEN_CHAR_LITERAL:
                        node = new_node(token, NODE_INTEGER);
                        break;

                default:
                        comperror(stream, token, "expected primary");
                        node = new_node(token, NODE_INTEGER);
                        break;
        }

        node_t *suffix = parse_suffix(stream);
        if (suffix)
        {
                suffix->left = node;
                node = suffix;
        }

        return node;
}

node_t *parse_suffix(token_stream_t *stream)
{
        if (lexer_accept(stream, LEXER_TOKEN_LSBRACKET))
        {
                token_t bracket = *stream->Current->Prev;
                node_t *index = parse_expr(stream);
                lexer_expect(stream, LEXER_TOKEN_RSBRACKET);

                node_t *node = new_node(bracket, NODE_SUFFIX);
                node->right = index;

                node_t *next = parse_suffix(stream);
                if (next)
                {
                        node->left = next;
                }

                return node;
        }

        return NULL;
}

node_t *parse_expr(token_stream_t *stream)
{
        node_t *left = parse_assignment(stream);

        while (lexer_accept(stream, LEXER_TOKEN_COMMA))
        {
                token_t tok = *stream->Current->Prev;
                node_t *right = parse_assignment(stream);
                node_t *comma = new_node(tok, NODE_MULTIEXPR);
                comma->left = left;
                comma->right = right;
                left = comma;
        }

        return left;
}
