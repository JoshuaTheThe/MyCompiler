
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum
{
        NODE_INTEGER,
        NODE_IDENTIFIER,
        NODE_BINOP,
        NODE_PREFIX,
        NODE_SUFFIX,
        NODE_ASSIGN,
        NODE_IF,
        NODE_WHILE,
        NODE_CALL,
        NODE_RETURN,
        NODE_BLOCK,
        NODE_DECLARATION,
        NODE_FUNCTION,
        NODE_GROUP,
        NODE_MULTIEXPR,
        NODE_CAST,
} node_kind_t;

typedef struct node
{
        token_t      token;
        node_kind_t  kind;
        bool         stmt;

        struct node *left;
        struct node *right;

        struct node *next;

        struct
        {
                size_t size;
        } priv;
} node_t;

node_t *new_node(token_t token, node_kind_t kind);
void append_node(node_t *root, node_t *n);
void clean_nodes(node_t *root);

node_t *parse_stmt(token_stream_t *stream);
node_t *parse_expr(token_stream_t *stream);
node_t *parse_assignment(token_stream_t *stream);        // ==
node_t *parse_equivilance(token_stream_t *stream);       // ==
node_t *parse_relational(token_stream_t *stream);        // <, >, <=, >=
node_t *parse_additive(token_stream_t *stream);          // +, -
node_t *parse_multiplicitive(token_stream_t *stream);    // *, /, %
node_t *parse_prefix(token_stream_t *stream);            // *, +, -
node_t *parse_primary(token_stream_t *stream);           // 69, ABCD, 'a'
node_t *parse_suffix(token_stream_t *stream);            // []
node_t *parse_keyword(token_stream_t *stream);

#endif
