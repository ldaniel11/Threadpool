#include "threadpool.h"
#include "list.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

enum working_status
{
    STARTED,
    IN_PROGRESS,
    COMPLETED
};

typedef struct future
{
    fork_join_task_t task;           // a function pointer type
    struct list_elem elem;           // the future is inserted into the list
    void *data;                      // the argument for the method thread_pool_submit
    void *result;                    // store the task result once it completes the execution
    sem_t task_done;                 // the semaphore used for checking the condition
    enum working_status task_status; // 3 statuses are depicted: started, in progress, and completed
    struct thread_pool *pool;        // The future is an instance of the task(external access)
    struct worker *worker;           // the worker is an instance of the task(internal access)
} future;

typedef struct worker
{
    struct list local;    // A worker thread consists of a queue containing multiple elements
    struct list_elem obj; // The element in the queue
    pthread_t thread;
    pthread_mutex_t local_lock; // We would like to access the lock
} worker;

typedef struct thread_pool
{
    struct list global;     // a global queue
    struct list workers;    // the workers list we should concentrate on
    int threads;            // the total number of threads in the thread pool
    pthread_mutex_t p_lock; // the mutexes lock for the pool list
    pthread_cond_t cond;    // the condition variable to be utilized
    int exit_flag;          // a flag used to indicate when the pool is shutting down
    pthread_t *thread_arr;  // the thread array in the thread pool
} thread_pool;

static void *worker_thread(void *p)
{

    thread_pool *swimming_pool = (thread_pool *)p;

    pthread_mutex_lock(&swimming_pool->p_lock);

    struct worker *w = NULL;

    struct list_elem *elem;

    for (elem = list_begin(&swimming_pool->workers); elem != list_end(&swimming_pool->workers);
         elem = list_next(elem))
    {
        struct worker *t = list_entry(elem, struct worker, obj);
        if (t->thread == pthread_self())
        {
            w = t;
            break;
        }
    }
    pthread_mutex_unlock(&swimming_pool->p_lock);

    struct list_elem *e;

    for (e = list_begin(&w->local); e != list_end(&w->local);
         e = list_next(e))
    {
        struct future *f = list_entry(e, struct future, elem);
        f->task_status = STARTED;
        f->task(swimming_pool, f->data);
    }

    
}

// create a thread pool
struct thread_pool *thread_pool_new(int nthreads)
{
    // create a thread pool
    thread_pool *swimming_pool = (thread_pool *)calloc(1, sizeof(thread_pool));

    swimming_pool->threads = nthreads;
    list_init(&swimming_pool->global);
    list_init(&swimming_pool->workers);

    swimming_pool->thread_arr = (pthread_t *)calloc(nthreads, sizeof(pthread_t));

    pthread_mutex_init(&swimming_pool->p_lock, NULL);
    pthread_mutex_init(&swimming_pool->cond, NULL);

    // initialize worker threads
    pthread_mutex_lock(&swimming_pool->p_lock);
    int a = 0; // a local variable used to represent the number of threads temporarily

    while (a < swimming_pool->threads)
    {
        struct worker *mcquain = (worker *)calloc(1, sizeof(worker));
        mcquain->thread = swimming_pool->thread_arr[a];
        list_init(&mcquain->local);
        pthread_mutex_init(&mcquain->local_lock, NULL);

        list_push_back(&swimming_pool->workers, &mcquain->obj);

        // I need to create a thread
        pthread_create(&swimming_pool->thread_arr[a], NULL, worker_thread, swimming_pool);

        a++;
    }
    pthread_mutex_unlock(&swimming_pool->p_lock);

    return swimming_pool;
}

void thread_pool_shutdown_and_destroy(struct thread_pool *pool)
{
}

struct future *thread_pool_submit(struct thread_pool *pool, fork_join_task_t task, void *data)
{

}

void *future_get(struct future *future)
{
}

void future_free(struct future *future)
{
}
