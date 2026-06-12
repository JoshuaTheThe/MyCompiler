
#include "gen.h"
#include "lexer.h"
#include "sym.h"
#include "parser.h"
#include "binop.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

const type_t type_integer = {
        .base        = BASETYPE_INT64,
        .level_count = 0,
        .levels      = {{0}},
};

const char *byte_reg_name[REGISTER_COUNT] = {
        "bl",
        "cl",
        "r8b",
        "r9b",
        "r10b",
        "r11b",
        "r12b",
        "r13b",
};

const char *word_reg_name[REGISTER_COUNT] = {
        "bx",
        "cx",
        "r8w",
        "r9w",
        "r10w",
        "r11w",
        "r12w",
        "r13w",
};

const char *dword_reg_name[REGISTER_COUNT] = {
        "ebx",
        "ecx",
        "r8d",
        "r9d",
        "r10d",
        "r11d",
        "r12d",
        "r13d",
};

const char *qword_reg_name[REGISTER_COUNT] = {
        "rbx",
        "rcx",
        "r8",
        "r9",
        "r10",
        "r11",
        "r12",
        "r13",
};


const char *(*gen_find_names_for(size_t size))[REGISTER_COUNT]
{
        switch (size)
        {
                case 8:
                        return &qword_reg_name;
                case 4:
                        return &dword_reg_name;
                case 2:
                        return &word_reg_name;
                case 1:
                        return &byte_reg_name;
                default:
                        return NULL;
        }
}

size_t gen_sizeof(type_t type)
{
        static const size_t sizes[_BASETYPE_CNT] = {
                0,8,4,2,1
        };

        if (type.base >= _BASETYPE_CNT)
                return 0; // UB for now
        if (type.level_count > 0)
                return sizes[1]; // sizeof(highest_word) or sizeof(uintptr)
        return sizes[type.base];
}

type_t gen_deref(type_t type)
{
        if (!(type.base != BASETYPE_NONE && type.level_count > 0 && type.levels[0].kind == LEVEL_PTR))
        {
                return (type_t){0};
        }
        if (type.levels[0].depth_or_length > 0)
            type.levels[0].depth_or_length -= 1;
        if (type.levels[0].depth_or_length == 0)
        {
                for (size_t i = 1; i < sizeof(type.levels) / sizeof(*type.levels); ++i)
                {
                        type.levels[i - 1] = type.levels[i];
                }

                type.levels[sizeof(type.levels) / sizeof(*type.levels) - 1] = (type_level_t){0};
                type.level_count -= 1;
        }

        return type;
}

size_t gen_sizeof_deref(type_t type)
{
        return gen_sizeof(gen_deref(type));
}

size_t gen_expect_a_or_b(gen_t *gen, type_t a, type_t b)
{
        size_t index = gen_pop(gen); // wont return on fail, no error check needed
        if (memcmp(&gen->reg_types[index], &a, sizeof(type_t)) && memcmp(&gen->reg_types[index], &b, sizeof(type_t)))
        {
                comperror(gen->stream, gen->node->token, "unexpected type, expected {%d, %d} or {%d, %d} but got {%d, %d}", a.base, a.level_count, b.base, b.level_count, gen->reg_types[index].base, gen->reg_types[index].level_count);
        }
        return index;
}

void gen_check_a_or_b(gen_t *gen, type_t a, type_t b, type_t t)
{
        if (memcmp(&t, &a, sizeof(type_t)) && memcmp(&t, &b, sizeof(type_t)))
        {
                comperror(gen->stream, gen->node->token, "unexpected type, expected {%d, %d} or {%d, %d} but got {%d, %d}", a.base, a.level_count, b.base, b.level_count, t.base, t.level_count);
        }
}

void gen_check(gen_t *gen, type_t a, type_t t)
{
        if (memcmp(&t, &a, sizeof(type_t)))
        {
                comperror(gen->stream, gen->node->token, "unexpected type, expected {%d, %d} but got {%d, %d}", a.base, a.level_count, t.base, t.level_count);
        }
}

