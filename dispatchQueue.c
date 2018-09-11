#include "dispatchQueue.h"
#include "num_cores.c"
#include <string.h>

task_t *task_create(void (*work)(void *), void *params, char *name)
{
    //knows it is a pointer but not what it is pointing to yet.
    task_t *new_task;
    //only have to allocate memory for pointers of structures. 
    task = malloc(sizeof().task_t);
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

dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
void dispatch_queue_destroy(dispatch_queue_t *);
    
int dispatch_async(dispatch_queue_t *, task_t *);
    
int dispatch_sync(dispatch_queue_t *, task_t *);
    
void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
int dispatch_queue_wait(dispatch_queue_t *);


