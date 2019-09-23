// This taken from the chibi's main .cpp showing how to bootstrap the repl
#include "chibi.h"

#if WIN32
#pragma warning(disable : 4267)
#endif

#define sexp_argv_symbol "command-line"
#define sexp_import_prefix "(import ("
#define sexp_import_suffix "))"
#define sexp_environment_prefix "(environment '("
#define sexp_environment_suffix "))"
#define sexp_trace_prefix "(module-env (load-module '("
#define sexp_trace_suffix ")))"
#define sexp_default_environment "(environment '(scheme small))"
#define sexp_advice_environment "(load-module '(chibi repl))"

#define sexp_version_string "chibi-scheme " sexp_version " \"" sexp_release_name "\" "

#define exit_failure() exit(70)
#define exit_success() exit(0)

#if SEXP_USE_PRINT_BACKTRACE_ON_SEGFAULT
#include <execinfo.h>
#include <signal.h>
void sexp_segfault_handler(int sig)
{
    void* array[10];
    size_t size;
    /* get void*'s for all entries on the stack */
    size = backtrace(array, 10);
    /* print out all the frames to stderr */
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
#endif

#define sexp_make_unblocking(ctx, port) (void)0

static sexp sexp_meta_env(sexp ctx)
{
    if (sexp_envp(sexp_global(ctx, SEXP_G_META_ENV)))
        return sexp_global(ctx, SEXP_G_META_ENV);
    return sexp_context_env(ctx);
}

static sexp sexp_param_ref(sexp ctx, sexp env, sexp name)
{
    sexp res = sexp_env_ref(ctx, env, name, SEXP_FALSE);
    return sexp_opcodep(res) ? sexp_parameter_ref(ctx, res) : NULL;
}

static sexp sexp_load_standard_params(sexp ctx, sexp e, int nonblocking)
{
    sexp_gc_var1(res);
    sexp_gc_preserve1(ctx, res);
    sexp_load_standard_ports(ctx, e, stdin, stdout, stderr, 0);
    if (nonblocking)
    {
        sexp_make_unblocking(ctx, sexp_param_ref(ctx, e, sexp_global(ctx, SEXP_G_CUR_IN_SYMBOL)));
        sexp_make_unblocking(ctx, sexp_param_ref(ctx, e, sexp_global(ctx, SEXP_G_CUR_OUT_SYMBOL)));
        sexp_make_unblocking(ctx, sexp_param_ref(ctx, e, sexp_global(ctx, SEXP_G_CUR_ERR_SYMBOL)));
    }
    res = sexp_make_env(ctx);
    sexp_env_parent(res) = e;
    sexp_context_env(ctx) = res;
    sexp_set_parameter(ctx, sexp_meta_env(ctx), sexp_global(ctx, SEXP_G_INTERACTION_ENV_SYMBOL), res);
    sexp_gc_release1(ctx);
    return res;
}

static sexp check_exception(sexp ctx, sexp res)
{
    sexp_gc_var4(err, advise, sym, tmp);
    if (res && sexp_exceptionp(res))
    {
        sexp_gc_preserve4(ctx, err, advise, sym, tmp);
        tmp = res;
        err = sexp_current_error_port(ctx);
        if (!sexp_oportp(err))
            err = sexp_make_output_port(ctx, stderr, SEXP_FALSE);
        sexp_print_exception(ctx, res, err);
        sexp_stack_trace(ctx, err);
#if SEXP_USE_MAIN_ERROR_ADVISE
        if (sexp_envp(sexp_global(ctx, SEXP_G_META_ENV)))
        {
            advise = sexp_eval_string(ctx, sexp_advice_environment, -1, sexp_global(ctx, SEXP_G_META_ENV));
            if (sexp_vectorp(advise))
            {
                advise = sexp_vector_ref(advise, SEXP_ONE);
                if (sexp_envp(advise))
                {
                    sym = sexp_intern(ctx, "repl-advise-exception", -1);
                    advise = sexp_env_ref(ctx, advise, sym, SEXP_FALSE);
                    if (sexp_procedurep(advise))
                        sexp_apply(ctx, advise, tmp = sexp_list2(ctx, res, err));
                }
            }
        }
#endif
        sexp_gc_release4(ctx);
        exit_failure();
    }
    return res;
}

static sexp sexp_add_import_binding(sexp ctx, sexp env)
{
    sexp_gc_var2(sym, tmp);
    sexp_gc_preserve2(ctx, sym, tmp);
    sym = sexp_intern(ctx, "repl-import", -1);
    tmp = sexp_env_ref(ctx, sexp_meta_env(ctx), sym, SEXP_VOID);
    sym = sexp_intern(ctx, "import", -1);
    sexp_env_define(ctx, env, sym, tmp);
    sexp_gc_release3(ctx);
    return env;
}

static sexp sexp_load_standard_repl_env(sexp ctx, sexp env, sexp k, int bootp, int nonblocking)
{
    sexp_gc_var1(e);
    sexp_gc_preserve1(ctx, e);
    e = sexp_load_standard_env(ctx, env, k);
    if (!sexp_exceptionp(e))
    {
#if SEXP_USE_MODULES
        if (!bootp)
            e = sexp_eval_string(ctx, sexp_default_environment, -1, sexp_global(ctx, SEXP_G_META_ENV));
        if (!sexp_exceptionp(e))
            sexp_add_import_binding(ctx, e);
#endif
        if (!sexp_exceptionp(e))
            e = sexp_load_standard_params(ctx, e, nonblocking);
    }
    sexp_gc_release1(ctx);
    return e;
}

static void do_init_context(sexp* ctx, sexp* env, sexp_uint_t heap_size,
    sexp_uint_t heap_max_size, sexp_sint_t fold_case)
{
    *ctx = sexp_make_eval_context(NULL, NULL, NULL, heap_size, heap_max_size);
    if (!*ctx)
    {
        fprintf(stderr, "chibi-scheme: out of memory\n");
        exit_failure();
    }
#if SEXP_USE_FOLD_CASE_SYMS
    sexp_global(*ctx, SEXP_G_FOLD_CASE_P) = sexp_make_boolean(fold_case);
#endif
    *env = sexp_context_env(*ctx);
}

#define handle_noarg()                                                                             \
    if (argv[i][2] != '\0')                                                                        \
    {                                                                                              \
        fprintf(stderr, "option %c doesn't take any argument but got: %s\n", argv[i][1], argv[i]); \
        exit_failure();                                                                            \
    }

#define init_context()                                                        \
    if (!chibi.ctx)                                                                 \
        do                                                                    \
        {                                                                     \
            do_init_context(&chibi.ctx, &chibi.env, heap_size, heap_max_size, fold_case); \
            sexp_gc_preserve4(chibi.ctx, tmp, sym, args, chibi.env);                      \
    } while (0)

#define load_init(bootp)                                                                                       \
    if (!init_loaded++)                                                                                        \
        do                                                                                                     \
        {                                                                                                      \
            init_context();                                                                                    \
            check_exception(ctx, env = sexp_load_standard_repl_env(ctx, env, SEXP_SEVEN, bootp, nonblocking)); \
    } while (0)


void chibi_repl(Chibi& chibi, sexp env, const std::string& str)
{
    sexp_gc_var3(tmp, res, obj);
    sexp_gc_preserve3(chibi.ctx, tmp, res, obj);

    //sexp_maybe_block_port(ctx, in, 1);
    obj = sexp_eval_string(chibi.ctx, str.c_str(), -1, NULL); // (ctx, in);
    //sexp_maybe_unblock_port(ctx, in);
    //if (obj == SEXP_EOF)
    //    break;
    if (sexp_exceptionp(obj))
    {
        sexp_print_exception(chibi.ctx, obj, chibi.err);
    }
    else
    {
        sexp_context_top(chibi.ctx) = 0;
        if (!(sexp_idp(obj) || sexp_pairp(obj) || sexp_nullp(obj)))
            obj = sexp_make_lit(chibi.ctx, obj);
        tmp = sexp_env_bindings(env);
        res = sexp_eval(chibi.ctx, obj, env);
#if SEXP_USE_WARN_UNDEFS
        sexp_warn_undefs(chibi.ctx, sexp_env_bindings(env), tmp, res);
#endif
        if (res && sexp_exceptionp(res))
        {
            sexp_print_exception(chibi.ctx, res, chibi.err);
            if (res != sexp_global(chibi.ctx, SEXP_G_OOS_ERROR))
                sexp_stack_trace(chibi.ctx, chibi.err);
        }
        else if (res != SEXP_VOID)
        {
            sexp_write(chibi.ctx, res, chibi.out);
            sexp_write_char(chibi.ctx, '\n', chibi.out);
        }
    }
    sexp_gc_release3(chibi.ctx);
}


static void sexp_add_path (sexp ctx, const char *str) {
  const char *colon;
  if (str && *str) {
    colon = strchr(str, ':');
    if (colon)
      sexp_add_path(ctx, colon+1);
    else
      colon = str + strlen(str);
    sexp_push(ctx, sexp_global(ctx, SEXP_G_MODULE_PATH), SEXP_VOID);
    sexp_car(sexp_global(ctx, SEXP_G_MODULE_PATH))
      = sexp_c_string(ctx, str, colon-str);
    sexp_immutablep(sexp_global(ctx, SEXP_G_MODULE_PATH)) = 1;
  }
}
sexp chibi_init(Chibi& chibi)
{
    sexp_sint_t fold_case = SEXP_DEFAULT_FOLD_CASE_SYMS;
    sexp_uint_t heap_size = 0, heap_max_size = SEXP_MAXIMUM_HEAP_SIZE;

    sexp_scheme_init();

    sexp_gc_var3(tmp, sym, args);
 
    args = SEXP_NULL;

    // TODO Preserve env
    int bootp = 0;
    bool nonblocking = true;
    do_init_context(&chibi.ctx, &chibi.env, heap_size, heap_max_size, fold_case);
    check_exception(chibi.ctx, chibi.env=sexp_load_standard_repl_env(chibi.ctx, chibi.env, SEXP_SEVEN, bootp, nonblocking)); \
    sexp_gc_preserve3(chibi.ctx, tmp, sym, args);
    sexp_context_tracep(chibi.ctx) = 1;

    args = sexp_cons(chibi.ctx, tmp = sexp_c_string(chibi.ctx, "Zep", -1), args);
    sexp_set_parameter(chibi.ctx, sexp_meta_env(chibi.ctx), sym = sexp_intern(chibi.ctx, sexp_argv_symbol, -1), args);

    chibi.in = sexp_param_ref(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_IN_SYMBOL));
    chibi.out = sexp_param_ref(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_OUT_SYMBOL));
    chibi.err = sexp_param_ref(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_ERR_SYMBOL));
    /*if (in == NULL || out == NULL)
    {
        /* Err
    }
    */
    if (chibi.err == NULL)
    {
        chibi.err = chibi.out;
    }
    sexp_port_sourcep(chibi.in) = 1;
    return chibi.err;
}

sexp chibi_destroy(Chibi& chibi)
{
    if (sexp_destroy_context(chibi.ctx) == SEXP_FALSE)
    {
        //fprintf(stderr, "destroy_context error\n");
        return SEXP_FALSE;
    }
    return SEXP_TRUE;
}
