
#include "lex/lexer.h"
#include "parser/parser.h"
#include "opt/optast.h"
#include "gen.h"

int main(int c,char **v)
{
        gen_init(stdout);
        for (int i = 1; i < c; ++i)
        {
                token_stream_t stream = lexer_create_stream(v[i]);
                lexer_analyse_file(&stream);
                node_t *root = NULL;
                while (stream.Current && stream.Current->Class != LEXER_TOKEN_EOF)
                {
                        node_t *new = parse_stmt(&stream);
                        if (!root)
                                root = new;
                        else
                                append_node(root, new);
                }

                node_t *optast = opt_ast(root);
                if (optast != root)
                        clean_nodes(root);

                gen_to_file(&stream, optast, stdout);
                clean_nodes(optast);
                lexer_close_stream(stream);
        }
}
