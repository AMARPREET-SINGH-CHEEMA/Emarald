#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/* ==================== VM Implementation ==================== */

void vm_init(VM* vm) {
    vm->stack_top = 0;
    vm->frame_count = 0;
    vm->bytes_allocated = 0;
    vm->gc_threshold = 1024 * 1024;  /* 1MB */
    vm->objects = NULL;
    
    /* Initialize global and string storage */
    vm->globals = dict_new();
    vm->strings = dict_new();
    
    /* Register native functions */
    ObjNativeFn* print_fn = malloc(sizeof(ObjNativeFn));
    print_fn->obj.type = VAL_NATIVE_FN;
    print_fn->obj.is_marked = false;
    print_fn->obj.next = vm->objects;
    vm->objects = (Object*)print_fn;
    print_fn->name = string_copy("print", 5);
    print_fn->function = native_print;
    
    dict_set(vm->globals, print_fn->name, value_obj((Object*)print_fn));
    
    ObjNativeFn* len_fn = malloc(sizeof(ObjNativeFn));
    len_fn->obj.type = VAL_NATIVE_FN;
    len_fn->obj.is_marked = false;
    len_fn->obj.next = vm->objects;
    vm->objects = (Object*)len_fn;
    len_fn->name = string_copy("len", 3);
    len_fn->function = native_len;
    
    dict_set(vm->globals, len_fn->name, value_obj((Object*)len_fn));
    
    ObjNativeFn* type_fn = malloc(sizeof(ObjNativeFn));
    type_fn->obj.type = VAL_NATIVE_FN;
    type_fn->obj.is_marked = false;
    type_fn->obj.next = vm->objects;
    vm->objects = (Object*)type_fn;
    type_fn->name = string_copy("type", 4);
    type_fn->function = native_type;
    
    dict_set(vm->globals, type_fn->name, value_obj((Object*)type_fn));
}

void vm_free(VM* vm) {
    /* Free all objects */
    Object* obj = vm->objects;
    while (obj != NULL) {
        Object* next = obj->next;
        free(obj);
        obj = next;
    }
    
    free(vm->globals);
    free(vm->strings);
}

void vm_push(VM* vm, Value val) {
    if (vm->stack_top < STACK_SIZE) {
        vm->stack[vm->stack_top++] = val;
    }
}

Value vm_pop(VM* vm) {
    if (vm->stack_top > 0) {
        return vm->stack[--vm->stack_top];
    }
    return value_nil();
}

static Value vm_peek(VM* vm, int distance) {
    if (vm->stack_top - distance - 1 >= 0) {
        return vm->stack[vm->stack_top - distance - 1];
    }
    return value_nil();
}

static int read_operand(CallFrame* frame) {
    int high = *frame->pc++;
    int low = *frame->pc++;
    return (high << 8) | low;
}

static void vm_runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Runtime Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#define BINARY_OP(op) \
    do { \
        Value b = vm_pop(vm); \
        Value a = vm_pop(vm); \
        if (a.type == VAL_INT && b.type == VAL_INT) { \
            vm_push(vm, value_int(a.as.integer op b.as.integer)); \
        } else if ((a.type == VAL_INT || a.type == VAL_FLOAT) && \
                   (b.type == VAL_INT || b.type == VAL_FLOAT)) { \
            double da = a.type == VAL_INT ? a.as.integer : a.as.floating; \
            double db = b.type == VAL_INT ? b.as.integer : b.as.floating; \
            vm_push(vm, value_float(da op db)); \
        } \
    } while (false)

#define COMPARISON_OP(op) \
    do { \
        Value b = vm_pop(vm); \
        Value a = vm_pop(vm); \
        if (a.type == VAL_INT && b.type == VAL_INT) { \
            vm_push(vm, value_bool(a.as.integer op b.as.integer)); \
        } else if ((a.type == VAL_INT || a.type == VAL_FLOAT) && \
                   (b.type == VAL_INT || b.type == VAL_FLOAT)) { \
            double da = a.type == VAL_INT ? a.as.integer : a.as.floating; \
            double db = b.type == VAL_INT ? b.as.integer : b.as.floating; \
            vm_push(vm, value_bool(da op db)); \
        } \
    } while (false)

