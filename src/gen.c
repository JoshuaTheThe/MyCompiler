
#include "gen.h"
#include "lexer.h"
#include "sym.h"
#include "parser.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

const char *qword_reg_name[REGISTER_COUNT] = {
        "rbx",
        "rcx",
        "rsi",
        "r8",
        "r9",
        "r10",
        "r11",
        "r12",
};

size_t gen_pop(gen_t *const gen)
{
        if (!gen)
        {
                comperror(gen->stream, gen->node->token, "could not pop register index (GeneratorExists=%s)", gen!=NULL ? "true" : "false");
                return 0;
        }

        if (gen->reg_idx_stack_sp > 0)
        {
                const size_t index = gen->reg_idx_stack[--gen->reg_idx_stack_sp];
                gen->reg_alloc_ref[index]--;
                return index;
        }
        comperror(gen->stream, gen->node->token, "could not pop register index (OutOfBounds=true, Sp=%d)", gen->reg_idx_stack_sp);
        return 0;
}

void gen_push(gen_t *const gen, size_t index)
{
        if (!gen)
        {
                comperror(gen->stream, gen->node->token, "could not pop register index (GeneratorExists=%s)", gen!=NULL ? "true" : "false");
                return;
        }

        if (index >= REGISTER_COUNT)
        {
                comperror(gen->stream, gen->node->token, "could not pop register index (OutOfBounds=true)");
                return;
        }

        if (gen->reg_idx_stack_sp+1 < REGISTER_STACK)
        {
                gen->reg_idx_stack[gen->reg_idx_stack_sp++] = index;
                gen->reg_alloc_ref[index]++;
        }
        else
                comperror(gen->stream, gen->node->token, "could not push register index (Register=%ld, OutOfBounds=true, Sp=%ld)", index, gen->reg_idx_stack_sp);
}

size_t gen_alloc_reg(gen_t *const gen)
{
        if (!gen)
        {
                comperror(gen->stream, gen->node->token, "could not allocate register (GeneratorExists=%s)", gen!=NULL ? "true" : "false");
                return 0;
        }

        // find unused
        for (size_t i = 0; i < sizeof(gen->reg_alloc) / sizeof(*gen->reg_alloc); ++i)
        {
                if (gen->reg_alloc[i])
                        continue;
                gen->reg_alloc[i]     = true;
                gen->reg_alloc_ref[i] = 0;
                return i;
        }

        // free unused but allocated
        for (size_t i = 0; i < sizeof(gen->reg_alloc) / sizeof(*gen->reg_alloc); ++i)
        {
                if (gen->reg_alloc_ref[i] != 0)
                        continue;
                gen->reg_alloc[i]     = true;
                gen->reg_alloc_ref[i] = 0;
                return i;
        }

        comperror(gen->stream, gen->node->token, "could not allocate register for expression (expression too large)");
        return 0;
}

void gen_free_reg(gen_t *const gen, size_t index)
{
        if (!gen || index >= REGISTER_COUNT)
                comperror(gen->stream, gen->node->token, "could not free register %ld (OutOfBounds=%s, GeneratorExists=%s)",
                          index, index >= REGISTER_COUNT ? "false" : "true", gen!=NULL ? "true" : "false");
        gen->reg_alloc[index] = false;
        gen->reg_alloc_ref[index] = 0;
}

void gen_init(FILE *file)
{
        if (!file) return;
        fprintf(file, "\t.section .text\n");
        fprintf(file, "\t.global  _start\n");
        fprintf(file, "_start:\n");
        fprintf(file, "\tandq $-16, %%rsp\n");
        fprintf(file, "\tcall 2f\n");
        fprintf(file, "\tmovq $60, %%rax\n");
        fprintf(file, "\tsyscall\n");
        fprintf(file, "1:\tjmp 1b\n");
}

void display_ast(node_t *root, FILE *file, size_t depth) // dump info for now
{
        if (!root || !file)
                return;
        fprintf(file, "%*.s", (int)depth, "");
        fprintf(file, "kind=%d,token={..}\n", root->kind);
        display_ast(root->left, file, depth + 1);
        display_ast(root->right, file, depth + 1);
        display_ast(root->next, file, depth);
}

static size_t node_to_i(node_t *node)
{
        if (node->kind != NODE_INTEGER)
                return 0;
        return strtol(node->token.Identifier, NULL, 0);
}

