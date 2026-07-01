#include "parallel.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* ==================== Parallel Execution Implementation ==================== */

/* Thread pool state */
typedef struct {
    pthread_t thread_id;
    bool active;
    int completed_tasks;
} WorkerThread;

typedef struct TaskNode {
    TaskFn function;
    void* context;
    int task_id;
    struct TaskNode* next;
} TaskNode;

typedef struct {
    TaskNode* head;
    TaskNode* tail;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} TaskQueue;

struct ThreadPool {
    WorkerThread* workers;
    int num_threads;
    TaskQueue queue;
    bool shutdown;
    int total_tasks_submitted;
    int total_tasks_completed;
    pthread_mutex_t stats_lock;
};

static void* worker_thread_func(void* arg);

/* Thread pool creation and destruction */
ThreadPool* parallel_create_thread_pool(int num_threads) {
    if (num_threads <= 0) {
        error_report_formatted(ERR_RUNTIME, 0, "Invalid thread count: %d", num_threads);
        return NULL;
    }
    
    ThreadPool* pool = malloc(sizeof(ThreadPool));
    if (!pool) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for thread pool");
        return NULL;
    }
    
    pool->workers = malloc(num_threads * sizeof(WorkerThread));
    if (!pool->workers) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for worker threads");
        free(pool);
        return NULL;
    }
    
    /* Initialize queue */
    pthread_mutex_init(&pool->queue.lock, NULL);
    pthread_cond_init(&pool->queue.not_empty, NULL);
    pool->queue.head = NULL;
    pool->queue.tail = NULL;
    pool->queue.size = 0;
    
    /* Initialize stats */
    pthread_mutex_init(&pool->stats_lock, NULL);
    pool->total_tasks_submitted = 0;
    pool->total_tasks_completed = 0;
    
    pool->num_threads = num_threads;
    pool->shutdown = false;
    
    /* Create worker threads */
    for (int i = 0; i < num_threads; i++) {
        pool->workers[i].active = false;
        pool->workers[i].completed_tasks = 0;
        int result = pthread_create(&pool->workers[i].thread_id, NULL, 
                                   worker_thread_func, pool);
        if (result != 0) {
            error_report_formatted(ERR_RUNTIME, 0, "Failed to create thread: %s", 
                                 strerror(result));
            parallel_destroy_thread_pool(pool);
            return NULL;
        }
        pool->workers[i].active = true;
    }
    
    return pool;
}

void parallel_destroy_thread_pool(ThreadPool* pool) {
    if (!pool) return;
    
    /* Signal shutdown */
    pthread_mutex_lock(&pool->queue.lock);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->queue.not_empty);
    pthread_mutex_unlock(&pool->queue.lock);
    
    /* Wait for all threads */
    for (int i = 0; i < pool->num_threads; i++) {
        if (pool->workers[i].active) {
            pthread_join(pool->workers[i].thread_id, NULL);
        }
    }
    
    /* Clean up queue */
    TaskNode* current = pool->queue.head;
    while (current) {
        TaskNode* next = current->next;
        free(current);
        current = next;
    }
    
    pthread_mutex_destroy(&pool->queue.lock);
    pthread_cond_destroy(&pool->queue.not_empty);
    pthread_mutex_destroy(&pool->stats_lock);
    
    free(pool->workers);
    free(pool);
}

int parallel_get_num_threads(ThreadPool* pool) {
    if (!pool) return 0;
    return pool->num_threads;
}

/* Worker thread function */
static void* worker_thread_func(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    
    while (true) {
        pthread_mutex_lock(&pool->queue.lock);
        
        /* Wait for task or shutdown */
        while (pool->queue.size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue.not_empty, &pool->queue.lock);
        }
        
        if (pool->shutdown && pool->queue.size == 0) {
            pthread_mutex_unlock(&pool->queue.lock);
            break;
        }
        
        /* Get task from queue */
        if (pool->queue.size > 0) {
            TaskNode* task = pool->queue.head;
            pool->queue.head = task->next;
            if (pool->queue.head == NULL) {
                pool->queue.tail = NULL;
            }
            pool->queue.size--;
            
            pthread_mutex_unlock(&pool->queue.lock);
            
            /* Execute task */
            if (task->function) {
                task->function(task->context, task->task_id);
            }
            free(task);
            
            /* Update stats */
            pthread_mutex_lock(&pool->stats_lock);
            pool->total_tasks_completed++;
            pthread_mutex_unlock(&pool->stats_lock);
        } else {
            pthread_mutex_unlock(&pool->queue.lock);
        }
    }
    
    return NULL;
}

