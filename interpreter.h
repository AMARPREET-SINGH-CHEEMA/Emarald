#ifndef EMARALD_INTERPRETER_H
#define EMARALD_INTERPRETER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ==================== Value System ==================== */

/* Emarald value types */
typedef enum {
    VAL_NIL,           /* null/None */
    VAL_BOOL,          /* boolean */
    VAL_INT,           /* 64-bit integer */
    VAL_FLOAT,         /* 64-bit float */
    VAL_STRING,        /* String object */
    VAL_ARRAY,         /* Array object */
    VAL_DICT,          /* Dictionary/Map object */
    VAL_FUNCTION,      /* Function object */
    VAL_NATIVE_FN,     /* C function */
    VAL_OBJECT,        /* Custom object instance */
    VAL_CLASS,         /* Class definition */
    VAL_ERROR,         /* Error/Exception */
} ValueType;

/* Union type representing any Emarald value */
typedef struct Value {
    ValueType type;
    union {
        bool boolean;
        int64_t integer;
        double floating;
        void* ptr;
    } as;
} Value;

/* ==================== Object System ==================== */

/* Base object header - all heap objects start with this */
typedef struct Object {
    ValueType type;
    bool is_marked;      /* For garbage collection */
    struct Object* next; /* For linked list of all objects */
} Object;

/* String object */
typedef struct {
    Object obj;
    char* chars;
    size_t length;
    uint32_t hash;
} ObjString;

/* Array object */
typedef struct {
    Object obj;
    Value* elements;
    size_t count;
    size_t capacity;
} ObjArray;

/* Dictionary/Map object */
typedef struct {
    ObjString* key;
    Value value;
} DictEntry;

typedef struct {
    Object obj;
    DictEntry* entries;
    size_t count;
    size_t capacity;
} ObjDict;

/* Function object */
typedef struct {
    Object obj;
    ObjString* name;
    int arity;           /* Number of parameters */
    int* bytecode;
    size_t bytecode_len;
    Value* constants;    /* Constants used in bytecode */
    size_t constants_len;
} ObjFunction;

/* Native function - C callback */
typedef Value (*NativeFn)(int argc, Value* argv);

typedef struct {
    Object obj;
    ObjString* name;
    NativeFn function;
} ObjNativeFn;

/* ==================== Bytecode Opcodes ==================== */

typedef enum {
    OP_LOAD_CONST,      /* Load constant */
    OP_LOAD_LOCAL,      /* Load local variable */
    OP_STORE_LOCAL,     /* Store local variable */
    OP_LOAD_GLOBAL,     /* Load global variable */
    OP_STORE_GLOBAL,    /* Store global variable */
    OP_BUILD_ARRAY,     /* Build array from n stack elements */
    OP_BUILD_DICT,      /* Build dict from n key-value pairs */
    OP_INDEX_GET,       /* Get element at index */
    OP_INDEX_SET,       /* Set element at index */
    OP_GET_ATTR,        /* Get object attribute */
    OP_SET_ATTR,        /* Set object attribute */
    OP_CALL,            /* Call function with n args */
    OP_RETURN,          /* Return from function */
    OP_JUMP,            /* Unconditional jump */
    OP_JUMP_IF_FALSE,   /* Jump if top of stack is false */
    OP_LOOP,            /* Loop jump */
    OP_ADD,             /* Addition */
    OP_SUBTRACT,        /* Subtraction */
    OP_MULTIPLY,        /* Multiplication */
    OP_DIVIDE,          /* Division */
    OP_MODULO,          /* Modulo */
    OP_POWER,           /* Exponentiation */
    OP_NEGATE,          /* Unary negation */
    OP_NOT,             /* Logical NOT */
    OP_EQUAL,           /* Equality check */
    OP_NOT_EQUAL,       /* Inequality check */
    OP_LESS,            /* Less than */
    OP_LESS_EQUAL,      /* Less than or equal */
    OP_GREATER,         /* Greater than */
    OP_GREATER_EQUAL,   /* Greater than or equal */
    OP_AND,             /* Logical AND */
    OP_OR,              /* Logical OR */
    OP_POP,             /* Pop stack */
    OP_PRINT,           /* Print top of stack */
    OP_NIL,             /* Push nil */
    OP_TRUE,            /* Push true */
    OP_FALSE,           /* Push false */
} OpCode;

/* ==================== Lexer/Parser ==================== */

