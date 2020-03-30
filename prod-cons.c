/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 100000
#define PRO_NUM 1

void *producer (void *args);
void *consumer (void *args);

typedef struct {
    void *(*work)(void *);
    void *arg;
    struct timeval start;
} workFunction;

typedef struct {
    workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
    int *toWrite;
} queue;

queue *queueInit (int *toWrite);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

// A counter for the consumers to know when all produced elements have been consumed, so they can now quit.
int outCounter;

int main () {

    // Initializes random number seed.
    srand(time(NULL));

    // Opens file.
    FILE *fp;
    fp = fopen("dataP1K100-200.csv", "w");

    for (int conNum=1; conNum<129; conNum*=2) {

        // Prints a message.
        printf("#Cons=%d Started.\n",conNum);

        // outCounter begins from -1 each time.
        outCounter = -1;

        // Allocates array with cells equal to the expected production.
        // Each cell will contain the in-queue waiting time of each produced element.
        int *toWrite = (int *)malloc(LOOP*PRO_NUM*sizeof(int));

        // Initializes queue.
        queue *fifo;
        fifo = queueInit (toWrite);
        if (fifo ==  NULL) {
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }

        pthread_t pro[PRO_NUM];
        pthread_t con[conNum];

        // Creates producer and consumer threads.
        for (int i=0; i<conNum; i++)
            pthread_create (&con[i], NULL, consumer, fifo);
        for (int i=0; i<PRO_NUM; i++)
            pthread_create (&pro[i], NULL, producer, fifo);

        // Waits for threads to finish.
        for (int i=0; i<PRO_NUM; i++)
            pthread_join (pro[i], NULL);
        for (int i=0; i<conNum; i++)
            pthread_join (con[i], NULL);

        // Deletes queue.
        queueDelete (fifo);

        // Writes results to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<LOOP*PRO_NUM; i++)
            fprintf(fp, "%d,", toWrite[i]);
        fprintf(fp, "\n");

        // Releases memory.
        free(toWrite);

        // Sleeps for 100ms before next iteration.
        sleep(0.1);

    }

    // Closes file.
    fclose(fp);

    return 0;
}

void work(void *arg) {
    int *a = (int *)arg;
    double r = 0;
    for (int i=0; i<a[0]; i++)
        r += sin((double)a[i+1]);

    // Prints result to screen.
    //printf("%f\n",r);
}

void *producer (void *q) {
    queue *fifo;

    fifo = (queue *)q;

    for (int i=0; i<LOOP; i++) {

        // Creates the work funtion arguments. k is the number of them.
        int k = (rand() % 101) + 100;
        int *a = (int *)malloc((k+1)*sizeof(int));
        a[0] = k;
        for (int i=0; i<k; i++)
            a[i+1] = k+i;

        // Creates the element that will be added to the queue.
        workFunction in;
        in.work = &work;
        in.arg = a;

        // Critical section begins.
        pthread_mutex_lock (fifo->mut);

        while (fifo->full) {
            //printf ("producer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        gettimeofday(&in.start, NULL);
        queueAdd (fifo, in);

        // Critical section ends.
        pthread_mutex_unlock (fifo->mut);

        // Signals the consumer that queue is not empty.
        pthread_cond_signal (fifo->notEmpty);

    }

    return (NULL);
}

void *consumer (void *q) {
    queue *fifo;
    workFunction out;
    struct timeval end;

    fifo = (queue *)q;

    while (1) {

        // Critical section begins.
        pthread_mutex_lock (fifo->mut);

        // Checks if the number of consumed elements has matched the production. If yes, then this consumer exits.
        if (outCounter == LOOP*PRO_NUM-1) {
            pthread_mutex_unlock (fifo->mut);
            break;
        }

        // This consumer is going to consume an element, so the outCounter is increased.
        outCounter++;

        while (fifo->empty) {
            //printf ("consumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &out);
        gettimeofday(&end, NULL);
        fifo->toWrite[outCounter] = (int) ((end.tv_sec-out.start.tv_sec)*1e6 + (end.tv_usec-out.start.tv_usec));

        // Critical section ends.
        pthread_mutex_unlock (fifo->mut);

        // Signals to producer that queue is not full.
        pthread_cond_signal (fifo->notFull);

        // Executes work outside the critical section.
        out.work(out.arg);

    }

    return (NULL);
}

queue *queueInit (int *toWrite) {
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    q->toWrite = toWrite;

    return (q);
}

void queueDelete (queue *q) {
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);

    free (q);
}

void queueAdd (queue *q, workFunction in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel (queue *q, workFunction *out) {
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}
