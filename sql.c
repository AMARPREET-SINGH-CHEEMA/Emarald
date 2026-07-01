#include "sql.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ==================== SQL Integration Layer Implementation ==================== */

typedef struct {
    char connection_string[512];
    bool connected;
    char error_message[256];
    char* last_query;
    void* native_handle;  /* Platform-specific database handle */
} SQLConnectionImpl;

struct SQLConnection {
    SQLConnectionImpl impl;
};

/* SQL connection management */
SQLConnection* sql_connect(const char* connection_string) {
    if (!connection_string) {
        error_report(ERR_IO, 0, "Connection string cannot be NULL");
        return NULL;
    }
    
    SQLConnection* conn = malloc(sizeof(SQLConnection));
    if (!conn) {
        error_report(ERR_IO, 0, "Memory allocation failed for SQL connection");
        return NULL;
    }
    
    strncpy(conn->impl.connection_string, connection_string, sizeof(conn->impl.connection_string) - 1);
    conn->impl.connection_string[sizeof(conn->impl.connection_string) - 1] = '\0';
    
    /* Validate connection string format */
    if (strncmp(connection_string, "postgres://", 11) != 0 &&
        strncmp(connection_string, "sqlite://", 9) != 0 &&
        strncmp(connection_string, "mysql://", 8) != 0) {
        error_report_formatted(ERR_IO, 0, "Unknown database driver in: %s", connection_string);
        conn->impl.connected = false;
        return conn;
    }
    
    conn->impl.connected = true;
    conn->impl.last_query = NULL;
    conn->impl.native_handle = NULL;
    memset(conn->impl.error_message, 0, sizeof(conn->impl.error_message));
    
    return conn;
}

void sql_disconnect(SQLConnection* conn) {
    if (!conn) return;
    
    if (conn->impl.last_query) {
        free(conn->impl.last_query);
        conn->impl.last_query = NULL;
    }
    
    conn->impl.connected = false;
    free(conn);
}

bool sql_is_connected(SQLConnection* conn) {
    return conn && conn->impl.connected;
}

/* Query execution */
SQLResult* sql_execute(SQLConnection* conn, const char* query) {
    if (!conn || !query) {
        error_report(ERR_RUNTIME, 0, "Invalid SQL connection or query");
        return NULL;
    }
    
    SQLResult* result = malloc(sizeof(SQLResult));
    if (!result) {
        error_report(ERR_COMPILATION, 0, "Memory allocation failed for SQL result");
        return NULL;
    }
    
    memset(result, 0, sizeof(SQLResult));
    
    if (!sql_is_connected(conn)) {
        snprintf(result->error_message, sizeof(result->error_message), 
                "Not connected to database");
        result->has_error = true;
        error_report(ERR_RUNTIME, 0, result->error_message);
        return result;
    }
    
    /* Validate query */
    if (!sql_is_valid_query(query)) {
        snprintf(result->error_message, sizeof(result->error_message), 
                "Invalid SQL query syntax");
        result->has_error = true;
        error_report(ERR_SYNTAX, 0, result->error_message);
        return result;
    }
    
    /* Store query for debugging */
    if (conn->impl.last_query) {
        free(conn->impl.last_query);
    }
    conn->impl.last_query = malloc(strlen(query) + 1);
    if (conn->impl.last_query) {
        strcpy(conn->impl.last_query, query);
    }
    
    /* Simulate successful query execution (mock implementation) */
    result->row_count = 0;
    result->column_count = 0;
    result->has_error = false;
    
    return result;
}

SQLResult* sql_execute_prepared(SQLConnection* conn, const char* query, 
                                int param_count, Value* params) {
    if (!conn || !query || !params) {
        error_report(ERR_RUNTIME, 0, "Invalid prepared statement parameters");
        return NULL;
    }
    
    /* Build parameterized query */
    SQLResult* result = sql_execute(conn, query);
    
    /* Validate parameter count */
    if (param_count < 0) {
        if (result) {
            result->has_error = true;
            snprintf(result->error_message, sizeof(result->error_message),
                    "Invalid parameter count: %d", param_count);
        }
    }
    
    return result;
}

void sql_result_free(SQLResult* result) {
    if (!result) return;
    
    if (result->columns) {
        for (int i = 0; i < result->column_count; i++) {
            free(result->columns[i]);
        }
        free(result->columns);
    }
    
    if (result->rows) {
        for (int i = 0; i < result->row_count; i++) {
            if (result->rows[i]) {
                free(result->rows[i]);
            }
        }
        free(result->rows);
    }
    
    free(result);
}

