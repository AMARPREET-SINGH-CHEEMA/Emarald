#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ==================== Bytecode Compiler Implementation ==================== */

typedef struct {
    ObjFunction* function;
    int* bytecode;
    size_t bytecode_capacity;
    size_t bytecode_len;
    Value* constants;
    size_t constants_capacity;
    size_t constants_len;
    ObjString* locals[STACK_SIZE];
    int local_count;
    bool had_error;
} Compiler;

static Compiler compiler;

/* Add a byte to the bytecode */
static void emit_byte(int byte) {
    if (compiler.bytecode_len >= compiler.bytecode_capacity) {
        compiler.bytecode_capacity = compiler.bytecode_capacity == 0 ? 8 : compiler.bytecode_capacity * 2;
        int* new_bytecode = realloc(compiler.bytecode, compiler.bytecode_capacity * sizeof(int));
        if (!new_bytecode) {
            fprintf(stderr, "Memory allocation failed for bytecode\n");
            compiler.had_error = true;
            return;
        }
        compiler.bytecode = new_bytecode;
    }

    compiler.bytecode[compiler.bytecode_len++] = byte;
}

/* Add a 16-bit operand (split into two bytes) */
static void emit_operand(int operand) {
    emit_byte((operand >> 8) & 0xFF);
    emit_byte(operand & 0xFF);
}

/* Get current bytecode offset */
static int current_offset(void) {
    return (int)compiler.bytecode_len;
}

/* Patch a 16-bit operand at given offset */
static void patch_operand(int offset, int operand) {
    if (offset + 1 >= compiler.bytecode_len) return;
    compiler.bytecode[offset] = (operand >> 8) & 0xFF;
    compiler.bytecode[offset + 1] = operand & 0xFF;
}

/* Add a constant and return its index */
static int add_constant(Value val) {
    if (compiler.constants_len >= compiler.constants_capacity) {
        compiler.constants_capacity = compiler.constants_capacity == 0 ? 8 : compiler.constants_capacity * 2;
        Value* new_constants = realloc(compiler.constants, compiler.constants_capacity * sizeof(Value));
        if (!new_constants) {
            fprintf(stderr, "Memory allocation failed for constants\n");
            compiler.had_error = true;
            return -1;
        }
        compiler.constants = new_constants;
    }

    compiler.constants[compiler.constants_len++] = val;
    return (int)(compiler.constants_len - 1);
}

static bool string_equals(ObjString* a, ObjString* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->length != b->length) return false;
    return memcmp(a->chars, b->chars, a->length) == 0;
}

static int resolve_local(ObjString* name) {
    for (int i = compiler.local_count - 1; i >= 0; i--) {
        if (string_equals(compiler.locals[i], name)) {
            return i;
        }
    }
    return -1;
}

static void add_local(ObjString* name) {
    if (compiler.local_count >= STACK_SIZE) {
        fprintf(stderr, "Too many local variables\n");
        compiler.had_error = true;
        return;
    }
    compiler.locals[compiler.local_count++] = name;
}

/* Compile binary operation */
static void compile_binary_op(OpCode op) {
    emit_byte(op);
}

/* Compile variable reference */
static void compile_variable(const char* name, size_t length) {
    ObjString* var_name = string_copy(name, length);
    if (!var_name) {
        fprintf(stderr, "Failed to create variable name string\n");
        compiler.had_error = true;
        return;
    }

    int local_idx = resolve_local(var_name);
    if (local_idx != -1) {
        emit_byte(OP_LOAD_LOCAL);
        emit_operand(local_idx);
        return;
    }

    int const_idx = add_constant(value_obj((Object*)var_name));
    if (const_idx >= 0) {
        emit_byte(OP_LOAD_GLOBAL);
        emit_operand(const_idx);
    }
}

static void compile_variable_name(ObjString* name) {
    int local_idx = resolve_local(name);
    if (local_idx != -1) {
        emit_byte(OP_LOAD_LOCAL);
        emit_operand(local_idx);
        return;
    }

    int const_idx = add_constant(value_obj((Object*)name));
    if (const_idx >= 0) {
        emit_byte(OP_LOAD_GLOBAL);
        emit_operand(const_idx);
    }
}

