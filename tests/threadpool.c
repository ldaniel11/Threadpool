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
    int exit_flag; // a flag used to indicate when the pull is shutting down
    pthread_t *thread_arr; // the thread array in the thread pool
}thread_pool;

static void * worker_thread(void *p) {
     
//      thread_pool* swimming_pool = (thread_pool*) p;

//      pthread_mutex_lock(&swimming_pool->p_lock);

     
//      int a = 0; // How many times you have already encountered
//      bool found = false; // a boolean value indicating whether or not you find the thread

//      worker* candidate = NULL;

//      struct list_elem* iteration = list_begin(&swimming_pool->worker);

//      while(iteration != list_end(&swimming_pool->worker) && !found) {

//          if (pthread_self() == swimming_pool->thread_arr[a]) {
//               candidate = list_entry(iteration, struct worker, elem);
//               found = true;
//          }
//          a++;
//          iteration = list_next(iteration);
//     }

//     pthread_mutex_unlock(&swimming_pool->p_lock);



}

// create a thread pool
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

}

struct future * thread_pool_submit(struct thread_pool *pool, fork_join_task_t task, void * data) {

}

void *future_get(struct future * future) {

}

void future_free(struct future * future) {

}




