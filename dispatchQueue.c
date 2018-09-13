#include "dispatchQueue.h"
#include "num_cores.c"
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>

/**
 * Takes in nothing. a thread polls through this method continually, either executing a task or waiting 
 * for one to execute! 
 */
void *thread_wrapper_func(void *dispatch_queue) { 
    dispatch_queue_t *queue_pointer = dispatch_queue;
    printf("\nstarting the thread wrapper function...\n");

    while (1) 
    {
        printf("\ni have entered the while loop. waiting for semaphore to poll.\n");

        //if semaphore says you are good to go, execute task at head of queue!
        sem_wait(&queue_pointer->queue_semaphore);
        printf("\nsem_wait has been executed!");
        
        //find the task to execute
        task_t *task_pointer = queue_pointer->head;
        task_t task = *task_pointer;

        void *params = task_pointer->params;
        void (*work)(void *) = task_pointer->work;  

        printf("\nStarting execution of task with name: %s\n", task_pointer->name); 

        //execute the task
        work(params);
      
        //signal 
        printf("\nTask has been executed!\n"); 

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
    //function for task to operate
    new_task->work = work;
    //params for the method to invoke on
    new_task->params = params;

printf("\nnewtask about to be returned\n");
    return new_task;
}

// frees memory associated to the task
void task_destroy(task_t *task){
    free(task);
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type){
    printf("\nmaking dispatch queue\n");

    dispatch_queue_t *dispatch_queue;
    //allocates memory for the queue to be stored
    dispatch_queue = malloc(sizeof(dispatch_queue_t));

    //allocates memory for the first task in the queue. currently no actual task.
    dispatch_queue->head = (task_t*)malloc(sizeof(task_t));

    dispatch_queue->queue_type = queue_type;

    //semaphore to track how many tasks there are to complete
    printf("\nmaking semaphore\n");
    sem_init(&dispatch_queue->queue_semaphore, 0, 0);
    printf("\nhave made semaphore\n");

    //number of threads is 1 if queue is serial
    int num_threads = 1;

    //number of threads in pool is same as number of cores if concurrent queue
    if (queue_type == CONCURRENT)
    {
        num_threads = get_num_cores();
    }

    printf("\nmaking thread queue in dispatch queue\n");
    dispatch_queue->thread_queue = (dispatch_queue_thread_t*)malloc(sizeof(dispatch_queue_thread_t) * num_threads);

    //now initialise all the threads to call the polling function!!
    for (int i = 0; i < num_threads; i++) 
    {
        dispatch_queue_thread_t thread = dispatch_queue->thread_queue[i];

        dispatch_queue_thread_t *thread_pointer = &thread;

        thread_pointer->queue = dispatch_queue;

        printf("\ngenerating a new thread\n");
        //generates a new thread which calls the wrapper function!
        if(pthread_create(&thread_pointer->pthread, NULL, thread_wrapper_func, &dispatch_queue)) {
            //something went wrong when generating the pthread
            fprintf(stderr, "\nError creating thread\n");
            return NULL;
        }   
        printf("\nmade thread\n");    
    }
    printf("\ni finish making the queue\n");
}
    
void dispatch_queue_destroy(dispatch_queue_t *dispatch_queue){
    //TODO complete this method when you understand it a little better :) 
    free(dispatch_queue->thread_queue);
    free(dispatch_queue);
}

//adds given task to the tail of the dispatch queue
void add_to_queue(dispatch_queue_t *dispatch_queue, task_t *task){
    printf("\nadding an element to the queue\n");
    task_t *current = dispatch_queue->head;

    while(current->next_task){
        current = current->next_task;
    }

    current->next_task = task;
}

//do before sync! adds a task to the queue
int dispatch_async(dispatch_queue_t *dispatch_queue, task_t *task){
    //need a semaphor to store when there are tasks arriving
    //sem_t semaphore = dispatch_queue->queue_semaphore;

    add_to_queue(dispatch_queue, task);

    //increment semaphore count when a new task is added to the queue
    sem_post(&dispatch_queue->queue_semaphore);

    return 0;
}
    
int dispatch_sync(dispatch_queue_t *, task_t *);
    
void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
int dispatch_queue_wait(dispatch_queue_t *);


