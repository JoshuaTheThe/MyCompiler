
#include "binop.h"
#include "gen.h"

bool gen_relational(gen_t *gen, node_t *node)
{
        switch (node->token.Class)
        {
                case LEXER_TOKEN_NOTEQ:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_pop(gen);
                        size_t lhs = gen_expect(gen, gen->reg_types[rhs]);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tcmp %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tsetne %%%s\n", byte_reg_name[lhs]);
                        fprintf(gen->output, "\tmovzx %%%s, %%%s\n", byte_reg_name[lhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_EQUALS:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_pop(gen);
                        size_t lhs = gen_expect(gen, gen->reg_types[rhs]);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tcmp %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tsete %%%s\n", byte_reg_name[lhs]);
                        fprintf(gen->output, "\tmovzx %%%s, %%%s\n", byte_reg_name[lhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_LESS:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_pop(gen);
                        size_t lhs = gen_expect(gen, gen->reg_types[rhs]);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tcmp %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tsetl %%%s\n", byte_reg_name[lhs]);
                        fprintf(gen->output, "\tmovzx %%%s, %%%s\n", byte_reg_name[lhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_GREATER:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_pop(gen);
                        size_t lhs = gen_expect(gen, gen->reg_types[rhs]);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tcmp %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tsetg %%%s\n", byte_reg_name[lhs]);
                        fprintf(gen->output, "\tmovzx %%%s, %%%s\n", byte_reg_name[lhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_LESSEQ:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_pop(gen);
                        size_t lhs = gen_expect(gen, gen->reg_types[rhs]);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tcmp %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tsetle %%%s\n", byte_reg_name[lhs]);
                        fprintf(gen->output, "\tmovzx %%%s, %%%s\n", byte_reg_name[lhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_GREATEREQ:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_pop(gen);
                        size_t lhs = gen_expect(gen, gen->reg_types[rhs]);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tcmp %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tsetge %%%s\n", byte_reg_name[lhs]);
                        fprintf(gen->output, "\tmovzx %%%s, %%%s\n", byte_reg_name[lhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                default:
                        return false;
        }
}

bool gen_binop_mdm(gen_t *gen, node_t *node) // Multiplication, Division And Modulus
{
        switch (node->token.Class)
        {
                case LEXER_TOKEN_ASTERISK:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_expect(gen, type_integer);
                        size_t lhs = gen_expect(gen, type_integer);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\txor %%rdx, %%rdx\n");
                        fprintf(gen->output, "\timul %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_SLASH:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_expect(gen, type_integer);
                        size_t lhs = gen_expect(gen, type_integer);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\txor %%rdx, %%rdx\n");
                        fprintf(gen->output, "\tidiv %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }
                case LEXER_TOKEN_PERCENT:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        size_t rhs = gen_expect(gen, type_integer);
                        size_t lhs = gen_expect(gen, type_integer);
                        size_t size = gen_sizeof(gen->reg_types[rhs]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\txor %%rdx, %%rdx\n");
                        fprintf(gen->output, "\tidiv %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        fprintf(gen->output, "\tmov %%rdx, %%%s\n", (*reg_names)[lhs]);
                        gen_push(gen, lhs);
                        gen_set_type(gen, lhs, type_integer);
                        return true;
                }

                default:
                        return false;
        }
}

void gen_binop(gen_t *gen, node_t *node)
{
        if (gen_relational(gen, node))
                return;
        if (gen_binop_mdm(gen, node))
                return;
        gen_node(gen, node->left);
        gen_node(gen, node->right);
        size_t rhs = gen_pop(gen);
        size_t lhs = gen_pop(gen);
        gen_check_a_or_b(gen, gen->reg_types[lhs], type_integer, gen->reg_types[rhs]);
        size_t size = gen_sizeof(gen->reg_types[lhs]);
        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
        switch (node->token.Class)
        {
                case LEXER_TOKEN_PLUS:
                {
                        if (gen->reg_types[lhs].level_count > 0 && gen->reg_types[lhs].levels[0].kind == LEVEL_PTR && gen_sizeof_deref(gen->reg_types[lhs]) > 1)
                        {
                                size_t sizeof_deref = gen_sizeof_deref(gen->reg_types[lhs]);
                                fprintf(gen->output, "\tshl $%d, %%%s\n", __builtin_ctz(sizeof_deref), (*reg_names)[rhs]);
                        }

                        fprintf(gen->output, "\tadd %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        break;
                }
                case LEXER_TOKEN_MINUS:
                {
                        if (gen->reg_types[lhs].level_count > 0 && gen->reg_types[lhs].levels[0].kind == LEVEL_PTR  && gen_sizeof_deref(gen->reg_types[lhs]) > 1)
                        {
                                size_t sizeof_deref = gen_sizeof_deref(gen->reg_types[lhs]);
                                fprintf(gen->output, "\tshl $%d, %%%s\n", __builtin_ctz(sizeof_deref), (*reg_names)[rhs]);
                        }

                        fprintf(gen->output, "\tsub %%%s, %%%s\n", (*reg_names)[rhs], (*reg_names)[lhs]);
                        break;
                }
                default:
                        break;
        }

        gen_push(gen, lhs);
}
