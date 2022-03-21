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
    struct thread_pool *pool;
} worker;

typedef struct thread_pool
{
    struct list global;     // a global queue
    struct list workers;    // the workers list we should concentrate on
    int threads;            // the total number of threads in the thread pool
    pthread_mutex_t p_lock; // the mutexes lock for the pool list
    pthread_cond_t cond;    // the condition variable to be utilized
    bool exit;              // a flag used to indicate when the pool is shutting down
    pthread_t *thread_arr;  // the thread array in the thread pool
} thread_pool;

//static _Thread_local struct worker *internal = NULL;

static void *worker_thread(void *p)
{
    struct worker *internal = (struct worker *)p;
    thread_pool *swimming_pool = internal->pool;

    // lock pool
    pthread_mutex_lock(&swimming_pool->p_lock);
    for (;;)
    {
        // The first possibility is to refer to the global queue
        if (!list_empty(&swimming_pool->global))
        {
            // If the queue is non-empty, we will extract the future and determine each corresponding status.
            struct future *f = list_entry(list_pop_front(&swimming_pool->global), struct future, elem);
            f->task_status = IN_PROGRESS;
            pthread_mutex_unlock(&swimming_pool->p_lock);
            f->result = f->task(swimming_pool, f->data); 

            pthread_mutex_lock(&swimming_pool->p_lock);
            f->task_status = COMPLETED;
            sem_post(&f->task_done);
            pthread_mutex_unlock(&swimming_pool->p_lock);
            continue;
        }
        pthread_mutex_unlock(&swimming_pool->p_lock);

        // The second possibility is to refer to the worker thread.
        struct list_elem *e;

        for (e = list_begin(&swimming_pool->workers); e != list_end(&swimming_pool->workers);
             e = list_next(e))
        {
            struct worker *w = list_entry(e, struct worker, obj);
            pthread_mutex_lock(&w->local_lock);
            // the work stealing approach
            if (!list_empty(&w->local))
            {
                struct future *f = list_entry(list_pop_back(&w->local), struct future, elem);
                f->task_status = IN_PROGRESS;
                pthread_mutex_unlock(&w->local_lock);
                f->result =f->task(swimming_pool, f->data);
                
                pthread_mutex_lock(&w->local_lock);
                f->task_status = COMPLETED;
                sem_post(&f->task_done);
                pthread_mutex_unlock(&w->local_lock);
                continue;
            }
            pthread_mutex_unlock(&w->local_lock);
        }

        // The third scenario is to determine whether the pool is ready to shutdown.
        pthread_mutex_lock(&swimming_pool->p_lock);
        if (swimming_pool->exit)
        {
            pthread_mutex_unlock(&swimming_pool->p_lock);
            break;
        }
        pthread_mutex_unlock(&swimming_pool->p_lock);
        pthread_mutex_lock(&internal->local_lock);
        pthread_cond_wait(&swimming_pool->cond, &internal->local_lock);
        pthread_mutex_unlock(&internal->local_lock);
    // check if global pool is not empty
        // set future status
        // remove future from list
        // unlock pool
        // do task
        // lock pool
        // change future status again
        // sem post
        // unlock pool
        // continue
    // check for work stealing
        // logic is similar as above

    // check shutdown condition is false
        // wait

        // pthread_mutex_lock(&w->local_lock);
        // if (!list_empty(&w->local))
        //  {
        //      e = list_pop_front(&w->local);

        //     struct future *f = list_entry(e, struct future, elem);
        //     if (f->task_status == STARTED)
        //     {
        //         f->task_status = IN_PROGRESS;
        //         f->task(swimming_pool, f->data);
        //     }
        // }
        // else if ()

        // pthread_mutex_unlock(&w->local_lock);
    }
    return NULL;
    // unlock pool
}

// create a thread pool
struct thread_pool *thread_pool_new(int nthreads)
{
    // create a thread pool
    thread_pool *swimming_pool = (thread_pool *)calloc(1, sizeof(thread_pool));

