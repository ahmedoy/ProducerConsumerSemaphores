#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_COUNTERS 5
#define COUNTERS_MAXSLEEP 5
#define COUNTERS_MINSLEEP 3
#define MONITOR_MAXSLEEP 1
#define MONITOR_MINSLEEP 1
#define GET_RANDOM(min, max) ((rand() % ((max) - (min) + 1)) + (min))

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
    Mutex_Counter *mut_counter;
} mMonitorArg;

Mutex_Counter *init_mCounter_Mutex_Counter()
{
    Mutex_Counter *mut_counter = (Mutex_Counter *)malloc(sizeof(Mutex_Counter));
    mut_counter->counter = 0;
    sem_init(&(mut_counter->mutex), 0, 1);
    return mut_counter;
}

mCounterArg *init_mCounterArg(int mCounterID, Mutex_Counter *mut_counter)
{
    mCounterArg *mcarg = (mCounterArg *)malloc(sizeof(mCounterArg));
    mcarg->mut_counter = mut_counter;
    mcarg->mCounterID = mCounterID;
    return mcarg;
}

mMonitorArg* init_mMonitorArg(Mutex_Counter *mut_counter){
    mMonitorArg* marg = (mMonitorArg*) malloc(sizeof(mMonitorArg));
    marg->mut_counter = mut_counter;
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
        printf("Counter %d received a message\nCounter %d waiting to write\n", counter_id, counter_id);
        sem_wait(mutex);
        // Critical Section
        (*counter)++;
        printf("Counter %d now increasing the counter, counter value = %d\n", counter_id, *counter);
        sleep(GET_RANDOM(COUNTERS_MINSLEEP, COUNTERS_MAXSLEEP));
        printf("Counter %d Leaving the counter\n", counter_id);
        sem_post(mutex);
    }
}

void *mMonitor(void *arg)
{
    mMonitorArg *m_arg = (mMonitorArg *)arg;
    sem_t *mutex = &(m_arg->mut_counter->mutex);
    int *counter = &(m_arg->mut_counter->counter);
    int stored_counter = 0;

    while(1){
        sleep(GET_RANDOM(MONITOR_MINSLEEP, MONITOR_MAXSLEEP));
        printf("Monitor waiting to read the counter\n");
        sem_wait(mutex);
        //Critical Section
        printf("Monitor reading counter of value %d\n", *counter);
        sleep(GET_RANDOM(MONITOR_MINSLEEP, MONITOR_MAXSLEEP));
        stored_counter = *counter;
        *counter = 0;
        printf("Monitor finished reading\n");
        sem_post(mutex);    

        while(stored_counter != 0){
            sleep(GET_RANDOM(MONITOR_MINSLEEP, MONITOR_MAXSLEEP));
            printf("Writing to buffer\n");
            stored_counter--;
        }
    }


}

int main()
{
    srand((unsigned)time(NULL));

    pthread_t mCounterThreads[NUM_COUNTERS];
    pthread_t mMonitorThread;

    Mutex_Counter *mut_counter = init_mCounter_Mutex_Counter();
    for (int i = 0; i < NUM_COUNTERS; i++)
    {

        pthread_create(&mCounterThreads[i], NULL, mCounter, (void *)init_mCounterArg(i + 1, mut_counter));
    }

    pthread_create(&mMonitorThread, NULL, mMonitor, init_mMonitorArg(mut_counter));


    for (int i = 0; i < NUM_COUNTERS; i++)
    {

        pthread_join(mCounterThreads[i], NULL);
    }

    


    // Destroy Semaphores
    sem_destroy(&(mut_counter->mutex));
}