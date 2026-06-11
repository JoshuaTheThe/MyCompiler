
#include "gen.h"

void display_ast(node_t *root, FILE *file, size_t depth) // dump info for now
{
        if (!root || !file)
                return;
        printf("%*.s", (int)depth, "");
        printf("kind=%d,token={..}\n", root->kind);
        display_ast(root->left, file, depth + 1);
        display_ast(root->right, file, depth + 1);
        display_ast(root->next, file, depth);
}

void gen_to_file(node_t *root, FILE *file)
{
        display_ast(root, file, 0);
}
