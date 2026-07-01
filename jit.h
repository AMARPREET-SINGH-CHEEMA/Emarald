#ifndef EMARALD_JIT_H
#define EMARALD_JIT_H

#include "interpreter.h"
#include <stdbool.h>

/* ==================== JIT Compiler ==================== */

/* JIT compilation modes */
typedef enum {
    JIT_DISABLED,
    JIT_LAZY,       /* Compile on first execution */
    JIT_EAGER,      /* Pre-compile frequently used functions */
    JIT_ADAPTIVE,   /* Monitor and compile hot paths */
} JITMode;

/* JIT compilation statistics */
typedef struct {
    int functions_compiled;
    int bytecode_instructions;
    int native_instructions;
    double compilation_time_ms;
    double speedup_factor;
} JITStats;

/* ==================== Public API ==================== */

/* JIT initialization and control */
void jit_init(JITMode mode);
void jit_shutdown(void);

/* Compilation */
void* jit_compile_function(ObjFunction* func);
void jit_compile_module(void);

/* Execution */
Value jit_execute_function(ObjFunction* func, int argc, Value* argv);

/* Optimization */
void jit_optimize_hotspots(void);
void jit_mark_hot_function(ObjFunction* func);

/* Profiling */
JITStats jit_get_stats(void);
void jit_print_stats(void);

/* Inline caching */
void jit_invalidate_cache(void);

#endif /* EMARALD_JIT_H */
