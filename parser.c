#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ==================== Parser Implementation ==================== */

typedef struct {
    Token current;
    Token previous;
    Lexer lexer;
    bool had_error;
    bool panic_mode;
} Parser;

static Parser parser;

static void error_at(Token* token, const char* message) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    fprintf(stderr, "[Line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error_at_current(const char* message) {
    error_at(&parser.current, message);
}

static void advance(void) {
    parser.previous = parser.current;
    for (;;) {
        parser.current = lexer_next_token(&parser.lexer);
        if (parser.current.type != TOKEN_ERROR) break;
        error_at_current(parser.current.start);
    }
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    error_at_current(message);
}

static Expr* make_expr(ExprType type, int line) {
    Expr* expr = malloc(sizeof(Expr));
    if (!expr) return NULL;
    expr->type = type;
    expr->line = line;
    memset(&expr->as, 0, sizeof(expr->as));
    return expr;
}

static Stmt* make_stmt(StmtType type, int line) {
    Stmt* stmt = malloc(sizeof(Stmt));
    if (!stmt) return NULL;
    stmt->type = type;
    stmt->line = line;
    memset(&stmt->as, 0, sizeof(stmt->as));
    return stmt;
}

static void append_statement(Stmt*** list, int* count, int* capacity, Stmt* stmt) {
    if (*count >= *capacity) {
        *capacity = *capacity == 0 ? 8 : *capacity * 2;
        *list = realloc(*list, sizeof(Stmt*) * (*capacity));
    }
    (*list)[(*count)++] = stmt;
}

static void append_expr(Expr*** list, int* count, int* capacity, Expr* expr) {
    if (*count >= *capacity) {
        *capacity = *capacity == 0 ? 8 : *capacity * 2;
        *list = realloc(*list, sizeof(Expr*) * (*capacity));
    }
    (*list)[(*count)++] = expr;
}

static Expr* expression(void);
static Expr* call(void);
static Expr* finish_call(Expr* callee);
static Stmt* declaration(void);
static Stmt* statement(void);
static Stmt* block_statement(void);
static Stmt* function_declaration(void);
static Stmt* if_statement(void);
static Stmt* while_statement(void);
static Stmt* return_statement(void);
static Stmt* class_declaration(void);

static ObjString* copy_identifier(void) {
    return string_copy(parser.previous.start, parser.previous.length);
}

static Expr* make_literal(Value value, int line) {
    Expr* expr = make_expr(EXPR_LITERAL, line);
    expr->as.literal = value;
    return expr;
}

static Expr* make_variable(const char* start, int length, int line) {
    Expr* expr = make_expr(EXPR_VARIABLE, line);
    expr->as.name = string_copy(start, length);
    return expr;
}

static Expr* primary(void) {
    if (match(TOKEN_TRUE)) return make_literal(value_bool(true), parser.previous.line);
    if (match(TOKEN_FALSE)) return make_literal(value_bool(false), parser.previous.line);
    if (match(TOKEN_NIL)) return make_literal(value_nil(), parser.previous.line);

    if (match(TOKEN_INT)) {
        char buffer[64] = {0};
        memcpy(buffer, parser.previous.start, parser.previous.length);
        int64_t value = strtoll(buffer, NULL, 10);
        return make_literal(value_int(value), parser.previous.line);
    }

    if (match(TOKEN_FLOAT)) {
        char buffer[64] = {0};
        memcpy(buffer, parser.previous.start, parser.previous.length);
        double value = strtod(buffer, NULL);
        return make_literal(value_float(value), parser.previous.line);
    }

    if (match(TOKEN_STRING)) {
        int len = parser.previous.length - 2;
        if (len < 0) len = 0;
        ObjString* string = string_copy(parser.previous.start + 1, len);
        if (!string) return make_literal(value_nil(), parser.previous.line);
        Expr* expr = make_expr(EXPR_LITERAL, parser.previous.line);
        expr->as.literal = value_obj((Object*)string);
        return expr;
    }

    if (match(TOKEN_IDENTIFIER)) {
        return make_variable(parser.previous.start, parser.previous.length, parser.previous.line);
    }

    if (match(TOKEN_LPAREN)) {
        Expr* expr = expression();
        consume(TOKEN_RPAREN, "Expect ')' after expression.");
        return expr;
    }

    error_at_current("Expect expression.");
    return make_literal(value_nil(), parser.current.line);
}