static void compile_store_variable(ObjString* name) {
    int local_idx = resolve_local(name);
    if (local_idx != -1) {
        emit_byte(OP_STORE_LOCAL);
        emit_operand(local_idx);
        return;
    }

    int const_idx = add_constant(value_obj((Object*)name));
    if (const_idx >= 0) {
        emit_byte(OP_STORE_GLOBAL);
        emit_operand(const_idx);
    }
}

/* Compile array construction */
static void compile_array(int element_count) {
    emit_byte(OP_BUILD_ARRAY);
    emit_operand(element_count);
}

/* Compile dictionary construction */
static void compile_dict(int pair_count) {
    emit_byte(OP_BUILD_DICT);
    emit_operand(pair_count);
}

/* Compile index access */
static void compile_index(void) {
    emit_byte(OP_INDEX_GET);
}

/* Compile attribute access */
static void compile_attribute(const char* name, size_t length) {
    ObjString* attr_name = string_copy(name, length);
    if (!attr_name) {
        fprintf(stderr, "Failed to create attribute name string\n");
        compiler.had_error = true;
        return;
    }

    int const_idx = add_constant(value_obj((Object*)attr_name));
    if (const_idx >= 0) {
        emit_byte(OP_GET_ATTR);
        emit_operand(const_idx);
    }
}

/* Compile function call */
static void compile_call(int arg_count) {
    emit_byte(OP_CALL);
    emit_operand(arg_count);
}

/* Compile jump instruction */
static int emit_jump(OpCode op) {
    emit_byte(op);
    int offset = current_offset();
    emit_operand(0);  /* Placeholder for jump target */
    return offset;
}

/* Compile conditional jump */
static void compile_jump_if_false(void) {
    int jump_offset = emit_jump(OP_JUMP_IF_FALSE);
    /* Return offset for later patching */
    (void)jump_offset;
}

/* Compile unconditional jump */
static void compile_jump(void) {
    int jump_offset = emit_jump(OP_JUMP);
    /* Return offset for later patching */
    (void)jump_offset;
}

/* Compile loop */
static void compile_loop(int loop_start) {
    emit_byte(OP_LOOP);
    int offset = current_offset();
    int target = loop_start - offset - 2;
    emit_operand(target);
}

/* Compile unary operations */
static void compile_unary(OpCode op) {
    emit_byte(op);
}

/* Compile binary arithmetic */
static void compile_arithmetic(OpCode op) {
    emit_byte(op);
}

/* Compile comparison */
static void compile_comparison(OpCode op) {
    emit_byte(op);
}

/* Compile logical operations */
static void compile_logical(OpCode op) {
    emit_byte(op);
}

/* Compile print statement */
static void compile_print(void) {
    emit_byte(OP_PRINT);
}

/* Compile pop (discard value) */
static void compile_pop(void) {
    emit_byte(OP_POP);
}

/* Compile return statement */
static void compile_return(void) {
    emit_byte(OP_RETURN);
}

/* Initialize compiler */
static void compiler_init(void) {
    compiler.bytecode = NULL;
    compiler.bytecode_capacity = 0;
    compiler.bytecode_len = 0;
    compiler.constants = NULL;
    compiler.constants_capacity = 0;
    compiler.constants_len = 0;
    compiler.had_error = false;
}

/* Finalize compilation and create function object */
static ObjFunction* compiler_finalize(void) {
    /* Add implicit return nil */
    emit_byte(OP_NIL);
    emit_byte(OP_RETURN);
    
    if (compiler.had_error) {
        return NULL;
    }
    
    ObjFunction* func = malloc(sizeof(ObjFunction));
    if (!func) {
        fprintf(stderr, "Failed to allocate function object\n");
        return NULL;
    }
    
    func->obj.type = VAL_FUNCTION;
    func->obj.is_marked = false;
    func->obj.next = NULL;
    func->name = string_copy("chunk", 5);
    func->arity = 0;
    func->bytecode = compiler.bytecode;
    func->bytecode_len = compiler.bytecode_len;
    func->constants = compiler.constants;
    func->constants_len = compiler.constants_len;
    
    return func;
}

