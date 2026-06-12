
#include "sym.h"
#include "error.h"
#include "gen.h"
#include "lexer.h"

symbol_t *sym_find(size_t scope, symbol_table_t tables[static scope], token_stream_t *stream, char name[IDENTIFIER_SIZE])
{
        for (ssize_t t = scope; t >= 0; --t)
        {
                symbol_table_t *table = &tables[t];
                for (size_t i = 0; i < table->count; ++i)
                {
                        if (!strncmp(table->items[i].name, name, sizeof(table->items[i].name)))
                        {
                                return &table->items[i];
                        }
                }
        }

        comperror(stream, *stream->Current, "unknown symbol: %s", name);
        return NULL;
}

symbol_t *sym_create(symbol_table_t *table, char name[static IDENTIFIER_SIZE], type_t type)
{
        symbol_t new = {0};
        memcpy(new.name, name, sizeof(new.name));
        da_append(table, new);
        table->items[table->count - 1].offset = table->currentoffset;
        table->items[table->count - 1].type   = type;
        table->currentoffset -= gen_sizeof(type);
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
