#include "cs537.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

#define FCFS    0
#define SJF     1
#define SCHED_POLICY    FCFS


/* Bounder buffer */
work_queue_t* work_queue = NULL;
int fill = 0;
int use = 0;
int count = 0;
int max_count = 0;
pthread_mutex_t m;
pthread_cond_t empty_cv, fill_cv;

// CS537: Parse the new arguments too
void getargs(int *port, int *num_threads, int *num_buffers, int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *num_threads = atoi(argv[2]);
    if (*num_threads < 0) {
        fprintf(stderr, "Number of threads must be positive integer!\n");
        exit(1);
    }
    *num_buffers = atoi(argv[3]);
    if (*num_buffers < 0) {
        fprintf(stderr, "Number of buffers must be positive integer!\n");
        exit(1);
    }
}

void put (work_queue_t value) {
    assert (count<max_count); 
    /* in FCFS we will push the new value at the fill posn. The worker thread will consume from use posn */
    work_queue[fill] = value;
    fill = (fill+1)%max_count;
    count++;
}

void put_sjf (work_queue_t value) {
    assert (count<max_count); 
    assert (SCHED_POLICY == SJF);

    if (count == 0) {
        work_queue[fill] = value;
        fill = (fill+1)%max_count;
        count++;
        return;
    }

    int i, j;
    for (i=use ; i!=fill ; i=((i+1)%max_count)) {
        if (work_queue[i].job_length <= value.job_length)
            continue;
        break;
    }
    for (j = i; j!=fill; j=((j+1)%max_count)) {
        work_queue[(j+1)%max_count] = work_queue[j];
    }
    work_queue[i] = value;
    fill = (fill+1)%max_count;
    count++;
}

work_queue_t get () {
    /* this API will return the value in the work-queue at use posn */
    assert(count>0);
    work_queue_t tmp = work_queue[use];
    use = (use+1) % max_count;
    count--;
    return tmp;
}

void * worker (void* args) {
    thread_args_t* myargs = (thread_args_t*) args;
    printf("Worker thread %d saying hello! \n", myargs->thread_id);

    // each worker will now go into an infinite loop - waiting to serve requests.

    while (1) {
        /* critical section for retrieving connection fd from work queue. Servicing will be outside the critical section */
        Pthread_mutex_lock (&m);
        while (count == 0)
            Pthread_cond_wait(&fill_cv, &m);
        work_queue_t job = get();
        Pthread_cond_signal(&empty_cv);
        Pthread_mutex_unlock(&m);
        int connfd = job.connfd;
        if (SCHED_POLICY == FCFS)
            requestHandle(connfd);
        else {
            printf("Worker %d, handling job (length = %d) \n", myargs->thread_id, job.job_length);
            requestHandleFinal (&job);
        }
        
        Close(connfd);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd, port, num_threads, num_buffers, i, connfd, clientlen;
    pthread_t *clients;
    thread_args_t * args;
    struct sockaddr_in clientaddr;
    
    Pthread_mutex_init(&m, NULL);
    Pthread_cond_init(&empty_cv, NULL);
    Pthread_cond_init(&fill_cv, NULL);

    getargs(&port, &num_threads, &num_buffers, argc, argv);

    /* Spawn pool of worker threads */
    clients = (pthread_t*) Malloc (num_threads * sizeof(pthread_t));
    args = (thread_args_t*) Malloc (num_threads * sizeof(thread_args_t));
    work_queue = (work_queue_t*) Malloc (num_buffers * sizeof(work_queue_t));

    max_count = num_buffers; // set the global variable for buffer limit. 
    for ( i=0 ; i< num_threads; i++) {
        args[i].thread_id = i+1;
        Pthread_create (&clients[i], NULL, &worker, (void*) &args[i]);
    }

    listenfd = Open_listenfd(port);

    while (1) {
        
        clientlen = sizeof (clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        // push the fd into the buffer. 
        /* critical section for pushing the connection fd into work-queue. */
        Pthread_mutex_lock (&m);
        while (count == max_count)
            Pthread_cond_wait (&empty_cv, &m);
        int newfd = Dup(connfd);
        work_queue_t newjob;
        newjob.connfd = newfd;
        if (SCHED_POLICY == FCFS) {
            put (newjob); 
            goto loopend;
        }
        /* SJF policy. Need to do prelim processing of request to find out job length. */
        requestHandleInitial(newfd, &newjob);
        put_sjf(newjob);

loopend:
        Pthread_cond_signal (&fill_cv);
        Pthread_mutex_unlock (&m);
        Close(connfd);
    }
}


    


 
