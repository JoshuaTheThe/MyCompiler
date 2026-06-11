
#include "sym.h"
#include "error.h"
#include "lexer.h"

symbol_table_t tables[MAX_SCOPE_DEPTH] = {0};
size_t         current_table_depth = 0;

symbol_t *sym_find(token_stream_t *stream, char name[static IDENTIFIER_SIZE])
{
        for (size_t i = 0; i < tables[current_table_depth].count; ++i)
        {
                if (!strncmp(tables[current_table_depth].items[i].name, name, sizeof(tables[current_table_depth].items[i].name)))
                {
                        return &tables[current_table_depth].items[i];
                }
        }

        comperror(stream, *stream->Current, "unknown symbol: %s", name);
        return NULL;
}

symbol_t *sym_create(char name[static IDENTIFIER_SIZE])
{
        symbol_t new = {0};
        memcpy(new.name, name, sizeof(new.name));
        da_append(&tables[current_table_depth], new);
        return &tables[current_table_depth].items[tables[current_table_depth].count-1];
}

void sym_clean(void)
{
        if (tables[current_table_depth].items)
                free(tables[current_table_depth].items);
        tables[current_table_depth] = (symbol_table_t){0};
}

void sym_push(token_stream_t *stream)
{
        if (current_table_depth < MAX_SCOPE_DEPTH)
        {
                current_table_depth++;
                return;
        }

        comperror(stream, *stream->Current, "internal - scope stack overflow!");
}

void sym_pop(token_stream_t *stream)
{
        if (current_table_depth > 0)
        {
                current_table_depth--;
                return;
        }

        comperror(stream, *stream->Current, "internal - scope stack underflow!");
}

symbol_table_t *sym_table(void)
{
        return &tables[current_table_depth];
}

size_t sym_depth(void)
{
        return current_table_depth;
}
