#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_COUNTERS 5
#define BUFFER_CAPACITY 10
#define COUNTERS_MAXSLEEP 5
#define COUNTERS_MINSLEEP 3
#define MONITOR_MAXSLEEP 1
#define MONITOR_MINSLEEP 1
#define COLLECTOR_MAXSLEEP 5
#define COLLECTOR_MINSLEEP 3
#define GET_RANDOM(min, max) ((rand() % ((max) - (min) + 1)) + (min))

// Constant Flags
#define UNIQUE_MESSAGES 1
#define DISPLAY_MONITOR 1
#define DISPLAY_COLLECTOR 1
#define DISPLAY_COUNTERS 1

typedef struct
{
    sem_t mutex;
    int counter;
} Mutex_Counter;

typedef struct
{
    Mutex_Counter *mut_counter;
    int mCounterID;
} mCounterArg;

typedef struct
{
    sem_t mutex;
    sem_t empty;
    sem_t full;
    int *buffer;
    int capacity;
} Mutex_Buffer;

typedef struct
{
    Mutex_Counter *mut_counter;
    Mutex_Buffer *mut_buf;
    int unique_messages;
} mMonitorArg;

Mutex_Counter *initMutex_Counter()
{
    Mutex_Counter *mut_counter = (Mutex_Counter *)malloc(sizeof(Mutex_Counter));
    mut_counter->counter = 0;
    sem_init(&(mut_counter->mutex), 0, 1);
    return mut_counter;
}

mCounterArg *initCounterArg(int mCounterID, Mutex_Counter *mut_counter)
{
    mCounterArg *mcarg = (mCounterArg *)malloc(sizeof(mCounterArg));
    mcarg->mut_counter = mut_counter;
    mcarg->mCounterID = mCounterID;
    return mcarg;
}

mMonitorArg *initMonitorArg(Mutex_Counter *mut_counter, Mutex_Buffer *mut_buf, int unique_messages)
{
    mMonitorArg *marg = (mMonitorArg *)malloc(sizeof(mMonitorArg));
    marg->mut_counter = mut_counter;
    marg->mut_buf = mut_buf;
    marg->unique_messages = unique_messages;
    return marg;
}

Mutex_Buffer *initMutexBuffer(int capacity)
{
    Mutex_Buffer *mut_buf = (Mutex_Buffer *)malloc(sizeof(Mutex_Buffer));
    mut_buf->capacity = capacity;
    mut_buf->buffer = (int *)malloc(sizeof(int));
    sem_init(&(mut_buf->mutex), 0, 1);
    sem_init(&(mut_buf->full), 0, 0);
    sem_init(&(mut_buf->empty), 0, capacity);
    return mut_buf;
}

void *mCounter(void *arg)
{
    mCounterArg *mcarg = (mCounterArg *)arg;
    sem_t *mutex = &(mcarg->mut_counter->mutex);
    int counter_id = mcarg->mCounterID;
    int *counter = &(mcarg->mut_counter->counter);

    while (1)
    {
        sleep(GET_RANDOM(COUNTERS_MINSLEEP, COUNTERS_MAXSLEEP));
        if (DISPLAY_COUNTERS)
        {
            printf("Counter Thread: %d received a message\nCounter Thread: %d waiting to write\n", counter_id, counter_id);
        }
        sem_wait(mutex);
        // Critical Section
        (*counter)++;
        if (DISPLAY_COUNTERS)
        {
            printf("Counter Thread: %d now increasing the counter, counter value = %d\n", counter_id, *counter);
        }
        sleep(GET_RANDOM(COUNTERS_MINSLEEP, COUNTERS_MAXSLEEP));
        if (DISPLAY_COUNTERS)
        {
            printf("Counter Thread: %d Leaving the counter\n", counter_id);
        }
        sem_post(mutex);
    }
}

