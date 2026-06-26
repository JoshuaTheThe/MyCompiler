
#include "parser/parser.h"
#include "lex/lexer.h"
#include "error.h"
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
                        clean_nodes(prev->extra);
                        free(prev);
                }

                prev = root;
                root = root->next;
        }

        if (prev)
        {
                clean_nodes(prev->left);
                clean_nodes(prev->right);
                clean_nodes(prev->extra);
                free(prev);
        }
}

node_t *parse_stmt(token_stream_t *stream)
{
        token_t *token = stream->Current;
        node_t *node = NULL;
        switch (token->Class)
        {
                case LEXER_TOKEN_LBRACKET:
                {
                        node_t *block = new_node(*stream->Current, NODE_BLOCK);
                        lexer_consume(stream);
                        node_t *root = NULL, *curr = NULL;
                        while (stream->Current->Class != LEXER_TOKEN_RBRACKET)
                        {
                                node_t *node = parse_stmt(stream);
                                if (!root)
                                {
                                        root = node;
                                        curr = root;
                                }
                                else if (curr)
                                {
                                        curr->next = node;
                                        curr = node;
                                }
                        }

                        lexer_consume(stream);
                        block->left = root;
                        return block;
                }
                case LEXER_TOKEN_IDENTIFIER:
                        node = parse_keyword(stream);
                        if (node)
                                break;
                        goto expr;
                default:
                expr:
                        node = parse_expr(stream);
                        lexer_expect(stream, LEXER_TOKEN_SEMICOLON);
                        break;
        }

        node->stmt = true;
        return node;
}

bool parse_type(token_stream_t *stream, node_t **node)
{
        if (stream->Current->Class == LEXER_TOKEN_IDENTIFIER &&
           (!strncmp(stream->Current->Identifier, "i64", 4) ||
            !strncmp(stream->Current->Identifier, "i32", 4) ||
            !strncmp(stream->Current->Identifier, "i16", 4) ||
            !strncmp(stream->Current->Identifier, "i8", 4)))
        {
                token_t type = *lexer_consume(stream);
                size_t depth = 0;
                while (lexer_accept(stream, LEXER_TOKEN_ASTERISK))
                        depth++;
                node_t *cast    = new_node(type, NODE_CAST);
                cast->priv.size = depth;
                *node = cast;
                return true;
        }

        return false;
}

