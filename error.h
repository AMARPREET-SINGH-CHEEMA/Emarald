#ifndef EMARALD_ERROR_H
#define EMARALD_ERROR_H

#include <stdbool.h>

/* ==================== Error Types ==================== */

typedef enum {
    /* Errors */
    ERR_SYNTAX,
    ERR_TYPE,
    ERR_REFERENCE,
    ERR_RUNTIME,
    ERR_COMPILATION,
    ERR_IO,
    
    /* Warnings */
    WARN_UNUSED,
    WARN_TYPE_MISMATCH,
    WARN_DEPRECATED,
} ErrorType;

/* ==================== Public API ==================== */

/* Initialize error system for a file */
void error_init(const char* filename);

/* Set current line number for error context */
void error_set_line(int line);

/* Report a general error */
void error_report(ErrorType type, int line, const char* message);

/* Report an error with line and column information */
void error_report_at(ErrorType type, int line, int column, const char* message);

/* Report a formatted error message */
void error_report_formatted(ErrorType type, int line, const char* format, ...);

/* Report an error at a specific token */
void error_report_at_token(ErrorType type, int line, const char* token_start, 
                           int token_length, const char* message);

/* Panic mode control (prevents error cascades) */
void error_enter_panic_mode(void);
void error_exit_panic_mode(void);
bool error_in_panic_mode(void);

/* Query error state */
int error_count(void);
int warning_count(void);
bool error_had_error(void);

/* Print summary of errors and warnings */
void error_print_summary(void);

/* Reset error state */
void error_reset(void);

/* ==================== Convenience Functions ==================== */

/* Syntax errors */
void error_unexpected_token(const char* token, int line);
void error_expected_token(const char* expected, const char* got, int line);

/* Reference errors */
void error_undefined_variable(const char* name, int line);
void error_attribute_not_found(const char* attr, const char* type, int line);

/* Type errors */
void error_type_mismatch(const char* expected, const char* got, int line);
void error_invalid_operands(const char* op, const char* left_type, 
                            const char* right_type, int line);

/* Runtime errors */
void error_function_arity(const char* func_name, int expected, int got, int line);
void error_index_out_of_bounds(int index, int size, int line);
void error_division_by_zero(int line);

/* I/O errors */
void error_file_not_found(const char* filename, int line);

/* Warnings */
void warning_unused_variable(const char* name, int line);
void warning_type_conversion(const char* from_type, const char* to_type, int line);
void warning_deprecated_feature(const char* feature, const char* replacement, int line);

#endif /* EMARALD_ERROR_H */
