
#ifndef GEN_H
#define GEN_H

#include "parser.h"
#include "sym.h"
#include <stddef.h>

#define REGISTER_COUNT (6)
#define REGISTER_STACK (32)

typedef struct
{
        symbol_table_t  sym_table[MAX_SCOPE_DEPTH];
        size_t          sym_scope;
        bool            reg_alloc[REGISTER_COUNT];
        type_t          reg_types[REGISTER_COUNT];
        size_t          reg_alloc_ref[REGISTER_COUNT];
        size_t          reg_idx_stack[REGISTER_STACK];
        size_t          reg_idx_stack_sp;
        token_stream_t *stream;
        node_t         *node;
        FILE           *output;
        bool            lea_over_deref;
        size_t          label;
        bool            tail_is_return;
} gen_t;

void gen_to_file(token_stream_t *stream, node_t *root, FILE *file);
void gen_init(FILE *file);
size_t gen_alloc_reg(gen_t *const gen);
void gen_free_reg(gen_t *const gen, size_t index);
size_t gen_pop(gen_t *const gen);
void gen_newexpr(gen_t *const gen);
void gen_push(gen_t *const gen, size_t index);
void gen_node(gen_t *gen, node_t *node);
void gen_set_type(gen_t *gen, size_t index, type_t type);
size_t gen_expect(gen_t *gen, type_t type);
type_t gen_get_type(gen_t *gen, size_t index);
size_t gen_sizeof(type_t type);
type_t gen_deref(type_t type);
size_t gen_sizeof_deref(type_t type);
const char *(*gen_find_names_for(size_t size))[REGISTER_COUNT]; // lmao
size_t gen_expect_a_or_b(gen_t *gen, type_t a, type_t b);
void gen_check(gen_t *gen, type_t a, type_t t);
void gen_check_a_or_b(gen_t *gen, type_t a, type_t b, type_t t);
type_t gen_lea(type_t type);

void gen_enter(gen_t *gen);
void gen_leave(gen_t *gen);
type_t gen_node_to_type(node_t *node);

extern const type_t type_integer;
extern const char *qword_reg_name[REGISTER_COUNT];
extern const char *dword_reg_name[REGISTER_COUNT];
extern const char *word_reg_name[REGISTER_COUNT];
extern const char *byte_reg_name[REGISTER_COUNT];

#endif
