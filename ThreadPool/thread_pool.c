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
} threadpool_task_t;                            /*  the task of child thread  */

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




int main(void) {
    threadpool_t *thread_pool = threadpool_create(3, 100, 100);         /*  create the thread pool  */
    printf("thread pool created");
    
    int num[20], i;                                                     /*  int *num = (int *)malloc(sizeof(int)*20);  */  
    for (i = 0; i < 20; i++) {
        num[i] = i;
        printf("add task %d\n", i);
        
        threadpool_add(thread_pool, process, (void *)&num[i]);          /*  add tasks  */
    }
    
    sleep(10);                                                          /*  Waiting for the child threads to complete the task  */
    threadpool_destroy(thread_pool);                                    /*  destroy the thread pool  */
    
    return 0;
}