void vm_execute(VM* vm, ObjFunction* function) {
    /* Push call frame */
    if (vm->frame_count >= FRAMES_MAX) {
        vm_runtime_error("Stack overflow");
        return;
    }
    
    CallFrame* frame = &vm->frames[vm->frame_count++];
    frame->function = function;
    frame->pc = function->bytecode;
    frame->stack_base = vm->stack + vm->stack_top;
    frame->local_count = 0;
    
    /* Execute bytecode */
    while (frame->pc < function->bytecode + function->bytecode_len) {
        OpCode op = (OpCode)(*frame->pc);
        frame->pc++;
        
        switch (op) {
            case OP_LOAD_CONST: {
                int const_idx = read_operand(frame);
                vm_push(vm, function->constants[const_idx]);
                break;
            }
            
            case OP_LOAD_LOCAL: {
                int local_idx = read_operand(frame);
                vm_push(vm, frame->stack_base[local_idx]);
                break;
            }
            
            case OP_STORE_LOCAL: {
                int local_idx = read_operand(frame);
                Value val = vm_pop(vm);
                frame->stack_base[local_idx] = val;
                break;
            }
            
            case OP_ADD: {
                BINARY_OP(+);
                break;
            }
            
            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            
            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            
            case OP_DIVIDE: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                if (b.type == VAL_INT && b.as.integer == 0) {
                    vm_runtime_error("Division by zero");
                    return;
                }
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    vm_push(vm, value_int(a.as.integer / b.as.integer));
                } else if ((a.type == VAL_INT || a.type == VAL_FLOAT) &&
                           (b.type == VAL_INT || b.type == VAL_FLOAT)) {
                    double da = a.type == VAL_INT ? a.as.integer : a.as.floating;
                    double db = b.type == VAL_INT ? b.as.integer : b.as.floating;
                    vm_push(vm, value_float(da / db));
                }
                break;
            }
            
            case OP_MODULO: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    if (b.as.integer == 0) {
                        vm_runtime_error("Modulo by zero");
                        return;
                    }
                    vm_push(vm, value_int(a.as.integer % b.as.integer));
                } else if ((a.type == VAL_INT || a.type == VAL_FLOAT) &&
                           (b.type == VAL_INT || b.type == VAL_FLOAT)) {
                    double da = a.type == VAL_INT ? a.as.integer : a.as.floating;
                    double db = b.type == VAL_INT ? b.as.integer : b.as.floating;
                    if (db == 0.0) {
                        vm_runtime_error("Modulo by zero");
                        return;
                    }
                    vm_push(vm, value_float(fmod(da, db)));
                }
                break;
            }
            
            case OP_POWER: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                double da = a.type == VAL_INT ? a.as.integer : a.as.floating;
                double db = b.type == VAL_INT ? b.as.integer : b.as.floating;
                vm_push(vm, value_float(pow(da, db)));
                break;
            }
            
            case OP_NEGATE: {
                Value val = vm_pop(vm);
                if (val.type == VAL_INT) {
                    vm_push(vm, value_int(-val.as.integer));
                } else if (val.type == VAL_FLOAT) {
                    vm_push(vm, value_float(-val.as.floating));
                }
                break;
            }
            
            case OP_NOT: {
                Value val = vm_pop(vm);
                vm_push(vm, value_bool(value_is_falsy(val)));
                break;
            }
            
            case OP_EQUAL: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                vm_push(vm, value_bool(value_equals(a, b)));
                break;
            }
            
            case OP_NOT_EQUAL: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                vm_push(vm, value_bool(!value_equals(a, b)));
                break;
            }
            
            case OP_LESS: {
                COMPARISON_OP(<);
                break;
            }
            
            case OP_LESS_EQUAL: {
                COMPARISON_OP(<=);
                break;
            }
            
            case OP_GREATER: {
                COMPARISON_OP(>);
                break;
            }
            
            case OP_GREATER_EQUAL: {
                COMPARISON_OP(>=);
                break;
            }
            
            case OP_AND: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                vm_push(vm, value_bool(!value_is_falsy(a) && 
                                      !value_is_falsy(b)));
                break;
            }
            
            case OP_OR: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                vm_push(vm, value_bool(!value_is_falsy(a) || 
                                      !value_is_falsy(b)));
                break;
            }
            
            case OP_BUILD_ARRAY: {
                int count = read_operand(frame);
                ObjArray* arr = array_new();
                for (int i = 0; i < count; i++) {
                    array_push(arr, vm_pop(vm));
                }
                vm_push(vm, value_obj((Object*)arr));
                break;
            }
            
            case OP_BUILD_DICT: {
                int count = read_operand(frame);
                ObjDict* dict = dict_new();
                for (int i = 0; i < count; i++) {
                    Value val = vm_pop(vm);
                    Value key = vm_pop(vm);
                    if (key.type == VAL_STRING) {
                        dict_set(dict, (ObjString*)key.as.ptr, val);
                    }
                }
                vm_push(vm, value_obj((Object*)dict));
                break;
            }
            
            case OP_INDEX_GET: {
                Value idx = vm_pop(vm);
                Value obj = vm_pop(vm);
                
                if (obj.type == VAL_ARRAY && idx.type == VAL_INT) {
                    ObjArray* arr = (ObjArray*)obj.as.ptr;
                    vm_push(vm, array_get(arr, (int)idx.as.integer));
                } else if (obj.type == VAL_DICT && idx.type == VAL_STRING) {
                    ObjDict* dict = (ObjDict*)obj.as.ptr;
                    vm_push(vm, dict_get(dict, (ObjString*)idx.as.ptr));
                } else {
                    vm_push(vm, value_nil());
                }
                break;
            }
            
            case OP_INDEX_SET: {
                Value val = vm_pop(vm);
                Value idx = vm_pop(vm);
                Value obj = vm_pop(vm);
                
                if (obj.type == VAL_ARRAY && idx.type == VAL_INT) {
                    ObjArray* arr = (ObjArray*)obj.as.ptr;
                    array_set(arr, (int)idx.as.integer, val);
                } else if (obj.type == VAL_DICT && idx.type == VAL_STRING) {
                    ObjDict* dict = (ObjDict*)obj.as.ptr;
                    dict_set(dict, (ObjString*)idx.as.ptr, val);
                }
                break;
            }
            
            case OP_PRINT: {
                Value val = vm_pop(vm);
                printf("%s\n", value_to_string(val));
                break;
            }
            
            case OP_POP: {
                vm_pop(vm);
                break;
            }
            
            case OP_NIL: {
                vm_push(vm, value_nil());
                break;
            }
            
            case OP_TRUE: {
                vm_push(vm, value_bool(true));
                break;
            }
            
            case OP_FALSE: {
                vm_push(vm, value_bool(false));
                break;
            }
            
            case OP_RETURN: {
                vm->frame_count--;
                if (vm->frame_count == 0) {
                    return;
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
            
            default:
                vm_runtime_error("Unknown opcode: %d", op);
                return;
        }
    }
    
    vm->frame_count--;
}

/* ==================== Example Program Execution ==================== */

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* source = malloc(file_size + 1);
    if (!source) {
        fclose(file);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    Stmt* program = parse(source);
    if (!program) {
        free(source);
        return 1;
    }

    ObjFunction* function = compile(program);
    if (!function) {
        free(source);
        return 1;
    }

    VM vm;
    vm_init(&vm);
    vm_execute(&vm, function);
    vm_free(&vm);

    free(source);
    free(function->name);
    free(function);
    return 0;
}
