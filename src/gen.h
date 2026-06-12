
#ifndef GEN_H
#define GEN_H

#include "parser.h"
#include "sym.h"
#include <stddef.h>

#define REGISTER_COUNT (8)
#define REGISTER_STACK (32)

typedef struct
{
        symbol_table_t  sym_table[MAX_SCOPE_DEPTH];
        size_t          sym_scope;
        bool            reg_alloc[REGISTER_COUNT];
        size_t          reg_alloc_ref[REGISTER_COUNT];
        size_t          reg_idx_stack[REGISTER_STACK];
        size_t          reg_idx_stack_sp;
        token_stream_t *stream;
        node_t         *node;
        FILE           *output;
} gen_t;

void gen_to_file(token_stream_t *stream, node_t *root, FILE *file);
void gen_init(FILE *file);
size_t gen_alloc_reg(gen_t *const gen);
void gen_free_reg(gen_t *const gen, size_t index);
size_t gen_pop(gen_t *const gen);
void gen_newexpr(gen_t *const gen);
void gen_push(gen_t *const gen, size_t index);
void gen_node(gen_t *gen, node_t *node);

#endif
