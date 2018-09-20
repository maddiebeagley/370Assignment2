/* 
 * File:   dispatchQueue.h
 * Author: robert
 *
 * Modified by: your UPI
 */

#ifndef DISPATCHQUEUE_H
#define	DISPATCHQUEUE_H

#include <pthread.h>
#include <semaphore.h>
    
#define error_exit(MESSAGE)     perror(MESSAGE), exit(EXIT_FAILURE)

    typedef enum { // whether dispatching a task synchronously or asynchronously
        ASYNC, SYNC
    } task_dispatch_type_t;
    
    typedef enum { // The type of dispatch queue.
        CONCURRENT, SERIAL
    } queue_type_t;

    typedef struct task {
        char name[64];              // to identify it when debugging
        void (*work)(void *);       // the function to perform
        void *params;               // parameters to pass to the function
        struct task *next_task;     // stores the task to be accessed next
        sem_t *task_semaphore;      // tracks whether a task has been completed
    } task_t;
    
    typedef struct dispatch_queue_t dispatch_queue_t;     // the dispatch queue type

    struct dispatch_queue_t {
        queue_type_t queue_type;            // the type of queue - serial or concurrent
        task_t *head;                       // pointer to the first task to be executed
        pthread_t *threads;                 // pointer to list of threads for queue to use
        sem_t *queue_semaphore;             // tracks how many tasks are ready to execute
        sem_t *task_access_semaphore;        // used to ensure head of queue is not accessed concurrently
    };
    
    task_t *task_create(void (*)(void *), void *, char*);
    
    void task_destroy(task_t *);

    dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
    void dispatch_queue_destroy(dispatch_queue_t *);
    
    int dispatch_async(dispatch_queue_t *, task_t *);
    
    int dispatch_sync(dispatch_queue_t *, task_t *);
    
    void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
    int dispatch_queue_wait(dispatch_queue_t *);

#endif	/* DISPATCHQUEUE_H */