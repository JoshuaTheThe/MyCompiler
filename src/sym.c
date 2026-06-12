
#include "sym.h"
#include "error.h"
#include "lexer.h"

symbol_t *sym_find(symbol_table_t *table, token_stream_t *stream, char name[static IDENTIFIER_SIZE])
{
        for (size_t i = 0; i < table->count; ++i)
        {
                if (!strncmp(table->items[i].name, name, sizeof(table->items[i].name)))
                {
                        return &table->items[i];
                }
        }

        comperror(stream, *stream->Current, "unknown symbol: %s", name);
        return NULL;
}

symbol_t *sym_create(symbol_table_t *table, char name[static IDENTIFIER_SIZE])
{
        symbol_t new = {0};
        memcpy(new.name, name, sizeof(new.name));
        da_append(table, new);
        return &table->items[table->count-1];
}

void sym_clean(symbol_table_t *table)
{
        if (!table)
                return;
        if (table->items)
                free(table->items);
        *table = (symbol_table_t){0};
}