/* Query parsing */
SQLQueryType sql_parse_query_type(const char* query) {
    if (!query) return SQL_SELECT;
    
    /* Skip leading whitespace */
    while (*query && isspace(*query)) query++;
    
    if (strncasecmp(query, "SELECT", 6) == 0) return SQL_SELECT;
    if (strncasecmp(query, "INSERT", 6) == 0) return SQL_INSERT;
    if (strncasecmp(query, "UPDATE", 6) == 0) return SQL_UPDATE;
    if (strncasecmp(query, "DELETE", 6) == 0) return SQL_DELETE;
    if (strncasecmp(query, "CREATE", 6) == 0) return SQL_CREATE;
    if (strncasecmp(query, "DROP", 4) == 0) return SQL_DROP;
    
    return SQL_SELECT;
}

bool sql_is_valid_query(const char* query) {
    if (!query || strlen(query) == 0) return false;
    
    /* Basic SQL syntax validation */
    const char* p = query;
    while (*p && isspace(*p)) p++;
    
    /* Check for valid SQL keywords */
    if (strncasecmp(p, "SELECT", 6) != 0 &&
        strncasecmp(p, "INSERT", 6) != 0 &&
        strncasecmp(p, "UPDATE", 6) != 0 &&
        strncasecmp(p, "DELETE", 6) != 0 &&
        strncasecmp(p, "CREATE", 6) != 0 &&
        strncasecmp(p, "DROP", 4) != 0) {
        return false;
    }
    
    /* Check for matching parentheses and quotes */
    int paren_count = 0;
    bool in_string = false;
    char string_char = 0;
    
    while (*p) {
        if (!in_string) {
            if (*p == '\'' || *p == '"') {
                in_string = true;
                string_char = *p;
            } else if (*p == '(') {
                paren_count++;
            } else if (*p == ')') {
                paren_count--;
                if (paren_count < 0) return false;
            }
        } else if (*p == string_char && (p == query || *(p-1) != '\\')) {
            in_string = false;
        }
        p++;
    }
    
    return paren_count == 0 && !in_string;
}

/* Parameter binding */
void sql_bind_int(SQLConnection* conn, int index, int64_t value) {
    if (!conn) return;
    /* In a full implementation, store bound parameters */
}

void sql_bind_float(SQLConnection* conn, int index, double value) {
    if (!conn) return;
    /* In a full implementation, store bound parameters */
}

void sql_bind_string(SQLConnection* conn, int index, const char* value) {
    if (!conn || !value) return;
    /* In a full implementation, store bound parameters with escaping */
}

void sql_bind_null(SQLConnection* conn, int index) {
    if (!conn) return;
    /* In a full implementation, mark parameter as NULL */
}

/* Result conversion */
Value sql_result_to_array(SQLResult* result) {
    if (!result) return value_nil();
    
    ObjArray* arr = array_new();
    if (!arr) return value_nil();
    
    /* Convert result rows to array of dictionaries */
    for (int i = 0; i < result->row_count; i++) {
        ObjDict* row_dict = dict_new();
        if (row_dict && result->rows[i]) {
            for (int j = 0; j < result->column_count; j++) {
                /* Store column values */
                if (result->columns && result->columns[j]) {
                    ObjString* key = string_copy(result->columns[j], 
                                                 strlen(result->columns[j]));
                    if (key && result->rows[i][j].type != VAL_ERROR) {
                        dict_set(row_dict, key, result->rows[i][j]);
                    }
                }
            }
        }
        array_push(arr, value_obj((Object*)row_dict));
    }
    
    return value_obj((Object*)arr);
}

Value sql_result_get_cell(SQLResult* result, int row, int col) {
    if (!result || row < 0 || row >= result->row_count || 
        col < 0 || col >= result->column_count) {
        return value_nil();
    }
    
    if (result->rows && result->rows[row]) {
        return result->rows[row][col];
    }
    
    return value_nil();
}

/* Error handling */
const char* sql_get_error(SQLConnection* conn) {
    if (!conn) return "Invalid connection";
    return conn->impl.error_message;
}

void sql_clear_error(SQLConnection* conn) {
    if (!conn) return;
    memset(conn->impl.error_message, 0, sizeof(conn->impl.error_message));
}