/* Task scheduling */
void parallel_submit_task(ThreadPool* pool, TaskFn fn, void* context) {
    if (!pool || !fn) {
        error_report(ERR_RUNTIME, 0, "Invalid thread pool or function");
        return;
    }
    
    TaskNode* task = malloc(sizeof(TaskNode));
    if (!task) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for task");
        return;
    }
    
    task->function = fn;
    task->context = context;
    task->task_id = 0;
    task->next = NULL;
    
    pthread_mutex_lock(&pool->queue.lock);
    
    if (pool->queue.tail) {
        pool->queue.tail->next = task;
    } else {
        pool->queue.head = task;
    }
    pool->queue.tail = task;
    pool->queue.size++;
    pool->total_tasks_submitted++;
    
    pthread_cond_signal(&pool->queue.not_empty);
    pthread_mutex_unlock(&pool->queue.lock);
}

void parallel_submit_tasks(ThreadPool* pool, ParallelTaskConfig config) {
    if (!pool || !config.function || config.total_tasks <= 0) {
        error_report(ERR_RUNTIME, 0, "Invalid thread pool or task configuration");
        return;
    }
    
    for (int i = 0; i < config.total_tasks; i++) {
        TaskNode* task = malloc(sizeof(TaskNode));
        if (!task) {
            error_report(ERR_RUNTIME, 0, "Memory allocation failed for task");
            return;
        }
        
        task->function = config.function;
        task->context = config.context;
        task->task_id = i;
        task->next = NULL;
        
        pthread_mutex_lock(&pool->queue.lock);
        
        if (pool->queue.tail) {
            pool->queue.tail->next = task;
        } else {
            pool->queue.head = task;
        }
        pool->queue.tail = task;
        pool->queue.size++;
        pool->total_tasks_submitted++;
        
        pthread_cond_signal(&pool->queue.not_empty);
        pthread_mutex_unlock(&pool->queue.lock);
    }
}

void parallel_wait_completion(ThreadPool* pool) {
    if (!pool) return;
    
    while (true) {
        pthread_mutex_lock(&pool->stats_lock);
        int submitted = pool->total_tasks_submitted;
        int completed = pool->total_tasks_completed;
        pthread_mutex_unlock(&pool->stats_lock);
        
        if (submitted == completed && submitted > 0) {
            break;
        }
        
        usleep(1000);  /* Sleep 1ms */
    }
}

/* Parallel loops */
void parallel_for(int start, int end, TaskFn body, void* context, int num_threads) {
    if (!body || num_threads <= 0) {
        error_report(ERR_RUNTIME, 0, "Invalid parallel_for parameters");
        return;
    }
    
    ThreadPool* pool = parallel_create_thread_pool(num_threads);
    if (!pool) return;
    
    /* Submit tasks for each iteration */
    for (int i = start; i < end; i++) {
        TaskNode* task = malloc(sizeof(TaskNode));
        if (!task) {
            error_report(ERR_RUNTIME, 0, "Memory allocation failed for task");
            parallel_destroy_thread_pool(pool);
            return;
        }
        
        task->function = body;
        task->context = context;
        task->task_id = i - start;
        task->next = NULL;
        
        pthread_mutex_lock(&pool->queue.lock);
        if (pool->queue.tail) {
            pool->queue.tail->next = task;
        } else {
            pool->queue.head = task;
        }
        pool->queue.tail = task;
        pool->queue.size++;
        pool->total_tasks_submitted++;
        pthread_cond_signal(&pool->queue.not_empty);
        pthread_mutex_unlock(&pool->queue.lock);
    }
    
    parallel_wait_completion(pool);
    parallel_destroy_thread_pool(pool);
}

Value parallel_map(ObjArray* arr, TaskFn fn, int num_threads) {
    if (!arr || !fn) {
        return value_nil();
    }
    
    ObjArray* result = array_new();
    if (!result) return value_nil();
    
    ThreadPool* pool = parallel_create_thread_pool(num_threads);
    if (!pool) {
        return value_obj((Object*)result);
    }
    
    parallel_wait_completion(pool);
    parallel_destroy_thread_pool(pool);
    
    return value_obj((Object*)result);
}

/* Mutex implementation */
Mutex* parallel_mutex_create(void) {
    Mutex* mutex = malloc(sizeof(Mutex));
    if (!mutex) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for mutex");
        return NULL;
    }
    
    if (pthread_mutex_init(&mutex->lock, NULL) != 0) {
        error_report(ERR_RUNTIME, 0, "Failed to initialize mutex");
        free(mutex);
        return NULL;
    }
    
    mutex->is_locked = false;
    return mutex;
}

void parallel_mutex_destroy(Mutex* mutex) {
    if (!mutex) return;
    pthread_mutex_destroy(&mutex->lock);
    free(mutex);
}

void parallel_mutex_lock(Mutex* mutex) {
    if (!mutex) return;
    pthread_mutex_lock(&mutex->lock);
    mutex->is_locked = true;
}

void parallel_mutex_unlock(Mutex* mutex) {
    if (!mutex) return;
    mutex->is_locked = false;
    pthread_mutex_unlock(&mutex->lock);
}

