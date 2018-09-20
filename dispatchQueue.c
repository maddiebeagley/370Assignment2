#include "dispatchQueue.h"
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <stdio.h>


void task_destroy(task_t *task);
volatile int threads_executing  =0;

pthread_mutex_t *lock;

task_t* pop(dispatch_queue_t *queue);
void push(dispatch_queue_t *dispatch_queue, task_t *task);

/**
 * Takes in nothing. a thread polls through this method continually, either executing a task or waiting 
 * for one to execute! 
 */
void *thread_wrapper_func(void *dispatch_queue) { 
    dispatch_queue_t *queue_pointer = dispatch_queue;

    //printf("address of queue from thread is: %p\n", dispatch_queue);
    while (1) {      
        //waits until there is a task for the thread to execute
	    //printf("waiting on the semaphore\n");
        sem_wait(queue_pointer->queue_semaphore);

	pthread_mutex_lock(&lock);
        threads_executing++;
	printf("threads executing: %d\n", threads_executing);
	pthread_mutex_unlock(&lock);
	    //printf("sem_wait has been executed\n");

        //waits until the head of the queue is free to be retrieved
        sem_wait(queue_pointer->task_access_semaphore);

        task_t* current_task = pop(queue_pointer);

        //head of the queue is now free for elements to be retrieved
        sem_post(queue_pointer->task_access_semaphore); 
        
	    //printf("got task %s off queue \n", current_task->name);

        void *params = current_task->params;
        void (*work)(void *) = current_task->work;       

        //execute the task
	printf("about to execute task: %s\n", current_task->name);
        work(params);

        printf("task with name: %s has been executed\n", current_task->name);

        //advertise that the current task has stopped executing
        if (current_task->task_semaphore) {
            sem_post(current_task->task_semaphore);
        }

        task_destroy(current_task);
	printf("threads executing: %d\n", threads_executing);
        //thread is no longer executing after completion of task function
	pthread_mutex_lock(&lock);
        threads_executing--;
	printf("threads decremented\n");
	pthread_mutex_unlock(&lock);
    }
    //function must return something
    return NULL;
}

task_t *task_create(void (*work)(void *), void *params, char *name){
    //allocate memory to the task
    task_t *new_task = malloc(sizeof(task_t));
    
    //name of task for debugging purposes
    strcpy(new_task->name, name);

    //function and input parameters for task to operate on
    new_task->work = work;
    new_task->params = params;
    new_task->next_task = NULL;

    return new_task;
}

// frees memory associated to the input task
void task_destroy(task_t *task){
    free(task);
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type){
    dispatch_queue_t *dispatch_queue = malloc(sizeof(dispatch_queue_t));
    //printf("address of queue from creation %p", dispatch_queue);

    //haven't yet initialised task at the head of the queue
    dispatch_queue->head = NULL;

    dispatch_queue->queue_type = queue_type;

    //semaphore to track how many tasks there are to complete
    sem_t *semaphore = malloc(sizeof(*semaphore));
    if (sem_init(semaphore, 0, 0) != 0) {
        fprintf(stderr, "\nerror creating semaphore\n");
    }
    dispatch_queue->queue_semaphore = semaphore;

    //semaphor to track if the head of the queue is currently being retrieved
    //only one thread should retrieve the head of the queue at any given time
    sem_t *head_semaphore = malloc(sizeof(*head_semaphore));
    if (sem_init(head_semaphore, 0, 1) !=0 ) {
        fprintf(stderr, "\nerror creating semaphore\n");
    }
    dispatch_queue->task_access_semaphore = head_semaphore;

    //number of threads is 1 if queue is serial
    int num_threads = 1;

    //number of threads in pool is same as number of cores if concurrent queue
    if (queue_type == CONCURRENT){
        num_threads = get_nprocs();
    }
	
	//printf("there are %d threads allocated\n", num_threads);

    //number of threads executing tasks is initially 0

    //allocate memory for the thread pool
    dispatch_queue->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);

    //initialise all the threads to call the polling function and wait for semaphore signal
    for (int i = 0; i < num_threads; i++) {
    //printf("making a thread\n");
        pthread_t thread = dispatch_queue->threads[i];
        pthread_t *thread_pointer = &thread;

        //generates a new thread which calls the polling wrapper function
        if(pthread_create(thread_pointer, NULL, thread_wrapper_func, dispatch_queue)) {
            fprintf(stderr, "\nError creating thread\n");
            return NULL;
        }   
    }
    return dispatch_queue;
}