static void compile_expr(Expr* expr);
static void compile_stmt(Stmt* stmt);

static OpCode binary_op_code(TokenType op) {
    switch (op) {
        case TOKEN_PLUS: return OP_ADD;
        case TOKEN_MINUS: return OP_SUBTRACT;
        case TOKEN_STAR: return OP_MULTIPLY;
        case TOKEN_SLASH: return OP_DIVIDE;
        case TOKEN_PERCENT: return OP_MODULO;
        case TOKEN_POWER: return OP_POWER;
        case TOKEN_EQUAL_EQUAL: return OP_EQUAL;
        case TOKEN_NOT_EQUAL: return OP_NOT_EQUAL;
        case TOKEN_LESS: return OP_LESS;
        case TOKEN_LESS_EQUAL: return OP_LESS_EQUAL;
        case TOKEN_GREATER: return OP_GREATER;
        case TOKEN_GREATER_EQUAL: return OP_GREATER_EQUAL;
        case TOKEN_AND: return OP_AND;
        case TOKEN_OR: return OP_OR;
        default: return OP_ADD;
    }
}

static void compile_literal(Value val) {
    int const_idx = add_constant(val);
    if (const_idx >= 0) {
        emit_byte(OP_LOAD_CONST);
        emit_operand(const_idx);
    }
}

static void compile_variable_load(ObjString* name) {
    int local_idx = resolve_local(name);
    if (local_idx != -1) {
        emit_byte(OP_LOAD_LOCAL);
        emit_operand(local_idx);
        return;
    }

    int const_idx = add_constant(value_obj((Object*)name));
    if (const_idx >= 0) {
        emit_byte(OP_LOAD_GLOBAL);
        emit_operand(const_idx);
    }
}

static void compile_variable_store(ObjString* name) {
    int local_idx = resolve_local(name);
    if (local_idx != -1) {
        emit_byte(OP_STORE_LOCAL);
        emit_operand(local_idx);
        return;
    }

    int const_idx = add_constant(value_obj((Object*)name));
    if (const_idx >= 0) {
        emit_byte(OP_STORE_GLOBAL);
        emit_operand(const_idx);
    }
}

static ObjFunction* compile_function_stmt(Stmt* stmt) {
    Compiler previous = compiler;
    compiler_init();
    compiler.local_count = 0;

    for (int i = 0; i < stmt->as.func_decl.param_count; i++) {
        add_local(stmt->as.func_decl.params[i]);
    }

    compile_stmt(stmt->as.func_decl.body);
    ObjFunction* func = compiler_finalize();

    Compiler current = compiler;
    compiler = previous;
    (void)current;
    return func;
}

static void compile_expr(Expr* expr) {
    if (!expr) return;
    switch (expr->type) {
        case EXPR_LITERAL:
            compile_literal(expr->as.literal);
            break;
        case EXPR_VARIABLE:
            compile_variable_load(expr->as.name);
            break;
        case EXPR_ASSIGN:
            compile_expr(expr->as.assign.value);
            compile_variable_store(expr->as.assign.target->as.name);
            break;
        case EXPR_UNARY:
            compile_expr(expr->as.unary.right);
            if (expr->as.unary.op == TOKEN_MINUS) {
                emit_byte(OP_NEGATE);
            } else if (expr->as.unary.op == TOKEN_NOT) {
                emit_byte(OP_NOT);
            }
            break;
        case EXPR_BINARY: {
            compile_expr(expr->as.binary.left);
            compile_expr(expr->as.binary.right);
            OpCode op = binary_op_code(expr->as.binary.op);
            emit_byte(op);
            break;
        }
        case EXPR_CALL:
            for (int i = 0; i < expr->as.call.arg_count; i++) {
                compile_expr(expr->as.call.arguments[i]);
            }
            compile_expr(expr->as.call.callee);
            compile_call(expr->as.call.arg_count);
            break;
        case EXPR_ARRAY:
            for (int i = 0; i < expr->as.array.count; i++) {
                compile_expr(expr->as.array.elements[i]);
            }
            compile_array(expr->as.array.count);
            break;
        case EXPR_INDEX:
            compile_expr(expr->as.index.object);
            compile_expr(expr->as.index.index);
            compile_index();
            break;
        case EXPR_ATTR:
            compile_expr(expr->as.attr.object);
            compile_attribute(expr->as.attr.name->chars, expr->as.attr.name->length);
            break;
        default:
            break;
    }
}

