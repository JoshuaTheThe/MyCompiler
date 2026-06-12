
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

typedef enum
{
        BASETYPE_NONE,
        BASETYPE_INT64,
        BASETYPE_INT32,
        BASETYPE_INT16,
        BASETYPE_INT8,
        _BASETYPE_CNT,
} basetype_t;

typedef enum
{
        LEVEL_NONE,
        LEVEL_PTR,
        LEVEL_FUNCTION,
        LEVEL_ARRAY,
} type_level_form_t;

typedef struct
{
        type_level_form_t kind;
        size_t            depth_or_length;
} type_level_t;

typedef struct
{
        basetype_t   base;
        type_level_t levels[8];
        size_t       level_count;
} type_t;

typedef struct
{
        char   name[IDENTIFIER_SIZE];
        long   offset;
        type_t type;
        bool   reference_by_name; // for functions, or global variables
} symbol_t;

typedef struct
{
        symbol_t *items;
        size_t    count, capacity;
        long      currentoffset;
} symbol_table_t;

symbol_t *sym_create(symbol_table_t *table, char name[static IDENTIFIER_SIZE], type_t type);
symbol_t *sym_find(size_t scope, symbol_table_t tables[static scope], token_stream_t *stream, char name[IDENTIFIER_SIZE]);
void sym_clean(symbol_table_t *table);

#endif
