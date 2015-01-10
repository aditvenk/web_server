#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

typedef struct {
    int connfd; 
    int job_length;
} work_queue_t;
/* Bounder buffer */
work_queue_t* work_queue = NULL;
int fill = 0;
int use = 0;
int count = 0;
int max_count = 0;
pthread_mutex_t m;
pthread_cond_t empty_cv, fill_cv;

#define SJF 1
#define SCHED_POLICY SJF


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

void print_wq () {
    printf("use = %d ; fill = %d; count = %d ; max_count = %d\n", use, fill, count, max_count);
    int i;
    for (i=use; ; i = (i+1)%max_count) {
        printf("%d : %d , ", work_queue[i].connfd, work_queue[i].job_length);
        if ((i+1)%max_count == use) { //next iteration will be back to use
            break;
        }
    }
    printf("\n");
}
int main (int argc, char** argv) {

    pthread_mutex_init (&m, NULL);
    pthread_cond_init(&empty_cv, NULL);
    pthread_cond_init(&fill_cv, NULL);

    
    int num_buffers = 4;
    max_count = num_buffers;
    work_queue = (work_queue_t*) malloc (num_buffers * sizeof(work_queue_t));
    work_queue_t job;
    while (1) {
        printf("enter new value : \n");
        scanf("%d %d", &job.connfd, &job.job_length);
        put_sjf(job);
        print_wq();
    }

    

}
