
#ifndef SYM_H
#define SYM_H

#include "lexer.h"

// tsoding
#define MAX_SCOPE_DEPTH (32)
#define da_append(xs, x)                                                             \
    do {                                                                             \
        if ((xs)->count >= (xs)->capacity) {                                         \
            if ((xs)->capacity == 0) (xs)->capacity = 256;                           \
            else (xs)->capacity *= 2;                                                \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
            if (!(xs)->items) abort();\
        }                                                                            \
                                                                                     \
        (xs)->items[(xs)->count++] = (x);                                            \
    } while (0)

typedef struct
{
        char   name[IDENTIFIER_SIZE];
        long   offset;
} symbol_t;

typedef struct
{
        symbol_t *items;
        size_t    count, capacity;
} symbol_table_t;

symbol_t *sym_create(symbol_table_t *table, char name[IDENTIFIER_SIZE]);
symbol_t *sym_find(symbol_table_t *table, token_stream_t *stream, char name[IDENTIFIER_SIZE]);
void sym_clean(symbol_table_t *table);

#endif