    swimming_pool->threads = nthreads;
    list_init(&swimming_pool->global);
    list_init(&swimming_pool->workers);

    swimming_pool->thread_arr = (pthread_t *)calloc(nthreads, sizeof(pthread_t)); // The memory is accessed

    pthread_mutex_init(&swimming_pool->p_lock, NULL);
    pthread_cond_init(&swimming_pool->cond, NULL);

    // initialize worker threads
    pthread_mutex_lock(&swimming_pool->p_lock);
    int a = 0; // a local variable used to represent the number of threads temporarily

    while (a < swimming_pool->threads)
    {
        struct worker *mcquain = (worker *)calloc(1, sizeof(worker));
        mcquain->thread = swimming_pool->thread_arr[a];
        mcquain->pool = swimming_pool;
        list_init(&mcquain->local);
        pthread_mutex_init(&mcquain->local_lock, NULL);

        list_push_back(&swimming_pool->workers, &mcquain->obj);

        // I need to create a thread
        pthread_create(&swimming_pool->thread_arr[a], NULL, worker_thread, mcquain);

        a++;
    }
    pthread_mutex_unlock(&swimming_pool->p_lock);

    return swimming_pool;
}

void thread_pool_shutdown_and_destroy(struct thread_pool *pool)
{
    pthread_mutex_lock(&pool->p_lock);

    pool->exit = true; // Set the flag to shut down the pool

    pthread_cond_broadcast(&pool->cond); // Other threads might be waiting so that all threads
                                         // can destroy altogether

    pthread_mutex_unlock(&pool->p_lock);

    int a = 0;
    while (a < pool->threads)
    {
        pthread_join(pool->thread_arr[a], NULL);
        a++;
    }

    free(pool->thread_arr);
    free(pool);
}

struct future *thread_pool_submit(struct thread_pool *pool, fork_join_task_t task, void *data)
{
    // The main purpose of this task is to submit a fork_join_task
    // to the threadpool and return its future.

    //(1) insert a mutex lock to achieve its desired implementation
    pthread_mutex_lock(&pool->p_lock);

    //(2) instantiate a target of future* and access its corresponding attributes
    future *future_target = (future *)calloc(1, sizeof(future));
    future_target->pool = pool;
    future_target->task = task;
    future_target->data = data;
    future_target->task_status = 0; // The status of the task is about to get started.

    sem_init(&future_target->task_done, 0, 0); // initialize the smarphone is required to do so.
    // similar to a boss that you finish your task and place it at the laptop

    // (3) Later, we decide to add an element to the global queue.
    list_push_front(&pool->global, &future_target->elem);

    // (4) The final step we are available to do is to wake up the threads in the thread pool
    // and unlock the threadpool.
    pthread_cond_broadcast(&pool->cond); // We need to be default to braodcast and wait for staff.
    pthread_mutex_unlock(&pool->p_lock);

    return future_target;
}


void *future_get(struct future * future) {
     // the external case.
     if (future->task_status != COMPLETED) {
         sem_wait(&future->task_done);
     }
     else {
         // the internal case

         pthread_mutex_lock(&future->pool->p_lock);

         // Later, we have to analyze the following three scenarios with respect to three status
        // in the enumerator working_status
        if (future->task_status == STARTED) {
        
            list_remove(&future->elem); // The element can be removed from the worker list.

            future->task_status = IN_PROGRESS;
            pthread_mutex_unlock(&future->pool->p_lock);

             
            future->result = future->task(future->pool, future->data); 
        
            pthread_mutex_lock(&future->pool->p_lock);
            future->task_status = COMPLETED;
            sem_post(&future->task_done);
            pthread_mutex_unlock(&future->pool->p_lock);
        }
        else if (future->task_status == IN_PROGRESS) {
            pthread_mutex_unlock(&future->pool->p_lock);
            sem_wait(&future->task_done);
        }
        else {
            pthread_mutex_unlock(&future->pool->p_lock);
            sem_wait(&future->task_done); // Just for emergency 
        }
     }
    return future->result;
}

void future_free(struct future *future){
    free(future);
}
