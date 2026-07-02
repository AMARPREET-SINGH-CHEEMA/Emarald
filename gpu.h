#ifndef EMARALD_GPU_H
#define EMARALD_GPU_H

#include "interpreter.h"
#include <stdbool.h>

/* Minimal GPU abstraction for Emarald
 * - Runtime enables GPU via EMERALD_GPU=1 environment variable
 * - If GPU not enabled/available, CPU fallback is used
 * - Provides basic vector operations (add) and memory transfer stubs
 */

/* Initialize GPU subsystem (returns true if GPU backend active) */
bool gpu_init(void);
void gpu_shutdown(void);

/* Query availability */
bool gpu_is_enabled(void);

/* Transfer array to device (stub) - returns opaque handle pointer */
void* gpu_alloc_from_array(ObjArray* arr);
void gpu_free_device_ptr(void* dev_ptr);

/* Vector addition: returns new ObjArray with results (device or CPU) */
Value gpu_vector_add(Value a, Value b);

#endif /* EMARALD_GPU_H */