//removes all memory allocated to the dispatch queue
void dispatch_queue_destroy(dispatch_queue_t *dispatch_queue){

    // if there are elements in the queue, free their memory
    if (dispatch_queue->head){     
        task_t *curr_task = dispatch_queue->head;
        while (curr_task->next_task) {
            task_t* next_task = curr_task->next_task;
            task_destroy(curr_task);
            curr_task = next_task;
        }
        task_destroy(curr_task);
    }

    //free the memory of the list of threads
    free(dispatch_queue->threads);

    //free the memory of the queue semaphores
    free(dispatch_queue->queue_semaphore);
    free(dispatch_queue->task_access_semaphore);
    free(dispatch_queue);   
}

int dispatch_async(dispatch_queue_t *dispatch_queue, task_t *task){
    //appends the given task to the queue
    push(dispatch_queue, task);

    //increment semaphore count when a new task is added to the queue
    if (sem_post(dispatch_queue->queue_semaphore) == 0){
    } else {
        printf("sem_post unsuccessful\n");
    }
    return 0;
}
    
int dispatch_sync(dispatch_queue_t *queue, task_t *task) {
    //initialise a semaphore to store when a task has been executed
    sem_t *semaphore = malloc(sizeof(*semaphore));
    if (sem_init(semaphore, 0, 0) !=0 ) {
        fprintf(stderr, "\nerror creating semaphore\n");
    }
    task->task_semaphore = semaphore;

    //append given task to dispatch queue
    sem_wait(queue->task_access_semaphore);
    push(queue, task);
    sem_post(queue->task_access_semaphore);

    //advertise new task to execute
    sem_post(queue->queue_semaphore);

    //wait until task semaphor signals completion of task
    sem_wait(task->task_semaphore);
    
    return 0; 
}
    
void dispatch_for(dispatch_queue_t *queue, long number, void (*work)(long)) {

    //initialise "number" tasks with appropriate inputs and append to queue
    for(long i = 0; i < number; i++) {
        long num = i;
        task_t* task = task_create((void(*)(void*))work, (void*)num, "task");
        dispatch_async(queue, task);
    }

    //wait until all elements in the queue have been executed
    dispatch_queue_wait(queue);
	dispatch_queue_destroy(queue);
}
    
int dispatch_queue_wait(dispatch_queue_t *queue) {
    //poll to see if all threads have finished executing tasks and 
    //there are no tasks left on the queue.
    while(1){
        if (threads_executing == 0 && !queue->head){
            return 0;
        }
    }
}

//******************* HELPER METHODS FOR QUEUE **********************

//adds given task to the tail of the dispatch queue
void push(dispatch_queue_t *dispatch_queue, task_t *task){
    //printf("adding a new task to the queue\n");
    if (!dispatch_queue->head){ //adding task to the head of the queue
        dispatch_queue->head = task;

    } else { //already elements in the queue, add task to the tail
        task_t *current = dispatch_queue->head;
        //printf("head is: %s\n", current->name);
        //continue cycling through queue until tail is reached
        while(current->next_task){
            current = current->next_task;
            //printf("next task is %s\n", current->name);
        }
        //assign current task to tail of queue
        current->next_task = task;
    }
}

task_t* pop(dispatch_queue_t *queue){
    //printf("popping an element off the queue\n");
    //set current task to execute as the head of the queue
    task_t* current_task = queue->head;
    //printf("name of head is: %s\n", current_task->name);

    //set the head of the queue to be the next task in the queue and remove previous head
    task_t *next_task = current_task->next_task;
    //task_destroy(queue->head);
    queue->head = queue->head->next_task;
    //printf("new head is %s\n", queue->head->name);

    return current_task;
}