static Expr* finish_call(Expr* callee) {
    Expr** arguments = NULL;
    int arg_count = 0;
    int arg_capacity = 0;

    if (!check(TOKEN_RPAREN)) {
        do {
            Expr* argument = expression();
            append_expr(&arguments, &arg_count, &arg_capacity, argument);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')' after arguments.");

    Expr* expr = make_expr(EXPR_CALL, parser.previous.line);
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.arg_count = arg_count;
    return expr;
}

static Expr* call(void) {
    Expr* expr = primary();
    for (;;) {
        if (match(TOKEN_LPAREN)) {
            expr = finish_call(expr);
            continue;
        }
        break;
    }
    return expr;
}

static Expr* unary(void) {
    if (match(TOKEN_NOT) || match(TOKEN_MINUS)) {
        TokenType op = parser.previous.type;
        Expr* right = unary();
        Expr* expr = make_expr(EXPR_UNARY, parser.previous.line);
        expr->as.unary.op = op;
        expr->as.unary.right = right;
        return expr;
    }
    return call();
}

static Expr* factor(void) {
    Expr* expr = unary();
    while (match(TOKEN_SLASH) || match(TOKEN_STAR) || match(TOKEN_PERCENT)) {
        TokenType op = parser.previous.type;
        Expr* right = unary();
        Expr* binary = make_expr(EXPR_BINARY, parser.previous.line);
        binary->as.binary.left = expr;
        binary->as.binary.right = right;
        binary->as.binary.op = op;
        expr = binary;
    }
    return expr;
}

static Expr* term(void) {
    Expr* expr = factor();
    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        TokenType op = parser.previous.type;
        Expr* right = factor();
        Expr* binary = make_expr(EXPR_BINARY, parser.previous.line);
        binary->as.binary.left = expr;
        binary->as.binary.right = right;
        binary->as.binary.op = op;
        expr = binary;
    }
    return expr;
}

static Expr* comparison(void) {
    Expr* expr = term();
    while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL) || match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL)) {
        TokenType op = parser.previous.type;
        Expr* right = term();
        Expr* binary = make_expr(EXPR_BINARY, parser.previous.line);
        binary->as.binary.left = expr;
        binary->as.binary.right = right;
        binary->as.binary.op = op;
        expr = binary;
    }
    return expr;
}

static Expr* equality(void) {
    Expr* expr = comparison();
    while (match(TOKEN_NOT_EQUAL) || match(TOKEN_EQUAL_EQUAL)) {
        TokenType op = parser.previous.type;
        Expr* right = comparison();
        Expr* binary = make_expr(EXPR_BINARY, parser.previous.line);
        binary->as.binary.left = expr;
        binary->as.binary.right = right;
        binary->as.binary.op = op;
        expr = binary;
    }
    return expr;
}

static Expr* logical_and(void) {
    Expr* expr = equality();
    while (match(TOKEN_AND)) {
        TokenType op = parser.previous.type;
        Expr* right = equality();
        Expr* binary = make_expr(EXPR_BINARY, parser.previous.line);
        binary->as.binary.left = expr;
        binary->as.binary.right = right;
        binary->as.binary.op = op;
        expr = binary;
    }
    return expr;
}

static Expr* logical_or(void) {
    Expr* expr = logical_and();
    while (match(TOKEN_OR)) {
        TokenType op = parser.previous.type;
        Expr* right = logical_and();
        Expr* binary = make_expr(EXPR_BINARY, parser.previous.line);
        binary->as.binary.left = expr;
        binary->as.binary.right = right;
        binary->as.binary.op = op;
        expr = binary;
    }
    return expr;
}

static Expr* assignment(void) {
    Expr* expr = logical_or();
    if (match(TOKEN_EQUAL)) {
        Expr* value = assignment();
        if (expr->type == EXPR_VARIABLE) {
            Expr* assign = make_expr(EXPR_ASSIGN, parser.previous.line);
            assign->as.assign.target = expr;
            assign->as.assign.value = value;
            return assign;
        }
        error_at_current("Invalid assignment target.");
    }
    return expr;
}

static Expr* expression(void) {
    return assignment();
}

static Stmt* expression_statement(void) {
    Expr* expr = expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    Stmt* stmt = make_stmt(STMT_EXPR, expr->line);
    stmt->as.expression = expr;
    return stmt;
}

