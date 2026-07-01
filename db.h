#ifndef EMARALD_DB_H
#define EMARALD_DB_H

#include "interpreter.h"
#include <stdbool.h>

/* ==================== Database Drivers ==================== */

/* Supported database drivers */
typedef enum {
    DB_POSTGRES,
    DB_SQLITE,
    DB_MYSQL,
    DB_MARIADB,
    DB_MONGODB,
} DatabaseDriver;

/* Database connection handle */
typedef struct DBConnection DBConnection;

/* Query result metadata */
typedef struct {
    int affected_rows;
    int64_t last_insert_id;
    bool has_error;
    char error_message[512];
} DBQueryMeta;

/* ==================== Public API ==================== */

/* Connection management */
DBConnection* db_connect(const char* connection_string);
void db_disconnect(DBConnection* conn);
bool db_is_connected(DBConnection* conn);
DatabaseDriver db_get_driver(DBConnection* conn);

/* Query execution */
Value db_query(DBConnection* conn, const char* query);
Value db_query_prepared(DBConnection* conn, const char* query, int param_count, Value* params);
DBQueryMeta db_get_query_metadata(DBConnection* conn);

/* Transaction control */
void db_begin_transaction(DBConnection* conn);
void db_commit(DBConnection* conn);
void db_rollback(DBConnection* conn);

/* Database operations */
void db_create_database(DBConnection* conn, const char* db_name);
void db_drop_database(DBConnection* conn, const char* db_name);
void db_select_database(DBConnection* conn, const char* db_name);

/* Error handling */
const char* db_get_error(DBConnection* conn);
int db_get_error_code(DBConnection* conn);

#endif /* EMARALD_DB_H */
