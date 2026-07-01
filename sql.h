#ifndef EMARALD_SQL_H
#define EMARALD_SQL_H

#include "interpreter.h"
#include <stdbool.h>

/* ==================== SQL Integration Layer ==================== */

/* SQL connection handle */
typedef struct SQLConnection SQLConnection;

/* SQL query result */
typedef struct {
    char** columns;
    int column_count;
    Value** rows;
    int row_count;
    bool has_error;
    char error_message[256];
} SQLResult;

/* SQL query types */
typedef enum {
    SQL_SELECT,
    SQL_INSERT,
    SQL_UPDATE,
    SQL_DELETE,
    SQL_CREATE,
    SQL_DROP,
} SQLQueryType;

/* ==================== Public API ==================== */

/* Connection management */
SQLConnection* sql_connect(const char* connection_string);
void sql_disconnect(SQLConnection* conn);
bool sql_is_connected(SQLConnection* conn);

/* Query execution */
SQLResult* sql_execute(SQLConnection* conn, const char* query);
SQLResult* sql_execute_prepared(SQLConnection* conn, const char* query, 
                                int param_count, Value* params);
void sql_result_free(SQLResult* result);

/* Query parsing */
SQLQueryType sql_parse_query_type(const char* query);
bool sql_is_valid_query(const char* query);

/* Parameter binding */
void sql_bind_int(SQLConnection* conn, int index, int64_t value);
void sql_bind_float(SQLConnection* conn, int index, double value);
void sql_bind_string(SQLConnection* conn, int index, const char* value);
void sql_bind_null(SQLConnection* conn, int index);

/* Result conversion to Emarald values */
Value sql_result_to_array(SQLResult* result);
Value sql_result_get_cell(SQLResult* result, int row, int col);

/* Error handling */
const char* sql_get_error(SQLConnection* conn);
void sql_clear_error(SQLConnection* conn);

#endif /* EMARALD_SQL_H */
