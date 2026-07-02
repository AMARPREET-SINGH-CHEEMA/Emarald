#include "gpu.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bool g_gpu_enabled = false;

bool gpu_init(void) {
    const char* e = getenv("EMERALD_GPU");
    if (e && strcmp(e, "1") == 0) {
        /* In a full implementation we'd probe OpenCL/CUDA here. */
        g_gpu_enabled = true;
    } else {
        g_gpu_enabled = false;
    }
    return g_gpu_enabled;
}

void gpu_shutdown(void) {
    /* Release GPU resources if any (stub) */
    g_gpu_enabled = false;
}

bool gpu_is_enabled(void) {
    return g_gpu_enabled;
}

void* gpu_alloc_from_array(ObjArray* arr) {
    if (!arr) return NULL;
    /* Stub: return a shallow copy pointer (not real device memory) */
    double* buf = malloc(sizeof(double) * arr->count);
    if (!buf) return NULL;
    for (size_t i = 0; i < arr->count; i++) {
        Value v = array_get(arr, (int)i);
        if (v.type == VAL_INT) buf[i] = (double)v.as.integer;
        else if (v.type == VAL_FLOAT) buf[i] = v.as.floating;
        else buf[i] = 0.0;
    }
    return (void*)buf;
}

void gpu_free_device_ptr(void* dev_ptr) {
    if (dev_ptr) free(dev_ptr);
}

/* Helper to convert Numbers to Value */
static Value make_number_from_double(double d) {
    /* Prefer float if fractional, otherwise int */
    if (d == (int64_t)d) return value_int((int64_t)d);
    return value_float(d);
}

/* Vector add implementation with CPU fallback */
Value gpu_vector_add(Value a, Value b) {
    if (a.type != VAL_ARRAY || b.type != VAL_ARRAY) {
        error_report(ERR_TYPE, 0, "gpu_vector_add expects two arrays");
        return value_nil();
    }
    ObjArray* A = (ObjArray*)a.as.ptr;
    ObjArray* B = (ObjArray*)b.as.ptr;
    if (!A || !B) return value_nil();
    if (A->count != B->count) {
        error_report(ERR_RUNTIME, 0, "Array sizes must match for vector add");
        return value_nil();
    }

    ObjArray* out = array_new();
    if (!out) return value_nil();

    if (g_gpu_enabled) {
        /* Stubbed GPU path: allocate device buffers and perform "GPU" add
         * For now we simulate by copying to temporary buffers and doing CPU math
         */
        void* dev_a = gpu_alloc_from_array(A);
        void* dev_b = gpu_alloc_from_array(B);
        if (!dev_a || !dev_b) {
            gpu_free_device_ptr(dev_a);
            gpu_free_device_ptr(dev_b);
            error_report(ERR_RUNTIME, 0, "Failed to allocate device buffers; falling back to CPU");
            /* Fall through to CPU path below */
        } else {
            double* da = (double*)dev_a;
            double* db = (double*)dev_b;
            for (size_t i = 0; i < A->count; i++) {
                double r = da[i] + db[i];
                array_push(out, make_number_from_double(r));
            }
            gpu_free_device_ptr(dev_a);
            gpu_free_device_ptr(dev_b);
            return value_obj((Object*)out);
        }
    }

    /* CPU fallback */
    for (int i = 0; i < (int)A->count; i++) {
        Value va = array_get(A, i);
        Value vb = array_get(B, i);
        if ((va.type != VAL_INT && va.type != VAL_FLOAT) ||
            (vb.type != VAL_INT && vb.type != VAL_FLOAT)) {
            array_push(out, value_nil());
            continue;
        }
        double da = (va.type == VAL_INT) ? (double)va.as.integer : va.as.floating;
        double db = (vb.type == VAL_INT) ? (double)vb.as.integer : vb.as.floating;
        double r = da + db;
        array_push(out, make_number_from_double(r));
    }
    return value_obj((Object*)out);
}
