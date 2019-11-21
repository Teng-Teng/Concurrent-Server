#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include "threadPool.h"

#define DEFAULT_TIME 10                         /*  check for every 10 seconds  */
#define MIN_WAIT_TASK_NUM 10                    /*  if queue_size > MIN_WAIT_TASK_NUM, add new thread to the thread pool  */
#define DEFAULT_THREAD_VARY 10                  /*  the number of threads created or destroyed each time  */
#define true 1
#define false 0

typedef struct {
    void *(*function)(void *);                  /*  function pointer，callback function  */
    void *arg;                                  /*  function parameter  */
} threadpool_task_t;                            /*  task for the child thread  */

/*  thread pool struct  */ 
struct threadpool_t {
    pthread_mutex_t lock;                       /*  lock of the thread pool  */
    pthread_mutex_t thread_counter;             /*  lock of the busy thread--busy_thread_num  */
    
    pthread_cond_t queue_not_full;              /*  when the task queue is full, the thread responsible for adding tasks is blocked, waiting for the condition variable  */
    pthread_cond_t queue_not_empty;             /*  when the task queue is not full , notify the thread waiting for the task  */
    
    pthread_t *threads;                         /*  an array to store tid for all the threads in the threadpool  */
    pthread_t management_tid;                   /*  management thread tid  */
    threadpool_task_t *task_queue;              /*  task queue  */
    
    int min_thread_num;                         /*  minimum thread number in the thread pool  */
    int max_thread_num;                         /*  maximum thread number in the thread pool  */
    int live_thread_num;                        /*  the number of living threads  */
    int active_thread_num;                      /*  the number of threads that was started and is not waiting on IO or on a lock  */
    int wait_exit_thread_num;                   /*  the number of threads to destroy  */
    
    int queue_front;                            
    int queue_rear;                             
    int queue_size;                             /*  actual number of tasks in the queue */
    int queue_max_size;                         /*  the maximum number of tasks the queue can hold  */
    
    int shutdown;                               /*  a flag to show thread pool status--true or false  */
};

/*  create the thread pool  */
threadpool_t *threadpool_create(int min_thread_num, int max_thread_num, int queue_max_size) {
    int i;
    threadpool_t *pool = NULL;                                          
    
    do {
        if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
            printf("malloc thread pool failed\n");
            break;
        }
        
        pool->min_thread_num = min_thread_num;
        pool->max_thread_num = max_thread_num;
        pool->active_thread_num = 0;
        pool->live_thread_num = min_thread_num;
        pool->wait_exit_thread_num = 0;
        pool->queue_size = 0;
        pool->queue_max_size = queue_max_size;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;
        
        /*  according to the maximum thread num, allocate memory block for thread array, sets the first max_thread_num bytes of the memory block to 0  */
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thread_num);
        if (pool->threads == NULL) {
            printf("malloc threads failed\n");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t) * max_thread_num);
        
        /*  according to the queue_max_size, allocate memory block for the task queue  */
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_max_size);
        if (pool->task_queue == NULL) {
            printf("malloc task_queue failed\n");
            break;
        }
        
        /*  initialize the mutex and condition variables  */
        if (pthread_mutex_init(&(pool->lock), NULL) != 0 || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
            || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0 || pthread_cond_init(&(pool->queue_not_full), NULL) != 0) {
            printf("initialize the mutex or condition variables failed\n");
            break;
        }
        
        /*  create min_thread_num new thread  */
        for (i = 0; i < min_thread_num; i++) {
            pthread_create(&(pool->threads[i]), NULL, thread_start, (void *)pool);
            printf("create thread 0x%x...\n", (unsigned int)pool->threads[i]);
        }
        
        /*  create management thread  */
        pthread_create(&(pool->management_tid), NULL, management_thread, (void *)pool);
        return pool;
        
    } while (0);
    
    /*  deallocates the thread pool memory previously allocated by a call to malloc  */
    threadpool_free(pool);
    return NULL;
}

