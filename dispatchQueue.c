#include "dispatchQueue.h"
#include "num_cores.c"
#include <string.h>

task_t *task_create(void (*work)(void *), void *params, char *name)
{
    //knows it is a pointer but not what it is pointing to yet.
    task_t *new_task;
    //only have to allocate memory for pointers of structures. 
    new_task = malloc(sizeof().task_t);
    //can put stuff in now that we have free memory to do so.
    //name of task for debugging purposes
    strcpy(new_task->name, name);
    //function for task to operate
    new_task->work = work;
    //params for the method to invoke on
    new_task->params = params;

    return new_task;
}

// frees memory associated to the task
void task_destroy(task_t *task){
    free(task)
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type){

    dispatch_queue_t *dispatch_queue;
    //allocates memory for the queue to be stored
    dispatch_queue = malloc(sizeof().dispatch_queue_t);

    //allocates memory for the first task in the queue. currently no actual task.
    task_t->head = (task_t*)malloc(sizeof().task_t);

    //number of threads is 1 if queue is serial
    int num_threads = 1;

    //number of threads in pool is same as number of cores if concurrent queue
    if (queue_type == CONCURRENT)
    {
        num_threads = get_num_cores();
    }

    task_t->thread_queue = (dispatch_queue_thread_t*)malloc(sizeof(dispatch_queue_thread_t) * num_threads);




}
    
void dispatch_queue_destroy(dispatch_queue_t *dispatch_queue){
    free(dispatch_queue->thread_queue);
    free(dispatch_queue);
}
    
int dispatch_async(dispatch_queue_t *, task_t *);
    
int dispatch_sync(dispatch_queue_t *, task_t *);
    
void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
int dispatch_queue_wait(dispatch_queue_t *);


