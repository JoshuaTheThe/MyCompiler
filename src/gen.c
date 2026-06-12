
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

const char *byte_arg_reg_name[6] = {
        "dil",
        "sil",
        "dl",
        "cl",
        "r8b",
        "r9b",
};

const char *word_arg_reg_name[6] = {
        "di",
        "si",
        "dx",
        "cx",
        "r8w",
        "r9w",
};

const char *dword_arg_reg_name[6] = {
        "edi",
        "esi",
        "dx",
        "cx",
        "r8w",
        "r9w",
};

const char *qword_arg_reg_name[6] = {
        "rdi",
        "rsi",
        "rdx",
        "rcx",
        "r8",
        "r9",
};

const char *byte_reg_name[REGISTER_COUNT] = {
        "bl",
        "r11b",
        "r12b",
        "r13b",
        "r14b",
        "r15b",
};

const char *word_reg_name[REGISTER_COUNT] = {
        "bx",
        "r11w",
        "r12w",
        "r13w",
        "r14w",
        "r15w",
};

const char *dword_reg_name[REGISTER_COUNT] = {
        "ebx",
        "r11d",
        "r12d",
        "r13d",
        "r14d",
        "r15d",
};

const char *qword_reg_name[REGISTER_COUNT] = {
        "rbx",
        "r11",
        "r12",
        "r13",
        "r14",
        "r15",
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

type_t gen_strip_type(type_t type)
{
        for (size_t i = 1; i < sizeof(type.levels) / sizeof(*type.levels); ++i)
        {
                type.levels[i - 1] = type.levels[i];
                type.level_count -= 1;
        }

        return type;
}

type_t gen_lea(type_t type)
{
        if (type.levels[0].kind == LEVEL_PTR)
            type.levels[0].depth_or_length += 1;
        else
        {
                for (size_t i = 0; i < sizeof(type.levels) / sizeof(*type.levels) - 1; ++i)
                {
                        type.levels[i + 1] = type.levels[i];
                }

                type.levels[0] = (type_level_t){.depth_or_length = 1, .kind = LEVEL_PTR};
                type.level_count += 1;
        }

        return type;
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
        fprintf(file, "\t.global  main\n");
        fprintf(file, "_start:\n");
        fprintf(file, "\tandq $-16, %%rsp\n");
        fprintf(file, "\tcall main\n");
        fprintf(file, "\tmovq %%rax, %%rdi\n");
        fprintf(file, "\tmovq $60, %%rax\n");
        fprintf(file, "\tsyscall\n");
        fprintf(file, "1:\tjmp 1b\n");
        fprintf(file, "__ret:\n");
        fprintf(file, "\tmovq %%rbp, %%rsp\n");
        fprintf(file, "\tpopq %%rbp\n");
        fprintf(file, "\tretq\n");
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
                        bool old = gen->lea_over_deref;
                        gen->lea_over_deref = false;
                        gen_node(gen, node->left);
                        gen->lea_over_deref = old;

                        size_t index = gen_pop(gen);
                        type_t type = gen_get_type(gen, index);
                        type = gen_deref(type);
                        if (type.base == BASETYPE_NONE)
                                comperror(gen->stream, node->token, "cant dereference a non pointer");
                        if (gen->lea_over_deref && type.level_count > 0 && type.levels[0].kind == LEVEL_PTR)
                                fprintf(gen->output, "\tmovq (%%%s), %%%s\n", qword_reg_name[index], qword_reg_name[index]);
                        else if (!gen->lea_over_deref)
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
                gen_pop(gen);
        memset(gen->reg_alloc, 0, sizeof(gen->reg_alloc));
        memset(gen->reg_alloc_ref, 0, sizeof(gen->reg_alloc_ref));
}

size_t gen_label(gen_t *gen)
{
        return gen->label++;
}

type_t gen_node_to_type(node_t *node)
{
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


        return type;
}

void gen_node(gen_t *gen, node_t *node)
{
        if (!node || !gen)
                return;
        gen->node = node;
        if (node->stmt)
        {
                gen_newexpr(gen);
                gen->tail_is_return = false;
        }
        switch (node->kind)
        {
                case NODE_FUNCTION:
                {
                        fprintf(gen->output, "%s:\n", node->token.Identifier);
                        fprintf(gen->output, "\tendbr64\n");
                        fprintf(gen->output, "\tpushq %%rbp\n");
                        fprintf(gen->output, "\tmovq %%rsp, %%rbp\n");
                        type_t type = gen_node_to_type(node);
                        type.level_count++;
                        for (size_t i = 0; i < type.level_count - 1; ++i)
                                type.levels[i] = type.levels[i + 1];
                        type.levels[0].depth_or_length = 0;
                        type.levels[0].kind = LEVEL_FUNCTION;
                        symbol_t *sym = sym_create(&gen->sym_table[gen->sym_scope], node->token.Identifier, type);
                        sym->reference_by_name = true;
                        gen_node(gen, node->left);
                        if (!gen->tail_is_return)
                                fprintf(gen->output, "\tjmp __ret\n");
                        break;
                }
                case NODE_IF:
                {
                        gen_node(gen, node->left);
                        size_t cond      = gen_pop(gen);
                        size_t size      = gen_sizeof(gen->reg_types[cond]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        size_t skip_path = gen_label(gen);
                        size_t else_path = gen_label(gen);
                        fprintf(gen->output, "\ttest %%%s, %%%s\n", (*reg_names)[cond], (*reg_names)[cond]);
                        fprintf(gen->output, "\tjz l%ld\n", else_path);
                        gen_node(gen, node->right);
                        fprintf(gen->output, "\tjmp l%ld\n", skip_path);
                        fprintf(gen->output, "l%ld:\n", else_path);
                        gen_node(gen, node->extra);
                        fprintf(gen->output, "l%ld:\n", skip_path);
                        break;
                }
                case NODE_BLOCK:
                        gen_enter(gen);
                        gen_node(gen, node->left);
                        gen_leave(gen);
                        break;
                case NODE_IDENTIFIER:
                {
                        gen->stream->Current = &node->token;
                        symbol_t *sym = sym_find(gen->sym_scope, gen->sym_table, gen->stream, node->token.Identifier);
                        size_t reg = gen_alloc_reg(gen);
                        size_t size  = gen_sizeof(sym->type);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        gen_push(gen, reg);
                        gen_set_type(gen, reg, sym->type);
                        if (!sym->reference_by_name)
                                if (!gen->lea_over_deref)
                                        fprintf(gen->output, "\tmov %ld(%%rbp), %%%s\n", sym->offset, (*reg_names)[reg]);
                                else
                                {
                                        fprintf(gen->output, "\tlea %ld(%%rbp), %%%s\n", sym->offset, (*reg_names)[reg]);
                                }
                        else
                                fprintf(gen->output, "\tlea *%s, %%%s\n", sym->name, (*reg_names)[reg]); // always lea for function
                        break;
                }

                case NODE_RETURN:
                {
                        gen_node(gen, node->left);
                        size_t result = gen_pop(gen);
                        // TODO! function return type check
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(8);
                        fprintf(gen->output, "\tmovq %%%s, %%rax\n", (*reg_names)[result]);
                        fprintf(gen->output, "\tjmp __ret\n");
                        gen->tail_is_return = true;
                        break;
                }

                case NODE_SUFFIX:
                {
                        //gen_node(gen, node->right); // arguments
                        gen_node(gen, node->left);
                        size_t function = gen_pop(gen);
                        if (gen->reg_types[function].level_count <= 0 || gen->reg_types[function].levels[0].kind != LEVEL_FUNCTION)
                        {
                                comperror(gen->stream, node->token, "cannot call into non function");
                        }

                        size_t size  = gen_sizeof(gen->reg_types[function]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);

                        fprintf(gen->output, "\tcall %%%s\n", (*reg_names)[function]);
                        fprintf(gen->output, "\tmov %%rax, %%%s\n", (*reg_names)[function]);
                        gen_push(gen, function);
                        gen_set_type(gen, function, gen_strip_type(gen->reg_types[function]));
                        break;
                }

                case NODE_DECLARATION:
                {

                        type_t type  = gen_node_to_type(node->left);
                        size_t size  = gen_sizeof(type);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        symbol_t *sym = sym_create(&gen->sym_table[gen->sym_scope], node->token.Identifier, type);
                        sym->reference_by_name = gen->sym_scope == 0;
                        if (gen->sym_scope > 0)
                        {
                                gen_node(gen, node->right);
                                size_t value = gen_expect(gen, type);
                                fprintf(gen->output, "\tsubq $%ld, %%rsp\n", size);
                                fprintf(gen->output, "\tmov %%%s, %ld(%%rbp)\n", (*reg_names)[value], sym->offset);
                                gen_push(gen, value);
                        }
                        break;
                }

                case NODE_ASSIGN:
                {
                        gen_node(gen, node->right);
                        gen->lea_over_deref = true;
                        gen_node(gen, node->left);
                        gen->lea_over_deref = false;
                        size_t address = gen_pop(gen);
                        size_t value   = gen_expect(gen, gen->reg_types[address]);
                        size_t size    = gen_sizeof(gen->reg_types[address]);
                        const char *(*reg_names)[REGISTER_COUNT] = gen_find_names_for(size);
                        fprintf(gen->output, "\tmov %%%s, (%%%s)\n", (*reg_names)[value], (*reg_names)[address]);
                        gen_push(gen, value);
                        break;
                }

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
                        type_t type = gen_node_to_type(node);
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
        gen_node(&gen, root);
        do
        {
                if (gen.sym_scope == 0)
                {
                        fprintf(gen.output, "\t.section .bss\n");
                        for (size_t i = 0; i < gen.sym_table[0].count; ++i)
                        {
                                if (gen.sym_table[0].items[i].type.levels[0].kind != LEVEL_FUNCTION)
                                        fprintf(gen.output, "%s: .space %ld\n", gen.sym_table[0].items[i].name, gen_sizeof(gen.sym_table[0].items[i].type));
                        }
                }

                sym_clean(&gen.sym_table[gen.sym_scope]);
                if (gen.sym_scope > 0)
                        gen.sym_scope -= 1;
        }
        while (gen.sym_scope);
}

void gen_enter(gen_t *gen)
{
        if (gen->sym_scope >= MAX_SCOPE_DEPTH)
        {
                comperror(gen->stream, gen->node->token, "scope overflow");
        }

        gen->sym_scope++;
        sym_clean(&gen->sym_table[gen->sym_scope]);
        gen->sym_table[gen->sym_scope].currentoffset = -8;
}

void gen_leave(gen_t *gen)
{
        if (gen->sym_scope <= 0)
        {
                comperror(gen->stream, gen->node->token, "scope underflow");
        }

        sym_clean(&gen->sym_table[gen->sym_scope]);
        gen->sym_scope--;
}
