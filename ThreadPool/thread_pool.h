typedef struct {
    void *(*function)(void *);                  /*  function pointer，callback function  */
    void *arg;                                  /*  function parameter  */
} threadpool_task_t;                            /*  task for the child thread  */

/*  thread pool struct  */ 
typedef struct {
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
} threadpool_t;

/*  create the thread pool  */
threadpool_t *threadpool_create(int min_thread_num, int max_thread_num, int queue_max_size);

/*  add a new task to the task queue  */
int add_task(threadpool_t *pool, void *(*function)(void *arg), void *arg);

/*  the new thread starts execution by invoking thread_start  */
void *thread_start(void *threadpool);

/*  the management thread starts execution by invoking management_thread  */
void *management_thread(void *threadpool);

/*  simulate child thread processing task  */
void *process(void *arg);

/*  check whether a thread is alive or not  */
int is_thread_alive(pthread_t tid);

/*  destroy the thread pool  */
int threadpool_destroy(threadpool_t *pool);

/*  deallocates the memory, destroy the mutex object and condition variable  */
int threadpool_free(threadpool_t *pool);




