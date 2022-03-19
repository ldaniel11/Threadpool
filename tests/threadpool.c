#include "threadpool.h"
#include "list.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

typedef struct future {
    fork_join_task_t task;  // a function pointer type
    struct list_elem elem; // the future is inserted into the list
    void* args; // the argument for the method thread_pool_submit
    void* result; // store the task result once it completes the execution
    sem_t task_done; // the semaphore used for checking the condition
    int task_status; // 3 status are depcited: has started -0 in progress -1, and has completed -2
    struct thread_pool *pool; // The future is an instance of the task(external access)
    struct worker *worker; // the worker is an instance of the task(internal access)
}future;

typedef struct worker {
    struct list local; // A worker thread consists of a queue containing multiple elements
    struct list_elem obj; // The element in the queue
    pthread_mutex_t local_lock; // We would like to access the lock 
}worker;

typedef struct thread_pool {
    struct list global; // a global queue
    struct list worker; // the workers list we should concentrate on
    int threads; // the total number of threads in the thread pool
    pthread_mutex_t p_lock; // the mutexes lock for the pool list
    pthread_cond_t cond; // the condition variable to be utilized 
    int exit_flag; // a flag used to indicate when the pool is shutting down
    pthread_t *thread_arr; // the thread array in the thread pool
}thread_pool;

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



static bool worker_empty(thread_pool* swimming_pool) {
    pthread_mutex_lock(&swimming_pool->p_lock);

    struct list_elem* temp = list_begin(&swimming_pool->worker);

    while(temp != list_end(&swimming_pool->worker)) {
        worker * curr_worker = list_entry(temp, struct worker, obj);
        if (!list_empty(&curr_worker->local)) {
               pthread_mutex_unlock(&swimming_pool->worker);
               return false; 
        }
        temp = list_next(&temp);
    }
    pthread_mutex_unlock(&swimming_pool->p_lock);
    return true;
}

struct thread_pool *thread_pool_new(int nthreads) {
   // create a thread pool
   thread_pool* swimming_pool = (thread_pool*)calloc(1, sizeof(thread_pool));

   swimming_pool->threads = nthreads;
   list_init(&swimming_pool->global);
   list_init(&swimming_pool->worker);

   swimming_pool->thread_arr = (pthread_t*)calloc(nthreads, sizeof(pthread_t));

   pthread_mutex_init(&swimming_pool->p_lock, NULL);
   pthread_mutex_init(&swimming_pool->cond, NULL);


   // initialize worker threads
   pthread_mutex_lock(&swimming_pool->p_lock);
   int a = 0; // a local variable used to represent the number of threads temporarily

   while (a < swimming_pool->threads) {
       struct worker* mcquain = (worker*)calloc(1, sizeof(worker));

       list_init(&mcquain->local);
       pthread_mutex_init(&mcquain->local_lock, NULL);

       list_push_back(&swimming_pool->worker, &mcquain->obj);

       // I need to create a thread  
       pthread_create(&swimming_pool->thread_arr[a], NULL, worker_thread,swimming_pool);

       a++;
    }
    pthread_mutex_unlock(&swimming_pool->p_lock);

    return swimming_pool;

}

void thread_pool_shutdown_and_destroy(struct thread_pool* pool) {
    pthread_mutex_lock(&pool->p_lock);
    
    pool-> exit_flag = 1; // Right now, 1 is the number shutting down the pool.

    pthread_cond_broadcast(&pool->cond); // Other threads must be waited so that all threads
                                         // can destory altogether
    
    pthread_mutex_unlock(&pool->p_lock);

    int a = 0;
    while(a < pool->threads) {
         pthread_join(pool->thread_arr[a], NULL);
         a++;
    }

    free(pool->thread_arr);
    free(pool);
}

struct future * thread_pool_submit(struct thread_pool *pool, fork_join_task_t task, void * data) { 
    // The main purpose of this task is to submit a fork_join_task 
    // to the threadpool and return its future.

    //(1) insert a mutex lock to achieve its desired implementation
    pthread_mutex_lock(&pool->p_lock);

    //(2) instantiate a target of future* and access its corresponding attributes
    future* future_target = (future*) calloc(1, sizeof(future));
    future_target->pool = pool;
    future_target->task = task;
    future_target->args = data;
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
    

}

void future_free(struct future * future) {

}




