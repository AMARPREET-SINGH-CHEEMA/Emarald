#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ==================== Error Reporting System Implementation ==================== */

/* Error context tracking */
typedef struct {
    int error_count;
    int warning_count;
    bool had_error;
    bool panic_mode;
    const char* current_file;
    int current_line;
} ErrorContext;

static ErrorContext error_ctx = {
    .error_count = 0,
    .warning_count = 0,
    .had_error = false,
    .panic_mode = false,
    .current_file = NULL,
    .current_line = 0
};

/* ANSI color codes for terminal output */
#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"

static const char* error_type_to_string(ErrorType type) {
    switch (type) {
        case ERR_SYNTAX:       return "Syntax Error";
        case ERR_TYPE:         return "Type Error";
        case ERR_REFERENCE:    return "Reference Error";
        case ERR_RUNTIME:      return "Runtime Error";
        case ERR_COMPILATION:  return "Compilation Error";
        case ERR_IO:           return "I/O Error";
        case WARN_UNUSED:      return "Unused Variable";
        case WARN_TYPE_MISMATCH: return "Type Mismatch";
        case WARN_DEPRECATED:  return "Deprecated";
        default:               return "Unknown Error";
    }
}

static bool is_warning(ErrorType type) {
    return type >= WARN_UNUSED;
}

void error_init(const char* filename) {
    error_ctx.current_file = filename ? filename : "<stdin>";
    error_ctx.error_count = 0;
    error_ctx.warning_count = 0;
    error_ctx.had_error = false;
    error_ctx.panic_mode = false;
}

void error_set_line(int line) {
    error_ctx.current_line = line;
}

void error_report(ErrorType type, int line, const char* message) {
    if (error_ctx.panic_mode && is_warning(type)) {
        return;
    }
    
    bool is_warn = is_warning(type);
    
    if (is_warn) {
        error_ctx.warning_count++;
        fprintf(stderr, COLOR_YELLOW "Warning" COLOR_RESET);
    } else {
        error_ctx.error_count++;
        error_ctx.had_error = true;
        if (!error_ctx.panic_mode) {
            error_ctx.panic_mode = true;
        }
        fprintf(stderr, COLOR_RED "Error" COLOR_RESET);
    }
    
    if (error_ctx.current_file && line > 0) {
        fprintf(stderr, " in %s at line %d", error_ctx.current_file, line);
    } else if (error_ctx.current_file) {
        fprintf(stderr, " in %s", error_ctx.current_file);
    }
    
    fprintf(stderr, ":\n  %s [%s]\n", message, error_type_to_string(type));
}

void error_report_at(ErrorType type, int line, int column, const char* message) {
    if (error_ctx.panic_mode && is_warning(type)) {
        return;
    }
    
    bool is_warn = is_warning(type);
    
    if (is_warn) {
        error_ctx.warning_count++;
        fprintf(stderr, COLOR_YELLOW "Warning" COLOR_RESET);
    } else {
        error_ctx.error_count++;
        error_ctx.had_error = true;
        if (!error_ctx.panic_mode) {
            error_ctx.panic_mode = true;
        }
        fprintf(stderr, COLOR_RED "Error" COLOR_RESET);
    }
    
    if (error_ctx.current_file) {
        fprintf(stderr, " in %s:%d:%d", error_ctx.current_file, line, column);
    }
    
    fprintf(stderr, ":\n  %s [%s]\n", message, error_type_to_string(type));
}

void error_report_formatted(ErrorType type, int line, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    error_report(type, line, buffer);
}

void error_report_at_token(ErrorType type, int line, const char* token_start, 
                           int token_length, const char* message) {
    if (error_ctx.panic_mode && is_warning(type)) {
        return;
    }
    
    bool is_warn = is_warning(type);
    
    if (is_warn) {
        error_ctx.warning_count++;
        fprintf(stderr, COLOR_YELLOW "Warning" COLOR_RESET);
    } else {
        error_ctx.error_count++;
        error_ctx.had_error = true;
        fprintf(stderr, COLOR_RED "Error" COLOR_RESET);
    }
    
    fprintf(stderr, " at token '");
    fwrite(token_start, 1, token_length, stderr);
    fprintf(stderr, "'");
    
    if (error_ctx.current_file && line > 0) {
        fprintf(stderr, " in %s:%d", error_ctx.current_file, line);
    }
    
    fprintf(stderr, ":\n  %s [%s]\n", message, error_type_to_string(type));
}

