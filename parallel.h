#ifndef EMARALD_PARALLEL_H
#define EMARALD_PARALLEL_H

#include "interpreter.h"
#include <stdbool.h>
#include <pthread.h>

/* ==================== Parallel Execution ==================== */

/* Thread pool for task scheduling */
typedef struct ThreadPool ThreadPool;

/* Task function signature */
typedef Value (*TaskFn)(void* context, int task_id);

/* Synchronization primitives */
typedef struct {
    pthread_mutex_t lock;
    bool is_locked;
} Mutex;

typedef struct {
    pthread_cond_t cond;
    int count;
    int initial_count;
} Semaphore;

typedef struct {
    pthread_rwlock_t lock;
} ReadWriteLock;

/* Parallel task configuration */
typedef struct {
    TaskFn function;
    void* context;
    int total_tasks;
    int num_threads;
} ParallelTaskConfig;

/* Parallel for loop configuration */
typedef struct {
    int start;
    int end;
    int step;
    TaskFn body;
    void* context;
    int num_threads;
} ParallelForConfig;

/* ==================== Public API ==================== */

/* Thread pool management */
ThreadPool* parallel_create_thread_pool(int num_threads);
void parallel_destroy_thread_pool(ThreadPool* pool);
int parallel_get_num_threads(ThreadPool* pool);

/* Task scheduling */
void parallel_submit_task(ThreadPool* pool, TaskFn fn, void* context);
void parallel_submit_tasks(ThreadPool* pool, ParallelTaskConfig config);
void parallel_wait_completion(ThreadPool* pool);

/* Parallel loops */
void parallel_for(int start, int end, TaskFn body, void* context, int num_threads);
Value parallel_map(ObjArray* arr, TaskFn fn, int num_threads);

/* Synchronization primitives */
Mutex* parallel_mutex_create(void);
void parallel_mutex_destroy(Mutex* mutex);
void parallel_mutex_lock(Mutex* mutex);
void parallel_mutex_unlock(Mutex* mutex);
bool parallel_mutex_try_lock(Mutex* mutex);

Semaphore* parallel_semaphore_create(int initial_count);
void parallel_semaphore_destroy(Semaphore* sem);
void parallel_semaphore_wait(Semaphore* sem);
void parallel_semaphore_signal(Semaphore* sem);

ReadWriteLock* parallel_rwlock_create(void);
void parallel_rwlock_destroy(ReadWriteLock* lock);
void parallel_rwlock_read_lock(ReadWriteLock* lock);
void parallel_rwlock_read_unlock(ReadWriteLock* lock);
void parallel_rwlock_write_lock(ReadWriteLock* lock);
void parallel_rwlock_write_unlock(ReadWriteLock* lock);

/* Thread-safe data structures */
typedef struct {
    Value* items;
    int head;
    int tail;
    int capacity;
    int size;
    pthread_mutex_t lock;
} ThreadSafeQueue;

ThreadSafeQueue* parallel_queue_create(int capacity);
void parallel_queue_destroy(ThreadSafeQueue* queue);
bool parallel_queue_enqueue(ThreadSafeQueue* queue, Value item);
bool parallel_queue_dequeue(ThreadSafeQueue* queue, Value* item);
int parallel_queue_size(ThreadSafeQueue* queue);

/* Work stealing and load balancing */
void parallel_work_steal_enable(ThreadPool* pool);
void parallel_load_balance(ThreadPool* pool);

/* Utilities */
int parallel_get_cpu_count(void);
void parallel_set_affinity(int thread_id, int cpu_id);

#endif /* EMARALD_PARALLEL_H */
