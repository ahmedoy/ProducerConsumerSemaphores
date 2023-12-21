#include <pthread.h>

#include <stdio.h>

#include <stdlib.h>

#include <time.h>

#include <semaphore.h>

#include <unistd.h>

#define NUM_COUNTERS 8

#define COUNTERS_MAXSLEEP 3

#define COUNTERS_MINSLEEP 0

#define GET_RANDOM(min, max) ((rand() % ((max) - (min) + 1)) + (min))

typedef struct
{
    sem_t mutex;
    int counter;
} mCounter_Mutex_Counter;

typedef struct
{
    mCounter_Mutex_Counter* mut_counter;
    int mCounterID;
} mCounterArg;

mCounter_Mutex_Counter *init_mCounter_Mutex_Counter()
{
    mCounter_Mutex_Counter *mut_counter = (mCounter_Mutex_Counter *)malloc(sizeof(mCounter_Mutex_Counter));
    mut_counter->counter = 0;
    sem_init(&(mut_counter->mutex), 0, 1);
    return mut_counter;
}


mCounterArg *init_mCounterArg(int mCounterID, mCounter_Mutex_Counter* mut_counter)
{
    mCounterArg *mcarg = (mCounterArg *)malloc(sizeof(mCounterArg));
    mcarg->mut_counter = mut_counter;
    mcarg->mCounterID = mCounterID;
    return mcarg;
}

void *mCounter(void *arg)
{
    mCounterArg *mcarg = (mCounterArg *)arg;
    sem_t *mutex = &(mcarg->mut_counter->mutex);
    int counter_id = mcarg->mCounterID;
    int* counter = &(mcarg->mut_counter->counter);

    while (1)
    {
        sleep(GET_RANDOM(COUNTERS_MINSLEEP, COUNTERS_MAXSLEEP));
        printf("Counter %d received a message\ncounter %d waiting to write\n", counter_id, counter_id);        
        sem_wait(mutex);
        // Critical Section
        (*counter)++;
        printf("Counter %d now increasing the counter, counter value = %d\n", counter_id, *counter);
        sem_post(mutex);
    }
}

int main()
{
    srand((unsigned)time(NULL));

    pthread_t mCounterThreads[NUM_COUNTERS];


    mCounter_Mutex_Counter* mC_mut_counter = init_mCounter_Mutex_Counter();
    for (int i = 0; i < NUM_COUNTERS; i++)
    {

        pthread_create(&mCounterThreads[i], NULL, mCounter, (void *)init_mCounterArg(i+1, mC_mut_counter));
    }


    for (int i = 0; i < NUM_COUNTERS; i++)
    {

        pthread_join(mCounterThreads[i], NULL);
    }


    //Destroy Semaphores
    sem_destroy(&(mC_mut_counter->mutex));

}