static void compile_stmt(Stmt* stmt) {
    if (!stmt) return;
    switch (stmt->type) {
        case STMT_EXPR:
            compile_expr(stmt->as.expression);
            emit_byte(OP_POP);
            break;
        case STMT_BLOCK:
            for (int i = 0; i < stmt->as.block.count; i++) {
                compile_stmt(stmt->as.block.statements[i]);
            }
            break;
        case STMT_FUNC_DECL: {
            ObjFunction* func = compile_function_stmt(stmt);
            int const_idx = add_constant(value_obj((Object*)func));
            if (const_idx >= 0) {
                emit_byte(OP_LOAD_CONST);
                emit_operand(const_idx);
                compile_variable_store(stmt->as.func_decl.name);
            }
            break;
        }
        case STMT_IF: {
            compile_expr(stmt->as.if_stmt.condition);
            int else_jump = emit_jump(OP_JUMP_IF_FALSE);
            compile_stmt(stmt->as.if_stmt.then_branch);
            if (stmt->as.if_stmt.else_branch) {
                int end_jump = emit_jump(OP_JUMP);
                patch_operand(else_jump, current_offset());
                compile_stmt(stmt->as.if_stmt.else_branch);
                patch_operand(end_jump, current_offset());
            } else {
                patch_operand(else_jump, current_offset());
            }
            break;
        }
        case STMT_WHILE: {
            int loop_start = current_offset();
            compile_expr(stmt->as.while_stmt.condition);
            int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
            compile_stmt(stmt->as.while_stmt.body);
            compile_loop(loop_start);
            patch_operand(exit_jump, current_offset());
            break;
        }
        case STMT_RETURN:
            if (stmt->as.return_expr) {
                compile_expr(stmt->as.return_expr);
            } else {
                emit_byte(OP_NIL);
            }
            emit_byte(OP_RETURN);
            break;
        case STMT_CLASS_DECL:
            compile_literal(value_nil());
            compile_variable_store(stmt->as.class_decl.name);
            break;
        default:
            break;
    }
}

/* Compile a complete program from AST */
ObjFunction* compile(Stmt* statements) {
    compiler_init();

    if (!statements) {
        fprintf(stderr, "No statements to compile\n");
        compiler.had_error = true;
        return NULL;
    }

    compile_stmt(statements);
    ObjFunction* func = compiler_finalize();
    return func;
}

/* ==================== Convenience Functions ==================== */

void compiler_emit_constant_instruction(OpCode op, Value val) {
    int const_idx = add_constant(val);
    if (const_idx >= 0) {
        emit_byte(op);
        emit_operand(const_idx);
    }
}

void compiler_emit_variable_load(const char* name, size_t length) {
    compile_variable(name, length);
}

void compiler_emit_variable_store(const char* name, size_t length) {
    ObjString* var_name = string_copy(name, length);
    if (!var_name) {
        fprintf(stderr, "Failed to create variable name string\n");
        compiler.had_error = true;
        return;
    }

    int const_idx = add_constant(value_obj((Object*)var_name));
    if (const_idx >= 0) {
        emit_byte(OP_STORE_GLOBAL);
        emit_operand(const_idx);
    }
}

int compiler_current_bytecode_size(void) {
    return (int)compiler.bytecode_len;
}

int compiler_current_constant_count(void) {
    return (int)compiler.constants_len;
}