static Stmt* block_statement(void) {
    Stmt** statements = NULL;
    int statement_count = 0;
    int statement_capacity = 0;

    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        append_statement(&statements, &statement_count, &statement_capacity, declaration());
    }

    consume(TOKEN_RBRACE, "Expect '}' after block.");

    Stmt* stmt = make_stmt(STMT_BLOCK, parser.previous.line);
    stmt->as.block.statements = statements;
    stmt->as.block.count = statement_count;
    return stmt;
}

static Stmt* function_declaration(void) {
    consume(TOKEN_IDENTIFIER, "Expect function name.");
    ObjString* name = copy_identifier();

    consume(TOKEN_LPAREN, "Expect '(' after function name.");

    ObjString** params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!check(TOKEN_RPAREN)) {
        do {
            consume(TOKEN_IDENTIFIER, "Expect parameter name.");
            if (param_count >= param_capacity) {
                param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                params = realloc(params, sizeof(ObjString*) * param_capacity);
            }
            params[param_count++] = copy_identifier();
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')' after parameters.");
    Stmt* body = block_statement();

    Stmt* stmt = make_stmt(STMT_FUNC_DECL, parser.previous.line);
    stmt->as.func_decl.name = name;
    stmt->as.func_decl.params = params;
    stmt->as.func_decl.param_count = param_count;
    stmt->as.func_decl.body = body;
    return stmt;
}

static Stmt* class_declaration(void) {
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    ObjString* name = copy_identifier();
    Stmt* body = block_statement();

    Stmt* stmt = make_stmt(STMT_CLASS_DECL, parser.previous.line);
    stmt->as.class_decl.name = name;
    stmt->as.class_decl.body = body;
    return stmt;
}

static Stmt* if_statement(void) {
    consume(TOKEN_LPAREN, "Expect '(' after 'if'.");
    Expr* condition = expression();
    consume(TOKEN_RPAREN, "Expect ')' after condition.");

    Stmt* then_branch = statement();
    Stmt* else_branch = NULL;
    if (match(TOKEN_ELSE)) {
        else_branch = statement();
    }

    Stmt* stmt = make_stmt(STMT_IF, parser.previous.line);
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_branch = then_branch;
    stmt->as.if_stmt.else_branch = else_branch;
    return stmt;
}

static Stmt* while_statement(void) {
    consume(TOKEN_LPAREN, "Expect '(' after 'while'.");
    Expr* condition = expression();
    consume(TOKEN_RPAREN, "Expect ')' after condition.");
    Stmt* body = statement();

    Stmt* stmt = make_stmt(STMT_WHILE, parser.previous.line);
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    return stmt;
}

static Stmt* return_statement(void) {
    Expr* value = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        value = expression();
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");

    Stmt* stmt = make_stmt(STMT_RETURN, parser.previous.line);
    stmt->as.return_expr = value;
    return stmt;
}

static Stmt* statement(void) {
    if (match(TOKEN_LBRACE)) return block_statement();
    if (match(TOKEN_IF)) return if_statement();
    if (match(TOKEN_WHILE)) return while_statement();
    if (match(TOKEN_RETURN)) return return_statement();
    return expression_statement();
}

static Stmt* declaration(void) {
    if (match(TOKEN_FN)) return function_declaration();
    if (match(TOKEN_CLASS)) return class_declaration();
    return statement();
}

Stmt* parse(const char* source) {
    lexer_init(&parser.lexer, source);
    parser.had_error = false;
    parser.panic_mode = false;
    parser.current.type = TOKEN_ERROR;
    parser.current.start = source;
    parser.current.length = 0;
    parser.current.line = 1;
    advance();

    Stmt** statements = NULL;
    int statement_count = 0;
    int statement_capacity = 0;

    while (!check(TOKEN_EOF)) {
        Stmt* stmt = declaration();
        if (stmt) {
            append_statement(&statements, &statement_count, &statement_capacity, stmt);
        } else {
            advance();
        }
    }

    consume(TOKEN_EOF, "Expect end of file.");
    if (parser.had_error) {
        for (int i = 0; i < statement_count; i++) {
            free(statements[i]);
        }
        free(statements);
        return NULL;
    }

    Stmt* block = make_stmt(STMT_BLOCK, parser.previous.line);
    block->as.block.statements = statements;
    block->as.block.count = statement_count;
    return block;
}
