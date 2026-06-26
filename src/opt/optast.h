
// optimise the AST directly
// for instance, 1+1+1 => 2+1 => 3
// also remove statements with no effect. (unless they are marked as volatile (future feature))

#ifndef OPTAST_H
#define OPTAST_H

#include "parser/parser.h"

node_t *opt_ast(node_t *root); // constructs a new tree

#endif