size_t gen_expect(gen_t *gen, type_t type)
{
        size_t index = gen_pop(gen); // wont return on fail, no error check needed
        if (memcmp(&gen->reg_types[index], &type, sizeof(type_t)))
        {
                comperror(gen->stream, gen->node->token, "unexpected type, expected {%d, %d} but got {%d, %d}", type.base, type.level_count, gen->reg_types[index].base, gen->reg_types[index].level_count);
        }
        return index;
}

void gen_set_type(gen_t *gen, size_t index, type_t type)
{
        if (!gen || index >= REGISTER_COUNT)
                comperror(gen->stream, gen->node->token, "could not free register %ld (OutOfBounds=%s, GeneratorExists=%s)",
                          index, index >= REGISTER_COUNT ? "false" : "true", gen!=NULL ? "true" : "false");
        gen->reg_types[index] = type;
}

type_t gen_get_type(gen_t *gen, size_t index)
{
        if (!gen || index >= REGISTER_COUNT)
                comperror(gen->stream, gen->node->token, "could not free register %ld (OutOfBounds=%s, GeneratorExists=%s)",
                          index, index >= REGISTER_COUNT ? "false" : "true", gen!=NULL ? "true" : "false");
        return gen->reg_types[index];
}

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
        fprintf(file, "\tmovq %%rax, %%rdi\n");
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

void gen_prefix(gen_t *gen, node_t *node)
{
        switch (node->token.Class)
        {
                case LEXER_TOKEN_ASTERISK:
                {
                        gen_node(gen, node->left);
                        size_t index = gen_pop(gen);
                        type_t type = gen_get_type(gen, index);
                        type = gen_deref(type);
                        if (type.base == BASETYPE_NONE)
                                comperror(gen->stream, node->token, "cant dereference a non pointer");
                        fprintf(gen->output, "\tmovq (%%%s), %%%s\n", qword_reg_name[index], qword_reg_name[index]);
                        gen_set_type(gen, index, type);
                        gen_push(gen, index);
                        break;
                }
                default: break;
        }
}

void gen_newexpr(gen_t *const gen)
{
        while (gen->reg_idx_stack_sp > 0)
                gen_expect(gen, type_integer);
        memset(gen->reg_alloc, 0, sizeof(gen->reg_alloc));
        memset(gen->reg_alloc_ref, 0, sizeof(gen->reg_alloc_ref));
}

void gen_node(gen_t *gen, node_t *node)
{
        if (!node || !gen)
                return;
        gen->node = node;
        if (node->stmt)
                gen_newexpr(gen);
        switch (node->kind)
        {
                case NODE_PREFIX:
                {
                        gen_prefix(gen, node);
                        break;
                }

                case NODE_BINOP:
                {
                        gen_binop(gen, node);
                        break;
                }
                case NODE_INTEGER:
                {
                        size_t index = gen_alloc_reg(gen);
                        gen_push(gen, index);
                        gen_set_type(gen, index, type_integer);
                        fprintf(gen->output, "\tmovq $%s, %%%s\n", gen->node->token.Identifier, qword_reg_name[index]);
                        break;
                }
                case NODE_CAST:
                {
                        gen_node(gen, node->left);
                        gen_node(gen, node->right);
                        type_t type = {0};
                        if (!strncmp(node->token.Identifier, "i64", 4))
                        {
                                type.base = BASETYPE_INT64;
                        }
                        else if (!strncmp(node->token.Identifier, "i32", 4))
                        {
                                type.base = BASETYPE_INT32;
                        }
                        else if (!strncmp(node->token.Identifier, "i16", 4))
                        {
                                type.base = BASETYPE_INT16;
                        }
                        else if (!strncmp(node->token.Identifier, "i8", 3))
                        {
                                type.base = BASETYPE_INT8;
                        }

                        // change in future to allow multiple levels
                        if (node->priv.size > 0)
                        {
                                type.levels[0].kind            = LEVEL_PTR;
                                type.levels[0].depth_or_length = node->priv.size;
                                type.level_count               = 1;
                        }

                        size_t index = gen_pop(gen);
                        gen_set_type(gen, index, type);
                        gen_push(gen, index);
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
        fprintf(file, "\tmovq %%%s, %%rax\n", qword_reg_name[gen_pop(&gen)]);
        fprintf(file, "\tretq\n");
        while (gen.sym_scope)
        {
                sym_clean(&gen.sym_table[gen.sym_scope]);
        }
}