typedef enum {
    /* Literals */
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    
    /* Keywords */
    TOKEN_FN,           /* fn */
    TOKEN_IF,           /* if */
    TOKEN_ELSE,         /* else */
    TOKEN_WHILE,        /* while */
    TOKEN_FOR,          /* for */
    TOKEN_RETURN,       /* return */
    TOKEN_CLASS,        /* class */
    TOKEN_USE,          /* use (import) */
    TOKEN_CONST,        /* const */
    TOKEN_BREAK,        /* break */
    TOKEN_CONTINUE,     /* continue */
    TOKEN_MATCH,        /* match */
    TOKEN_TRY,          /* try */
    TOKEN_CATCH,        /* catch */
    TOKEN_YIELD,        /* yield */
    
    /* Operators */
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_POWER,        /* ** */
    TOKEN_EQUAL,        /* = */
    TOKEN_EQUAL_EQUAL,  /* == */
    TOKEN_NOT_EQUAL,    /* != */
    TOKEN_LESS,         /* < */
    TOKEN_LESS_EQUAL,   /* <= */
    TOKEN_GREATER,      /* > */
    TOKEN_GREATER_EQUAL,/* >= */
    TOKEN_AND,          /* && */
    TOKEN_OR,           /* || */
    TOKEN_NOT,          /* ! */
    TOKEN_ARROW,        /* -> */
    TOKEN_COLON,        /* : */
    TOKEN_SEMICOLON,    /* ; */
    TOKEN_COMMA,        /* , */
    TOKEN_DOT,          /* . */
    TOKEN_LPAREN,       /* ( */
    TOKEN_RPAREN,       /* ) */
    TOKEN_LBRACE,       /* { */
    TOKEN_RBRACE,       /* } */
    TOKEN_LBRACKET,     /* [ */
    TOKEN_RBRACKET,     /* ] */
    
    /* Special */
    TOKEN_EOF,
    TOKEN_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

/* ==================== AST Node Structures ==================== */

typedef enum {
    EXPR_LITERAL,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_ARRAY,
    EXPR_DICT,
    EXPR_INDEX,
    EXPR_ATTR,
} ExprType;

typedef struct Expr {
    ExprType type;
    /* Union for different expression types */
} Expr;

typedef enum {
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_FUNC_DECL,
    STMT_CLASS_DECL,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BLOCK,
    STMT_RETURN,
} StmtType;

typedef struct Stmt {
    StmtType type;
    /* Union for different statement types */
} Stmt;

/* ==================== Virtual Machine ==================== */

#define STACK_SIZE 256
#define FRAMES_MAX 64

typedef struct {
    ObjFunction* function;
    int* pc;              /* Program counter */
    Value* stack_base;    /* Base of local stack */
    int local_count;      /* Number of local variables */
} CallFrame;

typedef struct {
    /* Stack */
    Value stack[STACK_SIZE];
    int stack_top;
    
    /* Call frames (for function calls) */
    CallFrame frames[FRAMES_MAX];
    int frame_count;
    
    /* Global variables */
    ObjDict* globals;
    
    /* Object management */
    Object* objects;      /* Linked list of all objects */
    size_t bytes_allocated;
    size_t gc_threshold;
    
    /* String interning (for fast string comparison) */
    ObjDict* strings;
    
} VM;

/* ==================== Function Prototypes ==================== */

/* Value operations */
Value value_nil(void);
Value value_bool(bool b);
Value value_int(int64_t i);
Value value_float(double f);
Value value_obj(Object* obj);

bool value_is_falsy(Value v);
bool value_equals(Value a, Value b);
char* value_to_string(Value v);

/* String operations */
ObjString* string_new(const char* chars, size_t length);
ObjString* string_copy(const char* chars, size_t length);

/* Array operations */
ObjArray* array_new(void);
void array_push(ObjArray* arr, Value val);
Value array_get(ObjArray* arr, int index);
void array_set(ObjArray* arr, int index, Value val);

/* Dictionary operations */
ObjDict* dict_new(void);
void dict_set(ObjDict* dict, ObjString* key, Value val);
Value dict_get(ObjDict* dict, ObjString* key);
bool dict_has(ObjDict* dict, ObjString* key);

/* Lexer */
void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);

/* Parser */
Stmt* parse(const char* source);

/* VM */
void vm_init(VM* vm);
void vm_free(VM* vm);
void vm_push(VM* vm, Value val);
Value vm_pop(VM* vm);
void vm_execute(VM* vm, ObjFunction* function);

/* Native functions */
Value native_print(int argc, Value* argv);
Value native_len(int argc, Value* argv);
Value native_type(int argc, Value* argv);

#endif /* EMARALD_INTERPRETER_H */