node_t *parse_keyword(token_stream_t *stream)
{
        token_t token = *stream->Current;
        if (stream->Current &&
            !strncmp(stream->Current->Identifier, "let", 4))
        {
                lexer_expect(stream, LEXER_TOKEN_IDENTIFIER); // let
                token_t name = *lexer_expect(stream, LEXER_TOKEN_IDENTIFIER); // name
                lexer_expect(stream, LEXER_TOKEN_COLON);
                node_t *type = NULL;
                if (!parse_type(stream, &type))
                {
                        comperror(stream, *stream->Current, "no type provided for declaration");
                }

                node_t *node  = new_node(token, NODE_DECLARATION);
                if (stream->Current->Class == LEXER_TOKEN_EQUAL)
                {
                        lexer_expect(stream, LEXER_TOKEN_EQUAL);      // initial value
                        node_t *initial = parse_expr(stream);
                        node->right     = initial;
                }

                lexer_expect(stream, LEXER_TOKEN_SEMICOLON);
                node->token   = name;
                node->left    = type;
                return node;
        }
        else if (stream->Current &&
            !strncmp(stream->Current->Identifier, "fn", 3))
        {
                // type information is explicit after the name of the function
                lexer_expect(stream, LEXER_TOKEN_IDENTIFIER); // fn
                token_t name = *lexer_expect(stream, LEXER_TOKEN_IDENTIFIER); // name
                lexer_expect(stream, LEXER_TOKEN_LPAREN); // TODO! add argument parsing
                lexer_expect(stream, LEXER_TOKEN_RPAREN);
                node_t *node  = new_node(token, NODE_FUNCTION);
                node_t *type  = NULL;
                if (!parse_type(stream, &type))
                {
                        type = new_node(name, NODE_CAST); // implicit int
                        memcpy(type->token.Identifier, "i64", 4);
                }

                node_t *body  = parse_stmt(stream);
                node->token   = name;
                node->left    = body;
                node->right   = type;
                return node;
        }
        else if (stream->Current &&
            !strncmp(stream->Current->Identifier, "if", 3))
        {
                lexer_expect(stream, LEXER_TOKEN_IDENTIFIER); // if
                lexer_expect(stream, LEXER_TOKEN_LPAREN);
                node_t *expr = parse_expr(stream);
                lexer_expect(stream, LEXER_TOKEN_RPAREN);
                node_t *stmt = parse_stmt(stream);
                node_t *node  = new_node(token, NODE_IF);
                node->left    = expr;
                node->right   = stmt; // how to store else?

                if (stream->Current &&
                    stream->Current->Class == LEXER_TOKEN_IDENTIFIER &&
                    !strncmp(stream->Current->Identifier, "else", 5))
                {
                        lexer_expect(stream, LEXER_TOKEN_IDENTIFIER);
                        node->extra = parse_stmt(stream);
                }
                return node;
        }
        else if (stream->Current &&
            !strncmp(stream->Current->Identifier, "return", 7))
        {
                lexer_expect(stream, LEXER_TOKEN_IDENTIFIER); // return
                node_t *expr = parse_expr(stream);
                lexer_expect(stream, LEXER_TOKEN_SEMICOLON);
                node_t *node  = new_node(token, NODE_RETURN);
                node->left    = expr;
                return node;
        }

        return NULL;
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
        token_t tok = *stream->Current;
        if (lexer_accept(stream, LEXER_TOKEN_ASTERISK))
        {
                node_t *deref_node = new_node(tok, NODE_PREFIX);
                deref_node->left   = parse_prefix(stream);
                deref_node->right  = NULL;
                return deref_node;
        }

        return parse_suffix(stream);
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
                        break;
                }

                case LEXER_TOKEN_INTEGER_LITERAL:
                        node = new_node(token, NODE_INTEGER);
                        break;

                case LEXER_TOKEN_LPAREN:
                {
                        if (parse_type(stream, &node))
                        {
                                lexer_expect(stream, LEXER_TOKEN_RPAREN);
                                node->left = parse_prefix(stream);
                                break;
                        }

                        // group
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

        return node;
}

node_t *parse_suffix(token_stream_t *stream)
{
        // syntax sugar for *(BASE+OFF)
        node_t *base = parse_primary(stream);

        // x[y] => *(x+y)
        while (lexer_accept(stream, LEXER_TOKEN_LSBRACKET) || lexer_accept(stream, LEXER_TOKEN_LPAREN))
        {
                token_t bracket = *stream->Current->Prev;
                if (bracket.Class == LEXER_TOKEN_LSBRACKET)
                {
                        node_t *index = parse_expr(stream);
                        lexer_expect(stream, LEXER_TOKEN_RSBRACKET);

                        node_t *dereference = new_node(bracket, NODE_PREFIX);
                        dereference->token.Class = LEXER_TOKEN_ASTERISK;

                        node_t *group            = new_node(bracket, NODE_GROUP);
                        group->left              = new_node(bracket, NODE_BINOP);
                        group->left->token.Class = LEXER_TOKEN_PLUS;
                        group->left->left        = base;
                        group->left->right       = index;
                        dereference->left        = group;
                        base = dereference;
                }
                else
                {
                        //node_t *arguments = parse_expr(stream);
                        lexer_expect(stream, LEXER_TOKEN_RPAREN);

                        node_t *call = new_node(bracket, NODE_SUFFIX);
                        call->left  = base; // callee
                        call->right = NULL;//arguments;
                        base = call;
                }
        }

        return base;
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