/*  add a new task to the task queue  */
int add_task(threadpool_t *pool, void *(*function)(void *arg), void *arg) {
    pthread_mutex_lock(&(pool->lock));
    
    while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)) 
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
        
    if (pool->shutdown) {
        pthread_cond_broadcast(&(pool->queue_not_empty));
        pthread_mutex_unlock(&(pool->lock));
        return 0;
    }
    
    if (pool->task_queue[pool->queue_rear].arg != NULL)
        pool->task_queue[pool->queue_rear].arg = NULL;
        
    /*  add a new task to the task queue  */
    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;                   /*  move the tail pointer, simulate circular queue  */
    pool->queue_size++;
    
    pthread_cond_signal(&(pool->queue_not_empty));                                      /*  unblock at least one of the threads that are blocked on the condition variable cond (if any threads are blocked on cond).  */
    pthread_mutex_unlock(&(pool->lock));
    
    return 0;
}

/*  the new thread starts execution by invoking thread_start  */
void *thread_start(void *threadpool) {
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;
    
    while (true) {
        pthread_mutex_lock(&(pool->lock));                                              /*  lock must be taken to wait on condition variable  */
        
        /*  if there is no task, the calling thread would block on the condition variable  */
        while ((pool->queue_size == 0) && (!pool->shutdown)) {                          
            printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
            
            /*  remove idle thread  */
            if (pool->wait_exit_thread_num > 0) {                                       
                pool->wait_exit_thread_num--;
                
                /*  unlock mutex, terminate calling thread  */
                if (pool->live_thread_num > pool->min_thread_num) {                     
                    printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
                    pool->live_thread_num--;
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
            }
        }
    }
    return NULL;
}

/*  the management thread starts execution by invoking management_thread  */
void *management_thread(void *threadpool) {
    int i;
    threadpool_t *pool = (threadpool_t *)threadpool;
    
    while (!pool->shutdown) {
        sleep(DEFAULT_TIME);                                            /*  management thread pool at regular intervals  */
        
        pthread_mutex_lock(&(pool->lock));
        int queue_size = pool->queue_size;
        int live_thread_num = pool->live_thread_num;
        pthread_mutex_unlock(&(pool->lock));
        
        pthread_mutex_lock(&(pool->thread_counter));
        int active_thread_num = pool->active_thread_num;
        pthread_mutex_unlock(&(pool->thread_counter));
        
        /*  algorithm for creating new threads  */
        if (queue_size >= MIN_WAIT_TASK_NUM && live_thread_num < pool->max_thread_num) {        
            pthread_mutex_lock(&(pool->lock));
            int add = 0;
            
            /*  create DEFAULT_THREAD_VARY threads each time  */
            for (i = 0; i < pool->max_thread_num && add < DEFAULT_THREAD_VARY && pool->live_thread_num < pool->max_thread_num; i++) {
                if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i])) {
                    pthread_create(&(pool->threads[i]), NULL, thread_start, (void *)pool);
                    add++;
                    pool->live_thread_num++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }
        
        /*  algorithm for killing redundant idle threads  */
        if ((active_thread_num * 2) < live_thread_num && live_thread_num > pool->min_thread_num) {
            /*  kill DEFAULT_THREAD_VARY threads each time  */
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_thread_num = DEFAULT_THREAD_VARY;
            pthread_mutex_unlock(&(pool->lock));
            
            for (i = 0; i < DEFAULT_THREAD_VARY; i++) 
                pthread_cond_signal(&(pool->queue_not_empty));          /*  unblock at least one of the threads that are blocked on the condition variable cond (if any threads are blocked on cond).
                                                                        and call pthread_exit() function to terminate the calling thread  */
        }
    }
    return NULL;
}

/*  simulate child thread processing task  */
void *process(void *arg) {
    printf("thread 0x%x working on task %d\n", (unsigned int)pthread_self(), (int)arg);
    sleep(1); 
    printf("task %d completed\n", (int)arg);
    
    return NULL;
}

int main(void) {
    threadpool_t *thread_pool = threadpool_create(3, 100, 100);         /*  create the thread pool  */
    printf("thread pool created");
    
    int num[20], i;                                                     /*  int *num = (int *)malloc(sizeof(int)*20);  */  
    for (i = 0; i < 20; i++) {
        num[i] = i;
        printf("add task %d\n", i);
        
        add_task(thread_pool, process, (void *)&num[i]);          /*  add a new task to the task queue  */
    }
    
    sleep(10);                                                          /*  simulate other tasks done by the server, for example, wait for the child threads to complete the task，
                                                                        the server establish connection with the client, callback event, read and write data  */
                                                              
    threadpool_destroy(thread_pool);                                    /*  destroy the thread pool  */
    
    return 0;
}