bool parallel_mutex_try_lock(Mutex* mutex) {
    if (!mutex) return false;
    int result = pthread_mutex_trylock(&mutex->lock);
    if (result == 0) {
        mutex->is_locked = true;
        return true;
    }
    return false;
}

/* Semaphore implementation */
Semaphore* parallel_semaphore_create(int initial_count) {
    if (initial_count < 0) {
        error_report_formatted(ERR_RUNTIME, 0, "Invalid semaphore count: %d", initial_count);
        return NULL;
    }
    
    Semaphore* sem = malloc(sizeof(Semaphore));
    if (!sem) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for semaphore");
        return NULL;
    }
    
    if (pthread_cond_init(&sem->cond, NULL) != 0) {
        error_report(ERR_RUNTIME, 0, "Failed to initialize semaphore");
        free(sem);
        return NULL;
    }
    
    sem->count = initial_count;
    sem->initial_count = initial_count;
    return sem;
}

void parallel_semaphore_destroy(Semaphore* sem) {
    if (!sem) return;
    pthread_cond_destroy(&sem->cond);
    free(sem);
}

void parallel_semaphore_wait(Semaphore* sem) {
    if (!sem) return;
    
    /* Stub - full implementation would use proper wait */
    while (sem->count <= 0) {
        usleep(100);
    }
    sem->count--;
}

void parallel_semaphore_signal(Semaphore* sem) {
    if (!sem) return;
    sem->count++;
}

/* Read-Write Lock implementation */
ReadWriteLock* parallel_rwlock_create(void) {
    ReadWriteLock* lock = malloc(sizeof(ReadWriteLock));
    if (!lock) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for RW lock");
        return NULL;
    }
    
    if (pthread_rwlock_init(&lock->lock, NULL) != 0) {
        error_report(ERR_RUNTIME, 0, "Failed to initialize RW lock");
        free(lock);
        return NULL;
    }
    
    return lock;
}

void parallel_rwlock_destroy(ReadWriteLock* lock) {
    if (!lock) return;
    pthread_rwlock_destroy(&lock->lock);
    free(lock);
}

void parallel_rwlock_read_lock(ReadWriteLock* lock) {
    if (!lock) return;
    pthread_rwlock_rdlock(&lock->lock);
}

void parallel_rwlock_read_unlock(ReadWriteLock* lock) {
    if (!lock) return;
    pthread_rwlock_unlock(&lock->lock);
}

void parallel_rwlock_write_lock(ReadWriteLock* lock) {
    if (!lock) return;
    pthread_rwlock_wrlock(&lock->lock);
}

void parallel_rwlock_write_unlock(ReadWriteLock* lock) {
    if (!lock) return;
    pthread_rwlock_unlock(&lock->lock);
}

/* Thread-safe queue */
ThreadSafeQueue* parallel_queue_create(int capacity) {
    if (capacity <= 0) {
        error_report_formatted(ERR_RUNTIME, 0, "Invalid queue capacity: %d", capacity);
        return NULL;
    }
    
    ThreadSafeQueue* queue = malloc(sizeof(ThreadSafeQueue));
    if (!queue) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for queue");
        return NULL;
    }
    
    queue->items = malloc(capacity * sizeof(Value));
    if (!queue->items) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for queue items");
        free(queue);
        return NULL;
    }
    
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    pthread_mutex_init(&queue->lock, NULL);
    
    return queue;
}

void parallel_queue_destroy(ThreadSafeQueue* queue) {
    if (!queue) return;
    pthread_mutex_destroy(&queue->lock);
    free(queue->items);
    free(queue);
}

bool parallel_queue_enqueue(ThreadSafeQueue* queue, Value item) {
    if (!queue) return false;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->size >= queue->capacity) {
        pthread_mutex_unlock(&queue->lock);
        return false;
    }
    
    queue->items[queue->tail] = item;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    pthread_mutex_unlock(&queue->lock);
    return true;
}

bool parallel_queue_dequeue(ThreadSafeQueue* queue, Value* item) {
    if (!queue || !item) return false;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->lock);
        return false;
    }
    
    *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    
    pthread_mutex_unlock(&queue->lock);
    return true;
}

int parallel_queue_size(ThreadSafeQueue* queue) {
    if (!queue) return 0;
    
    pthread_mutex_lock(&queue->lock);
    int size = queue->size;
    pthread_mutex_unlock(&queue->lock);
    
    return size;
}

/* Work stealing and load balancing */
void parallel_work_steal_enable(ThreadPool* pool) {
    if (!pool) return;
    /* Stub - work stealing implementation */
}

void parallel_load_balance(ThreadPool* pool) {
    if (!pool) return;
    /* Stub - load balancing implementation */
}

/* Utility functions */
int parallel_get_cpu_count(void) {
    int count = (int)sysconf(_SC_NPROCESSORS_ONLN);
    return count > 0 ? count : 1;
}

void parallel_set_affinity(int thread_id, int cpu_id) {
    /* Stub - CPU affinity setting */
}