void gen_binop(gen_t *gen, node_t *node)
{
        bool integer = false;
        if (node->left->kind == NODE_INTEGER || node->left->kind == NODE_INTEGER)
                integer = true;
        else
        {
                gen_node(gen, node->left);
                gen_node(gen, node->right);
        }

        switch (node->token.Class)
        {
                case LEXER_TOKEN_PLUS:
                {
                        if (integer)
                        {
                                size_t lhs = node_to_i(node->left);
                                size_t rhs = node_to_i(node->right);
                                size_t res = gen_alloc_reg(gen);
                                fprintf(gen->output, "\tmovq $%ld, %%%s\n", lhs+rhs, qword_reg_name[res]);
                                gen_push(gen, res);
                        }
                        else
                        {
                                size_t rhs = gen_pop(gen);
                                size_t lhs = gen_pop(gen);
                                fprintf(gen->output, "\taddq %%%s, %%%s\n", qword_reg_name[rhs], qword_reg_name[lhs]);
                                gen_push(gen, lhs);
                        }
                        break;
                }
                case LEXER_TOKEN_MINUS:
                {
                        if (integer)
                        {
                                size_t lhs = node_to_i(node->left);
                                size_t rhs = node_to_i(node->right);
                                size_t res = gen_alloc_reg(gen);
                                fprintf(gen->output, "\tmovq $%ld, %%%s\n", lhs-rhs, qword_reg_name[res]);
                                gen_push(gen, res);
                        }
                        else
                        {
                                size_t rhs = gen_pop(gen);
                                size_t lhs = gen_pop(gen);
                                fprintf(gen->output, "\tsubq %%%s, %%%s\n", qword_reg_name[rhs], qword_reg_name[lhs]);
                                gen_push(gen, lhs);
                        }
                        break;
                }
                case LEXER_TOKEN_ASTERISK:
                {
                        if (integer)
                        {
                                size_t lhs = node_to_i(node->left);
                                size_t rhs = node_to_i(node->right);
                                size_t res = gen_alloc_reg(gen);
                                fprintf(gen->output, "\tmovq $%ld, %%%s\n", lhs*rhs, qword_reg_name[res]);
                                gen_push(gen, res);
                        }
                        else
                        {
                                size_t rhs = gen_pop(gen);
                                size_t lhs = gen_pop(gen);
                                fprintf(gen->output, "\txorq %%rdx, %%rdx\n");
                                fprintf(gen->output, "\timulq %%%s, %%%s\n", qword_reg_name[rhs], qword_reg_name[lhs]);
                                gen_push(gen, lhs);
                        }
                        break;
                }
                case LEXER_TOKEN_SLASH:
                {
                        if (integer)
                        {
                                size_t lhs = node_to_i(node->left);
                                size_t rhs = node_to_i(node->right);
                                size_t res = gen_alloc_reg(gen);
                                fprintf(gen->output, "\tmovq $%ld, %%%s\n", lhs/rhs, qword_reg_name[res]);
                                gen_push(gen, res);
                        }
                        else
                        {
                                size_t rhs = gen_pop(gen);
                                size_t lhs = gen_pop(gen);
                                fprintf(gen->output, "\txorq %%rdx, %%rdx\n");
                                fprintf(gen->output, "\tidivq %%%s, %%%s\n", qword_reg_name[rhs], qword_reg_name[lhs]);
                                gen_push(gen, lhs);
                        }
                        break;
                }
                case LEXER_TOKEN_PERCENT:
                {
                        if (integer)
                        {
                                size_t lhs = node_to_i(node->left);
                                size_t rhs = node_to_i(node->right);
                                size_t res = gen_alloc_reg(gen);
                                fprintf(gen->output, "\tmovq $%ld, %%%s\n", lhs/rhs, qword_reg_name[res]);
                                gen_push(gen, res);
                        }
                        else
                        {
                                size_t rhs = gen_pop(gen);
                                size_t lhs = gen_pop(gen);
                                fprintf(gen->output, "\txorq %%rdx, %%rdx\n");
                                fprintf(gen->output, "\tidivq %%%s, %%%s\n", qword_reg_name[rhs], qword_reg_name[lhs]);
                                fprintf(gen->output, "\tmovq %%rdx, %%%s\n", qword_reg_name[lhs]);
                                gen_push(gen, lhs);
                        }
                        break;
                }
                default:
                        break;
        }
}

void gen_node(gen_t *gen, node_t *node)
{
        if (!node || !gen)
                return;
        gen->node = node;
        switch (node->kind)
        {
                case NODE_BINOP:
                {
                        gen_binop(gen, node);
                        break;
                }
                case NODE_INTEGER:
                {
                        size_t index = gen_alloc_reg(gen);
                        gen_push(gen, index);
                        fprintf(gen->output, "\tmovq $%s, %%%s\n", gen->node->token.Identifier, qword_reg_name[index]);
                        break;
                }
                default:
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        break;
        }

        gen_node(gen, node->next);
}

void gen_to_file(token_stream_t *stream, node_t *root, FILE *file)
{
        gen_t gen  = {0};
        gen.stream = stream;
        gen.node   = root;
        gen.output = file;
        fprintf(file, "2:\n");
        gen_node(&gen, root);
        fprintf(file, "\tmovq %%%s, %%rdi\n", qword_reg_name[gen_pop(&gen)]);
        fprintf(file, "\tretq\n");
        while (gen.sym_scope)
        {
                sym_clean(&gen.sym_table[gen.sym_scope]);
        }
}
