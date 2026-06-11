
#ifndef ERROR_H
#define ERROR_H

#include "lexer.h"

void comperror(token_stream_t *File, token_t ReferenceToken, const char *const fmt, ...);

#endif
