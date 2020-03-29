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
 *	Revised	: Charalampos Eleftheriadis
 *
 *	Date	: 28 March 2020
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 10000
#define PRO_NUM 6

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

    // Initialize random number seed.
    srand(time(NULL));

    // Open file.
    FILE *fp;
    fp = fopen("dataP6K10-20.csv", "w");

    for (int conNum=1; conNum<129; conNum*=2) {

        // Count from -1 each time.
        outCounter = -1;

        // Allocate array with cells equal to the expected production.
        // Each cell will contain the in-queue waiting time of each produced element.
        int *toWrite = (int *)malloc(LOOP*PRO_NUM*sizeof(int));

        // Initialize queue.
        queue *fifo;
        fifo = queueInit (toWrite);
        if (fifo ==  NULL) {
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }

        pthread_t pro[PRO_NUM];
        pthread_t con[conNum];

        // Create producer and consumer threads.
        for (int i=0; i<conNum; i++)
            pthread_create (&con[i], NULL, consumer, fifo);
        for (int i=0; i<PRO_NUM; i++)
            pthread_create (&pro[i], NULL, producer, fifo);

        // Wait for threads to finish.
        for (int i=0; i<PRO_NUM; i++)
            pthread_join (pro[i], NULL);
        for (int i=0; i<conNum; i++)
            pthread_join (con[i], NULL);

        // Delete queue.
        queueDelete (fifo);

        // Write results to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<LOOP*PRO_NUM; i++)
            fprintf(fp, "%d,", toWrite[i]);
        fprintf(fp, "\n");

        free(toWrite);

        usleep(100000);

    }

    // Close file.
    fclose(fp);

    return 0;
}

// Calculate sin of args with args of type [number of angles, angle #1, angle #2, etc]
void work(void *arg) {
    int *a = (int *)arg;
    double r = 0;
    for (int i=0; i<a[0]; i++)
        r += sin((double)a[i+1]);

    // Print result to screen.
    //printf("%f\n",r);
}

void *producer (void *q) {
    queue *fifo;

    fifo = (queue *)q;

    for (int i=0; i<LOOP; i++) {
		
        // Randomize number of angles between 10 and 20.
		int k = rand() % 10 + 10;
        int *a = (int *)malloc((k+1)*sizeof(int));
        a[0] = k;
		
        // Creates the different angles.
		for (int i=0; i<k; i++)
            a[i+1] = k+i;

        workFunction in;
        in.work = &work;
        in.arg = a;

        pthread_mutex_lock (fifo->mut);
        while (fifo->full) {
            //printf ("producer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        gettimeofday(&in.start, NULL);
        queueAdd (fifo, in);
        pthread_mutex_unlock (fifo->mut);
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
        
		pthread_mutex_lock (fifo->mut);
		
        // Checks the counter to see if the producers work is finished.
		if (outCounter == LOOP*PRO_NUM-1) {
            pthread_mutex_unlock (fifo->mut);
            break;
        }
		
		// Increase the counter indicating that this thread will eventually consume an element.
		// It might wait for the producer to fill in the queue, but eventually it will be there.
        outCounter++;
		
        while (fifo->empty) {
        //printf ("consumer: queue EMPTY.\n");
        pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &out);
        gettimeofday(&end, NULL);
		
		// Calculate the in-queue waiting time.
        fifo->toWrite[outCounter] = (int) ((end.tv_sec-out.start.tv_sec)*1e6 + (end.tv_usec-out.start.tv_usec));
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
		
		// Execute work function.
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
