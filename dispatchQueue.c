#include "dispatchQueue.h"
#include "num_cores.c"
#include <string.h>
#include <stdlib.h>


// frees memory associated to the task
void task_destroy(task_t *task){
    printf("\ndestroying task with name: %s\n", task->name);
    free(task);
}

/**
 * Takes in nothing. a thread polls through this method continually, either executing a task or waiting 
 * for one to execute! 
 */
void *thread_wrapper_func(void *dispatch_queue) { 
    dispatch_queue_t *queue_pointer = dispatch_queue;

    int sem_value;
    sem_getvalue(queue_pointer->queue_semaphore, &sem_value);
    printf("\nsemaphore value from thread: %d", sem_value);

    while (1) 
    {
        //if semaphore says you are good to go, execute task at head of queue!
        printf("\ncalling sem wait\n");
        
        //waits until there is a task for the thread to execute
        sem_wait(queue_pointer->queue_semaphore);
        printf("\nsem_wait has been executed, task ready to get!\n");

        //waits until the head of the queue is free to be retrieved
        sem_wait(queue_pointer->queue_head_semaphore);
        printf("\nsem_wait has been executed, head of the queue ready to get!\n");
            
        //find the task to execute
        void *params = queue_pointer->head->params;
        void (*work)(void *) = queue_pointer->head->work;

        printf("Starting execution of task with name: %s\n", queue_pointer->head->name);   

        //set the head of the queue to be the next task in the queue and remove previous head
        task_t *next_task = queue_pointer->head->next_task;
        task_destroy(queue_pointer->head);
        queue_pointer->head = queue_pointer->head->next_task;

        //head of the queue is now free for elements to be removed from.
        sem_post(queue_pointer->queue_head_semaphore);        

        //execute the task
        work(params);
      
        //signal 
        printf("Task has been executed!\n"); 
    }
    //function must return something
    return NULL;
}

task_t *task_create(void (*work)(void *), void *params, char *name){
    printf("\ncreating a new task\n");
    //knows it is a pointer but not what it is pointing to yet.
    task_t *new_task;
    //only have to allocate memory for pointers of structures. 
    new_task = malloc(sizeof(task_t));
    //can put stuff in now that we have free memory to do so.
    //name of task for debugging purposes
    strcpy(new_task->name, name);
    printf("new task name: %s\n", new_task->name);
    //function for task to operate
    new_task->work = work;
    //params for the method to invoke on
    new_task->params = params;

    return new_task;
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type){
    printf("\ncreating dispatch queue\n");
    dispatch_queue_t *dispatch_queue = malloc(sizeof(dispatch_queue_t));


    //haven't yet initialised task at the head of the queue
    dispatch_queue->head = NULL;

    dispatch_queue->queue_type = queue_type;

    //semaphore to track how many tasks there are to complete

    sem_t *semaphore = malloc(sizeof(*semaphore));
    int err1 = sem_init(semaphore, 0, 0);
    if (err1 !=0 ) {
        fprintf(stderr, "\nerror creating semaphore\n");
    }
    dispatch_queue -> queue_semaphore = semaphore;

    sem_t *head_semaphore = malloc(sizeof(*head_semaphore));
    int err2 = sem_init(head_semaphore, 0, 1);
    if (err2 !=0 ) {
        fprintf(stderr, "\nerror creating semaphore\n");
    }
    dispatch_queue -> queue_head_semaphore = head_semaphore;

    //sem_init(&(dispatch_queue->queue_semaphore), 0, 0);
    printf("\ninitialised semaphore\n");
    int sem_value;
    sem_getvalue(dispatch_queue->queue_semaphore, &sem_value);
    printf("\nsemaphore value during queue initialisation: %d\n", sem_value);

    //number of threads is 1 if queue is serial
    int num_threads = 1;

    //number of threads in pool is same as number of cores if concurrent queue
    if (queue_type == CONCURRENT)
    {
        num_threads = get_num_cores();
    }
    

    dispatch_queue->thread_queue = (dispatch_queue_thread_t*)malloc(sizeof(dispatch_queue_thread_t) * num_threads);

    //now initialise all the threads to call the polling function!!
    for (int i = 0; i < num_threads; i++) 
    {
        printf("\ncreating thread %i\n", i);
        dispatch_queue_thread_t thread = dispatch_queue->thread_queue[i];

        dispatch_queue_thread_t *thread_pointer = &thread;

        thread_pointer->queue = dispatch_queue;

        //generates a new thread which calls the wrapper function!
        if(pthread_create(&thread_pointer->pthread, NULL, thread_wrapper_func, dispatch_queue)) {
            //something went wrong when generating the pthread
            fprintf(stderr, "\nError creating thread\n");
            return NULL;
        }   
    }
    printf("\ndispatch queue created\n");

    return dispatch_queue;
}
    
void dispatch_queue_destroy(dispatch_queue_t *dispatch_queue){

    printf("\ndestroying dispatch queue\n");
    //TODO complete this method when you understand it a little better :) 
    task_destroy(dispatch_queue->head);
    free(dispatch_queue->thread_queue->queue);
    free(dispatch_queue);

    
}

//adds given task to the tail of the dispatch queue
void add_to_queue(dispatch_queue_t *dispatch_queue, task_t *task){

    printf("\nadding element to the queue\n");
    if (!dispatch_queue->head){ //adding task to the head of the queue
        dispatch_queue->head = task;

    } else { //already elements in the queue, add task to the tail
        task_t *current = dispatch_queue->head;

        while(current->next_task){
            current = current->next_task;
        }
        current->next_task = task;
    }
}

//do before sync! adds a task to the queue
int dispatch_async(dispatch_queue_t *dispatch_queue, task_t *task){
    printf("\ncalling sem post when adding new task: %s\n", task->name);
    //appends the given task to the queue
    add_to_queue(dispatch_queue, task);

    //increment semaphore count when a new task is added to the queue
    if (sem_post(dispatch_queue->queue_semaphore) == 0){
    } else {
        printf("sem_post unsuccessful\n");
    }
    /*
    Need some way of ensuring that the head of the queue can only be accessed at once.
    Have a semaphore that stores if the head of the queue is currently being changed.
    The semaphore is 1 if the head is free to change. when any thread is accessing it 
    it will be set to zero so that getting the element at the head of the queue is 
    blocking code so that head can only be acccessed by one thread at a time.
    */
    return 0;
}
    
int dispatch_sync(dispatch_queue_t *queue, task_t *task) {
   return 0; 
}
    
void dispatch_for(dispatch_queue_t *queue, long param, void (*work)(long)) {
}
    
int dispatch_queue_wait(dispatch_queue_t *queue) {
    return 0;
}


