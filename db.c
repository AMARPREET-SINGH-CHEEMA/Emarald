#include "db.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ==================== Database Drivers Implementation ==================== */

typedef struct {
    DatabaseDriver driver;
    char connection_string[512];
    bool connected;
    char error_message[512];
    int error_code;
    int affected_rows;
    int64_t last_insert_id;
    void* native_handle;
    bool in_transaction;
} DBConnectionImpl;

struct DBConnection {
    DBConnectionImpl impl;
};

/* Detect database driver from connection string */
static DatabaseDriver detect_driver(const char* conn_str) {
    if (!conn_str) return DB_POSTGRES;
    
    if (strncmp(conn_str, "postgres://", 11) == 0) return DB_POSTGRES;
    if (strncmp(conn_str, "sqlite://", 9) == 0) return DB_SQLITE;
    if (strncmp(conn_str, "sqlite3://", 10) == 0) return DB_SQLITE;
    if (strncmp(conn_str, "mysql://", 8) == 0) return DB_MYSQL;
    if (strncmp(conn_str, "mariadb://", 10) == 0) return DB_MARIADB;
    if (strncmp(conn_str, "mongodb://", 10) == 0) return DB_MONGODB;
    
    return DB_POSTGRES;  /* Default */
}

DBConnection* db_connect(const char* connection_string) {
    if (!connection_string) {
        error_report(ERR_IO, 0, "Connection string cannot be NULL");
        return NULL;
    }
    
    DBConnection* conn = malloc(sizeof(DBConnection));
    if (!conn) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for database connection");
        return NULL;
    }
    
    memset(conn, 0, sizeof(DBConnection));
    
    strncpy(conn->impl.connection_string, connection_string, 
            sizeof(conn->impl.connection_string) - 1);
    conn->impl.connection_string[sizeof(conn->impl.connection_string) - 1] = '\0';
    
    conn->impl.driver = detect_driver(connection_string);
    conn->impl.connected = true;
    conn->impl.error_code = 0;
    conn->impl.in_transaction = false;
    
    return conn;
}

void db_disconnect(DBConnection* conn) {
    if (!conn) return;
    
    if (conn->impl.in_transaction) {
        db_rollback(conn);
    }
    
    conn->impl.connected = false;
    free(conn);
}

bool db_is_connected(DBConnection* conn) {
    return conn && conn->impl.connected;
}

DatabaseDriver db_get_driver(DBConnection* conn) {
    if (!conn) return DB_POSTGRES;
    return conn->impl.driver;
}

/* Query execution */
Value db_query(DBConnection* conn, const char* query) {
    if (!conn || !query) {
        error_report(ERR_RUNTIME, 0, "Invalid database connection or query");
        return value_nil();
    }
    
    if (!db_is_connected(conn)) {
        snprintf(conn->impl.error_message, sizeof(conn->impl.error_message),
                "Not connected to database");
        error_report(ERR_RUNTIME, 0, conn->impl.error_message);
        return value_nil();
    }
    
    /* Create result array */
    ObjArray* result = array_new();
    if (!result) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for query result");
        return value_nil();
    }
    
    /* In a full implementation, execute query and populate result */
    conn->impl.affected_rows = 0;
    conn->impl.last_insert_id = 0;
    
    return value_obj((Object*)result);
}

Value db_query_prepared(DBConnection* conn, const char* query, 
                        int param_count, Value* params) {
    if (!conn || !query || !params) {
        error_report(ERR_RUNTIME, 0, "Invalid prepared statement parameters");
        return value_nil();
    }
    
    if (param_count < 0) {
        error_report_formatted(ERR_RUNTIME, 0, "Invalid parameter count: %d", param_count);
        return value_nil();
    }
    
    /* Escape parameters and build final query */
    /* This is a stub - real implementation would use parameterized queries */
    
    return db_query(conn, query);
}

DBQueryMeta db_get_query_metadata(DBConnection* conn) {
    DBQueryMeta meta = {0};
    
    if (!conn) {
        meta.has_error = true;
        snprintf(meta.error_message, sizeof(meta.error_message), "Invalid connection");
        return meta;
    }
    
    meta.affected_rows = conn->impl.affected_rows;
    meta.last_insert_id = conn->impl.last_insert_id;
    meta.has_error = conn->impl.error_code != 0;
    strncpy(meta.error_message, conn->impl.error_message, sizeof(meta.error_message) - 1);
    
    return meta;
}

/* Transaction control */
void db_begin_transaction(DBConnection* conn) {
    if (!conn || !db_is_connected(conn)) {
        error_report(ERR_RUNTIME, 0, "Not connected to database");
        return;
    }
    
    conn->impl.in_transaction = true;
}

void db_commit(DBConnection* conn) {
    if (!conn || !db_is_connected(conn)) {
        error_report(ERR_RUNTIME, 0, "Not connected to database");
        return;
    }
    
    if (!conn->impl.in_transaction) {
        error_report(ERR_RUNTIME, 0, "No active transaction");
        return;
    }
    
    conn->impl.in_transaction = false;
}

void db_rollback(DBConnection* conn) {
    if (!conn || !db_is_connected(conn)) {
        error_report(ERR_RUNTIME, 0, "Not connected to database");
        return;
    }
    
    if (!conn->impl.in_transaction) {
        error_report(ERR_RUNTIME, 0, "No active transaction");
        return;
    }
    
    conn->impl.in_transaction = false;
}

/* Database operations */
void db_create_database(DBConnection* conn, const char* db_name) {
    if (!conn || !db_name) {
        error_report(ERR_RUNTIME, 0, "Invalid connection or database name");
        return;
    }
    
    if (!db_is_connected(conn)) {
        error_report(ERR_RUNTIME, 0, "Not connected to database");
        return;
    }
    
    /* Execute CREATE DATABASE query */
}

void db_drop_database(DBConnection* conn, const char* db_name) {
    if (!conn || !db_name) {
        error_report(ERR_RUNTIME, 0, "Invalid connection or database name");
        return;
    }
    
    if (!db_is_connected(conn)) {
        error_report(ERR_RUNTIME, 0, "Not connected to database");
        return;
    }
    
    /* Execute DROP DATABASE query */
}

void db_select_database(DBConnection* conn, const char* db_name) {
    if (!conn || !db_name) {
        error_report(ERR_RUNTIME, 0, "Invalid connection or database name");
        return;
    }
    
    if (!db_is_connected(conn)) {
        error_report(ERR_RUNTIME, 0, "Not connected to database");
        return;
    }
    
    /* Execute USE database query */
}

/* Error handling */
const char* db_get_error(DBConnection* conn) {
    if (!conn) return "Invalid connection";
    return conn->impl.error_message;
}

int db_get_error_code(DBConnection* conn) {
    if (!conn) return -1;
    return conn->impl.error_code;
}
