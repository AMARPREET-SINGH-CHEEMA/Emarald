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
    } else if (token->type == TOKEN_ERROR) {
        /* Nothing */
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error(const char* message) {
    error_at(&parser.previous, message);
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

/* Forward declarations */
static Stmt* statement(void);
static Stmt* declaration(void);
static Expr* expression(void);

/* Simple statement parsing */
static Stmt* expression_statement(void) {
    Expr* expr = expression();
    
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_EXPR;
    /* In a full implementation, store expr in stmt */
    return stmt;
}

/* Function declaration */
static Stmt* function_declaration(void) {
    consume(TOKEN_IDENTIFIER, "Expect function name");
    consume(TOKEN_LPAREN, "Expect '(' after function name");
    
    /* Parse parameters */
    if (!check(TOKEN_RPAREN)) {
        do {
            consume(TOKEN_IDENTIFIER, "Expect parameter name");
        } while (match(TOKEN_COMMA));
    }
    
    consume(TOKEN_RPAREN, "Expect ')' after parameters");
    consume(TOKEN_COLON, "Expect ':' before function body");
    
    /* Parse body (simplified - in full implementation parse full block) */
    while (!check(TOKEN_EOF) && !check(TOKEN_FN) && 
           !check(TOKEN_CLASS)) {
        statement();
    }
    
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_FUNC_DECL;
    return stmt;
}

/* If statement */
static Stmt* if_statement(void) {
    Expr* condition = expression();
    consume(TOKEN_COLON, "Expect ':' after if condition");
    
    /* Parse then branch */
    Stmt* then_branch = statement();
    Stmt* else_branch = NULL;
    
    if (match(TOKEN_ELSE)) {
        consume(TOKEN_COLON, "Expect ':' after else");
        else_branch = statement();
    }
    
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_IF;
    return stmt;
}

/* While statement */
static Stmt* while_statement(void) {
    Expr* condition = expression();
    consume(TOKEN_COLON, "Expect ':' after while condition");
    
    Stmt* body = statement();
    
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_WHILE;
    return stmt;
}

/* For statement */
static Stmt* for_statement(void) {
    consume(TOKEN_IDENTIFIER, "Expect loop variable");
    
    Expr* iterable = expression();
    consume(TOKEN_COLON, "Expect ':' after for clause");
    
    Stmt* body = statement();
    
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_FOR;
    return stmt;
}

/* Return statement */
static Stmt* return_statement(void) {
    Expr* value = NULL;
    
    if (!check(TOKEN_EOF)) {
        value = expression();
    }
    
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_RETURN;
    return stmt;
}

/* Statement parsing */
static Stmt* statement(void) {
    if (match(TOKEN_IF)) return if_statement();
    if (match(TOKEN_WHILE)) return while_statement();
    if (match(TOKEN_FOR)) return for_statement();
    if (match(TOKEN_RETURN)) return return_statement();
    
    return expression_statement();
}

/* Declaration parsing */
static Stmt* declaration(void) {
    if (match(TOKEN_FN)) return function_declaration();
    if (match(TOKEN_CLASS)) {
        /* Class declaration - not implemented yet */
        Stmt* stmt = malloc(sizeof(Stmt));
        stmt->type = STMT_CLASS_DECL;
        return stmt;
    }
    
    return statement();
}

/* Primary expression - literals, variables, grouping */
static Expr* primary(void) {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
        match(TOKEN_STRING)) {
        Expr* expr = malloc(sizeof(Expr));
        expr->type = EXPR_LITERAL;
        return expr;
    }
    
    if (match(TOKEN_IDENTIFIER)) {
        Expr* expr = malloc(sizeof(Expr));
        expr->type = EXPR_VARIABLE;
        return expr;
    }
    
    if (match(TOKEN_LPAREN)) {
        Expr* expr = expression();
        consume(TOKEN_RPAREN, "Expect ')' after expression");
        return expr;
    }
    
    if (match(TOKEN_LBRACKET)) {
        Expr* expr = malloc(sizeof(Expr));
        expr->type = EXPR_ARRAY;
        
        if (!check(TOKEN_RBRACKET)) {
            do {
                expression();
            } while (match(TOKEN_COMMA));
        }
        
        consume(TOKEN_RBRACKET, "Expect ']' after array");
        return expr;
    }
    
    error_at_current("Expect expression");
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    return expr;
}

/* Unary expressions */
static Expr* unary(void) {
    if (match(TOKEN_NOT) || match(TOKEN_MINUS)) {
        unary();
        Expr* expr = malloc(sizeof(Expr));
        expr->type = EXPR_UNARY;
        return expr;
    }
    
    return primary();
}

/* Exponentiation (right-associative) */
static Expr* power(void) {
    Expr* expr = unary();
    
    if (match(TOKEN_POWER)) {
        Expr* right = power();  /* Right-associative */
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Multiplication, division, modulo */
static Expr* factor(void) {
    Expr* expr = power();
    
    while (match(TOKEN_SLASH) || match(TOKEN_STAR) || match(TOKEN_PERCENT)) {
        Expr* right = power();
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Addition, subtraction */
static Expr* term(void) {
    Expr* expr = factor();
    
    while (match(TOKEN_MINUS) || match(TOKEN_PLUS)) {
        Expr* right = factor();
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Comparison */
static Expr* comparison(void) {
    Expr* expr = term();
    
    while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL) ||
           match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL)) {
        Expr* right = term();
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Equality */
static Expr* equality(void) {
    Expr* expr = comparison();
    
    while (match(TOKEN_NOT_EQUAL) || match(TOKEN_EQUAL_EQUAL)) {
        Expr* right = comparison();
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Logical AND */
static Expr* logical_and(void) {
    Expr* expr = equality();
    
    while (match(TOKEN_AND)) {
        Expr* right = equality();
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Logical OR */
static Expr* logical_or(void) {
    Expr* expr = logical_and();
    
    while (match(TOKEN_OR)) {
        Expr* right = logical_and();
        Expr* binary = malloc(sizeof(Expr));
        binary->type = EXPR_BINARY;
        expr = binary;
    }
    
    return expr;
}

/* Call expression - function calls and indexing */
static Expr* call(void) {
    Expr* expr = logical_or();
    
    for (;;) {
        if (match(TOKEN_LPAREN)) {
            /* Function call */
            if (!check(TOKEN_RPAREN)) {
                do {
                    expression();
                } while (match(TOKEN_COMMA));
            }
            consume(TOKEN_RPAREN, "Expect ')' after arguments");
            
            Expr* call_expr = malloc(sizeof(Expr));
            call_expr->type = EXPR_CALL;
            expr = call_expr;
        } else if (match(TOKEN_LBRACKET)) {
            /* Indexing */
            expression();
            consume(TOKEN_RBRACKET, "Expect ']' after index");
            
            Expr* index_expr = malloc(sizeof(Expr));
            index_expr->type = EXPR_INDEX;
            expr = index_expr;
        } else if (match(TOKEN_DOT)) {
            /* Attribute access */
            consume(TOKEN_IDENTIFIER, "Expect property name after '.'");
            
            Expr* attr_expr = malloc(sizeof(Expr));
            attr_expr->type = EXPR_ATTR;
            expr = attr_expr;
        } else {
            break;
        }
    }
    
    return expr;
}

/* Assignment */
static Expr* assignment(void) {
    Expr* expr = call();
    
    if (match(TOKEN_EQUAL)) {
        Expr* value = assignment();
        /* In full implementation, check expr is valid assignment target */
    }
    
    return expr;
}

/* Top-level expression parsing */
static Expr* expression(void) {
    return assignment();
}

/* Main parse function */
Stmt* parse(const char* source) {
    lexer_init(&parser.lexer, source);
    parser.had_error = false;
    parser.panic_mode = false;
    
    advance();
    
    Stmt* statements = malloc(sizeof(Stmt));
    statements->type = STMT_BLOCK;
    
    while (!check(TOKEN_EOF)) {
        declaration();
    }
    
    return statements;
}
