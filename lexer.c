#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ==================== Lexer Implementation ==================== */

void lexer_init(Lexer* lexer, const char* source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char lexer_peek(Lexer* lexer) {
    return *lexer->current;
}

static char lexer_peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static char lexer_advance(Lexer* lexer) {
    return *lexer->current++;
}

static bool lexer_match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    return true;
}

static void skip_whitespace(Lexer* lexer) {
    while (true) {
        char c = lexer_peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                lexer_advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer_advance(lexer);
                break;
            case '/':
                if (lexer_peek_next(lexer) == '/') {
                    /* Single-line comment */
                    while (!is_at_end(lexer) && lexer_peek(lexer) != '\n') {
                        lexer_advance(lexer);
                    }
                } else if (lexer_peek_next(lexer) == '*') {
                    /* Multi-line comment */
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    while (!is_at_end(lexer)) {
                        if (lexer_peek(lexer) == '\n') lexer->line++;
                        if (lexer_peek(lexer) == '*' && 
                            lexer_peek_next(lexer) == '/') {
                            lexer_advance(lexer);
                            lexer_advance(lexer);
                            break;
                        }
                        lexer_advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    return token;
}

static bool is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

static bool is_identifier_continue(char c) {
    return isalnum(c) || c == '_';
}

static TokenType get_keyword_type(const char* start, int length) {
    /* Simple keyword lookup - could be optimized with a hash table */
    struct {
        const char* keyword;
        TokenType type;
    } keywords[] = {
        {"fn", TOKEN_FN},
        {"if", TOKEN_IF},
        {"else", TOKEN_ELSE},
        {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR},
        {"return", TOKEN_RETURN},
        {"class", TOKEN_CLASS},
        {"use", TOKEN_USE},
        {"const", TOKEN_CONST},
        {"break", TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE},
        {"match", TOKEN_MATCH},
        {"try", TOKEN_TRY},
        {"catch", TOKEN_CATCH},
        {"yield", TOKEN_YIELD},
        {NULL, TOKEN_IDENTIFIER}
    };
    
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        int kw_len = strlen(keywords[i].keyword);
        if (length == kw_len && 
            strncmp(start, keywords[i].keyword, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token scan_identifier(Lexer* lexer) {
    while (is_identifier_continue(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    int length = (int)(lexer->current - lexer->start);
    TokenType type = get_keyword_type(lexer->start, length);
    return make_token(lexer, type);
}

static Token scan_number(Lexer* lexer) {
    /* Integer part */
    while (isdigit(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    /* Fractional part */
    if (lexer_peek(lexer) == '.' && isdigit(lexer_peek_next(lexer))) {
        lexer_advance(lexer);  /* consume '.' */
        while (isdigit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
        return make_token(lexer, TOKEN_FLOAT);
    }
    
    return make_token(lexer, TOKEN_INT);
}

static Token scan_string(Lexer* lexer, char quote) {
    while (!is_at_end(lexer) && lexer_peek(lexer) != quote) {
        if (lexer_peek(lexer) == '\n') lexer->line++;
        if (lexer_peek(lexer) == '\\') {
            lexer_advance(lexer);  /* Skip escape character */
        }
        lexer_advance(lexer);
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }
    
    lexer_advance(lexer);  /* Closing quote */
    return make_token(lexer, TOKEN_STRING);
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = lexer_advance(lexer);
    
    /* Identifiers and keywords */
    if (is_identifier_start(c)) {
        lexer->current--;
        return scan_identifier(lexer);
    }
    
    /* Numbers */
    if (isdigit(c)) {
        lexer->current--;
        return scan_number(lexer);
    }
    
    /* Strings */
    if (c == '"' || c == '\'') {
        return scan_string(lexer, c);
    }
    
    /* Single and multi-character operators */
    switch (c) {
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '-':
            if (lexer_match(lexer, '>')) {
                return make_token(lexer, TOKEN_ARROW);
            }
            return make_token(lexer, TOKEN_MINUS);
        case '*':
            if (lexer_match(lexer, '*')) {
                return make_token(lexer, TOKEN_POWER);
            }
            return make_token(lexer, TOKEN_STAR);
        case '/': return make_token(lexer, TOKEN_SLASH);
        case '%': return make_token(lexer, TOKEN_PERCENT);
        case '=':
            if (lexer_match(lexer, '=')) {
                return make_token(lexer, TOKEN_EQUAL_EQUAL);
            }
            return make_token(lexer, TOKEN_EQUAL);
        case '!':
            if (lexer_match(lexer, '=')) {
                return make_token(lexer, TOKEN_NOT_EQUAL);
            }
            return make_token(lexer, TOKEN_NOT);
        case '<':
            if (lexer_match(lexer, '=')) {
                return make_token(lexer, TOKEN_LESS_EQUAL);
            }
            return make_token(lexer, TOKEN_LESS);
        case '>':
            if (lexer_match(lexer, '=')) {
                return make_token(lexer, TOKEN_GREATER_EQUAL);
            }
            return make_token(lexer, TOKEN_GREATER);
        case '&':
            if (lexer_match(lexer, '&')) {
                return make_token(lexer, TOKEN_AND);
            }
            return error_token(lexer, "Unexpected character '&'");
        case '|':
            if (lexer_match(lexer, '|')) {
                return make_token(lexer, TOKEN_OR);
            }
            return error_token(lexer, "Unexpected character '|'");
        case ':': return make_token(lexer, TOKEN_COLON);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': return make_token(lexer, TOKEN_DOT);
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
    }
    
    return error_token(lexer, "Unexpected character");
}

/* ==================== Value Implementation ==================== */

Value value_nil(void) {
    Value val;
    val.type = VAL_NIL;
    return val;
}

Value value_bool(bool b) {
    Value val;
    val.type = VAL_BOOL;
    val.as.boolean = b;
    return val;
}

Value value_int(int64_t i) {
    Value val;
    val.type = VAL_INT;
    val.as.integer = i;
    return val;
}

Value value_float(double f) {
    Value val;
    val.type = VAL_FLOAT;
    val.as.floating = f;
    return val;
}

Value value_obj(Object* obj) {
    Value val;
    val.type = obj->type;
    val.as.ptr = (void*)obj;
    return val;
}

bool value_is_falsy(Value v) {
    switch (v.type) {
        case VAL_NIL: return true;
        case VAL_BOOL: return !v.as.boolean;
        case VAL_INT: return v.as.integer == 0;
        case VAL_FLOAT: return v.as.floating == 0.0;
        default: return false;
    }
}

bool value_equals(Value a, Value b) {
    if (a.type != b.type) return false;
    
    switch (a.type) {
        case VAL_NIL: return true;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_INT: return a.as.integer == b.as.integer;
        case VAL_FLOAT: return a.as.floating == b.as.floating;
        case VAL_STRING: {
            ObjString* s1 = (ObjString*)a.as.ptr;
            ObjString* s2 = (ObjString*)b.as.ptr;
            return s1->length == s2->length &&
                   memcmp(s1->chars, s2->chars, s1->length) == 0;
        }
        default: return a.as.ptr == b.as.ptr;
    }
}

char* value_to_string(Value v) {
    /* Note: This should use a string builder in production */
    static char buffer[256];
    
    switch (v.type) {
        case VAL_NIL:
            return "nil";
        case VAL_BOOL:
            return v.as.boolean ? "true" : "false";
        case VAL_INT:
            snprintf(buffer, sizeof(buffer), "%lld", v.as.integer);
            return buffer;
        case VAL_FLOAT:
            snprintf(buffer, sizeof(buffer), "%g", v.as.floating);
            return buffer;
        case VAL_STRING: {
            ObjString* s = (ObjString*)v.as.ptr;
            strncpy(buffer, s->chars, s->length);
            buffer[s->length] = '\0';
            return buffer;
        }
        default:
            return "[object]";
    }
}

/* ==================== String Pool Implementation ==================== */

#include <stdio.h>

ObjString* string_new(const char* chars, size_t length) {
    /* Allocate string object */
    ObjString* string = malloc(sizeof(ObjString));
    string->obj.type = VAL_STRING;
    string->obj.is_marked = false;
    string->obj.next = NULL;
    
    /* Allocate and copy character data */
    string->chars = malloc(length + 1);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    string->length = length;
    
    /* Simple hash function */
    uint32_t hash = 0;
    for (size_t i = 0; i < length; i++) {
        hash = hash * 31 + (uint8_t)chars[i];
    }
    string->hash = hash;
    
    return string;
}

ObjString* string_copy(const char* chars, size_t length) {
    return string_new(chars, length);
}

/* ==================== Array Implementation ==================== */

ObjArray* array_new(void) {
    ObjArray* array = malloc(sizeof(ObjArray));
    array->obj.type = VAL_ARRAY;
    array->obj.is_marked = false;
    array->obj.next = NULL;
    
    array->elements = malloc(sizeof(Value) * 10);
    array->count = 0;
    array->capacity = 10;
    
    return array;
}

void array_push(ObjArray* arr, Value val) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        arr->elements = realloc(arr->elements, 
                                sizeof(Value) * arr->capacity);
    }
    arr->elements[arr->count++] = val;
}

Value array_get(ObjArray* arr, int index) {
    if (index < 0 || index >= (int)arr->count) {
        return value_nil();  /* Out of bounds */
    }
    return arr->elements[index];
}

void array_set(ObjArray* arr, int index, Value val) {
    if (index >= 0 && index < (int)arr->count) {
        arr->elements[index] = val;
    }
}

/* ==================== Dictionary Implementation ==================== */

#define DICT_LOAD_FACTOR 0.75

ObjDict* dict_new(void) {
    ObjDict* dict = malloc(sizeof(ObjDict));
    dict->obj.type = VAL_DICT;
    dict->obj.is_marked = false;
    dict->obj.next = NULL;
    
    dict->entries = malloc(sizeof(DictEntry) * 16);
    dict->count = 0;
    dict->capacity = 16;
    
    return dict;
}

static bool string_equals(ObjString* a, ObjString* b) {
    if (a == b) return true;
    if (a->length != b->length || a->hash != b->hash) return false;
    return memcmp(a->chars, b->chars, a->length) == 0;
}

static int find_entry(DictEntry* entries, int capacity, 
                      ObjString* key) {
    uint32_t index = key->hash % capacity;
    
    while (true) {
        DictEntry* entry = &entries[index];
        
        if (entry->key == NULL) {
            return index;
        }
        
        if (string_equals(entry->key, key)) {
            return index;
        }
        
        index = (index + 1) % capacity;
    }
}

void dict_set(ObjDict* dict, ObjString* key, Value val) {
    if (dict->count + 1 > dict->capacity * DICT_LOAD_FACTOR) {
        /* Resize */
        int new_capacity = dict->capacity * 2;
        DictEntry* new_entries = malloc(sizeof(DictEntry) * new_capacity);
        memset(new_entries, 0, sizeof(DictEntry) * new_capacity);
        
        /* Rehash */
        for (int i = 0; i < dict->capacity; i++) {
            if (dict->entries[i].key == NULL) continue;
            int index = find_entry(new_entries, new_capacity, 
                                   dict->entries[i].key);
            new_entries[index] = dict->entries[i];
        }
        
        free(dict->entries);
        dict->entries = new_entries;
        dict->capacity = new_capacity;
    }
    
    int index = find_entry(dict->entries, dict->capacity, key);
    bool is_new = dict->entries[index].key == NULL;
    
    dict->entries[index].key = key;
    dict->entries[index].value = val;
    
    if (is_new) dict->count++;
}

Value dict_get(ObjDict* dict, ObjString* key) {
    if (dict->count == 0) return value_nil();
    
    int index = find_entry(dict->entries, dict->capacity, key);
    if (dict->entries[index].key == NULL) {
        return value_nil();
    }
    
    return dict->entries[index].value;
}

bool dict_has(ObjDict* dict, ObjString* key) {
    if (dict->count == 0) return false;
    
    int index = find_entry(dict->entries, dict->capacity, key);
    return dict->entries[index].key != NULL;
}

/* ==================== Native Functions ==================== */

Value native_print(int argc, Value* argv) {
    for (int i = 0; i < argc; i++) {
        printf("%s", value_to_string(argv[i]));
    }
    printf("\n");
    return value_nil();
}

Value native_len(int argc, Value* argv) {
    if (argc != 1) return value_nil();
    
    Value v = argv[0];
    switch (v.type) {
        case VAL_STRING: {
            ObjString* s = (ObjString*)v.as.ptr;
            return value_int(s->length);
        }
        case VAL_ARRAY: {
            ObjArray* a = (ObjArray*)v.as.ptr;
            return value_int(a->count);
        }
        case VAL_DICT: {
            ObjDict* d = (ObjDict*)v.as.ptr;
            return value_int(d->count);
        }
        default:
            return value_nil();
    }
}

Value native_type(int argc, Value* argv) {
    if (argc != 1) return value_nil();
    
    static ObjString* types[] = {
        NULL,  /* Will be initialized */
    };
    
    const char* type_names[] = {
        "nil", "bool", "int", "float", "string", 
        "array", "dict", "function", "native_fn", "object", "class", "error"
    };
    
    Value v = argv[0];
    return value_obj((Object*)string_new(type_names[v.type], 
                                        strlen(type_names[v.type])));
}