void error_enter_panic_mode(void) {
    error_ctx.panic_mode = true;
}

void error_exit_panic_mode(void) {
    error_ctx.panic_mode = false;
}

bool error_in_panic_mode(void) {
    return error_ctx.panic_mode;
}

int error_count(void) {
    return error_ctx.error_count;
}

int warning_count(void) {
    return error_ctx.warning_count;
}

bool error_had_error(void) {
    return error_ctx.had_error;
}

void error_print_summary(void) {
    if (error_ctx.error_count == 0 && error_ctx.warning_count == 0) {
        fprintf(stderr, COLOR_BLUE "✓" COLOR_RESET " No errors or warnings\n");
        return;
    }
    
    fprintf(stderr, "\n");
    if (error_ctx.error_count > 0) {
        fprintf(stderr, COLOR_RED "✗ %d error%s" COLOR_RESET "\n", 
                error_ctx.error_count, 
                error_ctx.error_count == 1 ? "" : "s");
    }
    if (error_ctx.warning_count > 0) {
        fprintf(stderr, COLOR_YELLOW "⚠ %d warning%s" COLOR_RESET "\n", 
                error_ctx.warning_count, 
                error_ctx.warning_count == 1 ? "" : "s");
    }
}

void error_reset(void) {
    error_ctx.error_count = 0;
    error_ctx.warning_count = 0;
    error_ctx.had_error = false;
    error_ctx.panic_mode = false;
}

/* ==================== Specific Error Messages ==================== */

void error_unexpected_token(const char* token, int line) {
    error_report_formatted(ERR_SYNTAX, line, "Unexpected token: '%s'", token);
}

void error_expected_token(const char* expected, const char* got, int line) {
    error_report_formatted(ERR_SYNTAX, line, "Expected '%s' but got '%s'", expected, got);
}

void error_undefined_variable(const char* name, int line) {
    error_report_formatted(ERR_REFERENCE, line, "Undefined variable: '%s'", name);
}

void error_type_mismatch(const char* expected, const char* got, int line) {
    error_report_formatted(ERR_TYPE, line, "Type mismatch: expected %s, got %s", expected, got);
}

void error_invalid_operands(const char* op, const char* left_type, 
                            const char* right_type, int line) {
    error_report_formatted(ERR_TYPE, line, 
                          "Invalid operands for '%s': %s and %s", op, left_type, right_type);
}

void error_function_arity(const char* func_name, int expected, int got, int line) {
    error_report_formatted(ERR_RUNTIME, line, 
                          "Function '%s' expects %d arguments but got %d", 
                          func_name, expected, got);
}

void error_index_out_of_bounds(int index, int size, int line) {
    error_report_formatted(ERR_RUNTIME, line, 
                          "Index %d out of bounds (size: %d)", index, size);
}

void error_attribute_not_found(const char* attr, const char* type, int line) {
    error_report_formatted(ERR_REFERENCE, line, 
                          "Attribute '%s' not found in type '%s'", attr, type);
}

void error_division_by_zero(int line) {
    error_report(ERR_RUNTIME, line, "Division by zero");
}

void error_file_not_found(const char* filename, int line) {
    error_report_formatted(ERR_IO, line, "File not found: '%s'", filename);
}

void warning_unused_variable(const char* name, int line) {
    error_report_formatted(WARN_UNUSED, line, "Unused variable: '%s'", name);
}

void warning_type_conversion(const char* from_type, const char* to_type, int line) {
    error_report_formatted(WARN_TYPE_MISMATCH, line, 
                          "Implicit conversion from %s to %s", from_type, to_type);
}

void warning_deprecated_feature(const char* feature, const char* replacement, int line) {
    if (replacement) {
        error_report_formatted(WARN_DEPRECATED, line, 
                              "%s is deprecated, use %s instead", feature, replacement);
    } else {
        error_report_formatted(WARN_DEPRECATED, line, "%s is deprecated", feature);
    }
}
