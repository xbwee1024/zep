#pragma once

#define  _MATH_DEFINES_DEFINED
#include <string>
#include "chibi/eval.h"
#include "chibi/gc_heap.h"
struct Chibi
{
    Chibi()
    {
        ctx = NULL;
        env = NULL;
    }
    sexp_gc_var6(ctx, env, res, err, out, in);
};

sexp chibi_init(Chibi& chibi);
sexp chibi_destroy(Chibi& chibi);
void chibi_repl(Chibi& chibi, sexp env, const std::string& str);