void *mMonitor(void *arg)
{
    mMonitorArg *m_arg = (mMonitorArg *)arg;
    sem_t *mutex = &(m_arg->mut_counter->mutex);
    int *counter = &(m_arg->mut_counter->counter);
    int stored_counter = 0;

    sem_t *mutex_buff = &(m_arg->mut_buf->mutex);
    sem_t *empty = &(m_arg->mut_buf->empty);
    sem_t *full = &(m_arg->mut_buf->full);
    int *buffer = m_arg->mut_buf->buffer;
    int buffer_idx = 0;
    int buffer_cap = m_arg->mut_buf->capacity;
    int unique_messages = m_arg->unique_messages;
    int msg_number = 1;

    while (1)
    {
        sleep(GET_RANDOM(MONITOR_MINSLEEP, MONITOR_MAXSLEEP));
        if (DISPLAY_MONITOR)
        {
            printf("Monitor Thread: waiting to read the counter\n");
        }

        sem_wait(mutex);
        // Critical Section
        if (DISPLAY_MONITOR)
        {
            printf("Monitor Thread: reading counter of value %d\n", *counter);
        }
        sleep(GET_RANDOM(MONITOR_MINSLEEP, MONITOR_MAXSLEEP));
        stored_counter = *counter;
        *counter = 0;
        if (DISPLAY_MONITOR)
        {
            printf("Monitor Thread: finished reading\n");
        }
        sem_post(mutex);

        while (stored_counter != 0)
        {
            sem_wait(empty);
            if (DISPLAY_MONITOR)
            {
                printf("Monitor Thread: Writing Message %d to buffer at position %d\n", msg_number, buffer_idx);
            }
            sem_wait(mutex_buff);
            *(buffer + buffer_idx) = msg_number;
            if (DISPLAY_MONITOR)
            {
                printf("Monitor Thread: Exited Buffer\n");
            }
            sem_post(mutex_buff);
            sem_post(full);
            if (unique_messages)
            {
                msg_number++;
            }
            stored_counter--;
            if (DISPLAY_MONITOR)
            {
                printf("Monitor Thread: Messages remaining: %d\n", stored_counter);
            }
            buffer_idx++;
            buffer_idx = buffer_idx % buffer_cap;
        }
    }
}

void *mCollector(void *arg)
{
    Mutex_Buffer *mut_buf = (Mutex_Buffer *)arg;

    sem_t *mutex_buff = &(mut_buf->mutex);
    sem_t *empty = &(mut_buf->empty);
    sem_t *full = &(mut_buf->full);
    int *buffer = mut_buf->buffer;
    int buffer_idx = 0;
    int buffer_cap = mut_buf->capacity;

    while (1)
    {
        sem_wait(full);
        if (DISPLAY_COLLECTOR)
        {
            printf("Collector Thread: Waiting to read from buffer\n");
        }
        sem_wait(mutex_buff);
        if (DISPLAY_COLLECTOR)
        {
            printf("Collector Thread: Reading Message %d from buffer position: %d\n", *(buffer + buffer_idx), buffer_idx);
        }
        sleep(GET_RANDOM(COLLECTOR_MINSLEEP, COLLECTOR_MAXSLEEP));
        sem_post(mutex_buff);
        sem_post(empty);
        buffer_idx++;
        buffer_idx = buffer_idx % buffer_cap;
    }
}

int main()
{
    srand((unsigned)time(NULL));

    pthread_t mCounterThreads[NUM_COUNTERS];
    pthread_t mMonitorThread;
    pthread_t mCollectorThread;

    Mutex_Counter *mut_counter = initMutex_Counter();
    Mutex_Buffer *mut_buf = initMutexBuffer(BUFFER_CAPACITY);
    for (int i = 0; i < NUM_COUNTERS; i++)
    {

        pthread_create(&mCounterThreads[i], NULL, mCounter, (void *)initCounterArg(i + 1, mut_counter));
    }

    pthread_create(&mMonitorThread, NULL, mMonitor, initMonitorArg(mut_counter, mut_buf, UNIQUE_MESSAGES));
    pthread_create(&mCollectorThread, NULL, mCollector, mut_buf);

    pthread_join(mCollectorThread, NULL);

    // Destroy Semaphores
    sem_destroy(&(mut_counter->mutex));
}