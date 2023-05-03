#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "a2_helper.h"
#define NR_TH 4
#define NR_THS 37

sem_t sem3, sem4, sem12;
sem_t *sem23_52, *sem21_52;
pthread_mutex_t mutexThs;
pthread_cond_t varThs;

int nrTotalThSimultan = 0;
int th12Flag = 0, flg23 = 0;

typedef struct thread_str
{ /// valori pentru functia info
    int proces;
    int threadID;

} thread_str;
void *th23(void *param)
{
    thread_str *th = (thread_str *)param;
    info(BEGIN, th->proces, th->threadID);
    info(END, th->proces, th->threadID);
    sem_post(sem23_52);
    return NULL;
}
void *th21(void *param)
{
    // sem_wait(sem21_52);
    thread_str *th = (thread_str *)param;
    info(BEGIN, th->proces, th->threadID);
    info(END, th->proces, th->threadID);
    return NULL;
}
void *th52(void *param)
{
    sem_wait(sem23_52);
    thread_str *th = (thread_str *)param;
    info(BEGIN, th->proces, th->threadID);
    info(END, th->proces, th->threadID);
    sem_post(sem21_52);
    return NULL;
}
void *thFunc_cr(void *param)
{
    thread_str *th = (thread_str *)param;
    if (th->threadID == 3)
    {
        sem_wait(&sem4);
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
        sem_post(&sem3);
    }
    else if (th->threadID == 4)
    {
        info(BEGIN, th->proces, th->threadID);
        sem_post(&sem4);
        sem_wait(&sem3);
        info(END, th->proces, th->threadID);
    }
    else
    {
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
    }

    return NULL;
}

void *thFuncProc2(void *param)
{
    thread_str *th = (thread_str *)param;

    info(BEGIN, th->proces, th->threadID);
    info(END, th->proces, th->threadID);

    return NULL;
}
void *thFunc(void *param)
{
    thread_str *th = (thread_str *)param;
    pthread_mutex_lock(&mutexThs);
    while (nrTotalThSimultan >= 6)
    {
        pthread_cond_wait(&varThs, &mutexThs);
    }
    nrTotalThSimultan += 1;
    if (nrTotalThSimultan == 6)
    {
        sem_post(&sem12);
    }
    pthread_mutex_unlock(&mutexThs);

    info(BEGIN, th->proces, th->threadID);
    if (th->threadID == 12)
    {
        sem_wait(&sem12);
    }
    info(END, th->proces, th->threadID);

    pthread_mutex_lock(&mutexThs);
    nrTotalThSimultan -= 1;
    pthread_cond_signal(&varThs);
    pthread_mutex_unlock(&mutexThs);
    return NULL;
}
int main(int argc, char **argv)
{
    thread_str paramThreads[NR_TH];
    pthread_t thread_ids[NR_TH];

    thread_str paramThs[NR_THS];
    pthread_t threadIDs[NR_THS];

    thread_str paramThread25[NR_TH];
    pthread_t thread_id25[NR_TH];

    pthread_mutex_init(&mutexThs, NULL);
    pthread_cond_init(&varThs, NULL);

    sem_init(&sem3, 0, 0);
    sem_init(&sem4, 0, 0);
    sem_init(&sem12, 0, 0);
    sem23_52 = sem_open("sem1", O_CREAT | O_EXCL, 0644, 0);
    sem21_52 = sem_open("sem2", O_CREAT | O_EXCL, 0644, 0);
    // sem_init(&sem21_52, 1, 0);
    // sem_init(&sem23_52, 1, 0);
    init();
    pid_t proc2 = -1, proc3 = -1, proc4 = -1, proc5 = -1, proc6 = -1, proc7 = -1;
    info(BEGIN, 1, 0); // incep p1 - procesul principal
    proc2 = fork();
    if (proc2 == 0)
    {
        // aici in p2
        info(BEGIN, 2, 0); // incep p2
        sem23_52 = sem_open("sem1", O_RDWR);
        sem21_52 = sem_open("sem2", O_RDWR);
        for (int i = 0; i < NR_TH; i++)
        {
            paramThread25[i].threadID = i + 1;
            paramThread25[i].proces = 2;
            if (paramThread25[i].threadID == 1)
            {
                pthread_create(&thread_id25[i], NULL, th21, &paramThread25[i]);
            }
            else if (paramThread25[i].threadID == 3)
            {
                pthread_create(&thread_id25[i], NULL, th23, &paramThread25[i]);
            }
            else
                pthread_create(&thread_id25[i], NULL, thFuncProc2, &paramThread25[i]);
        }
        for (int i = 0; i < NR_TH; i++)
        {
            pthread_join(thread_id25[i], NULL);
        }
        proc4 = fork();
        if (proc4 == 0)
        {
            info(BEGIN, 4, 0); // incep p4
            proc5 = fork();
            if (proc5 == 0)
            {
                // aici in p5
                info(BEGIN, 5, 0); // incep p5
                sem23_52 = sem_open("sem1", O_RDWR);
                sem21_52 = sem_open("sem2", O_RDWR);
                for (int i = NR_TH - 1; i >= 0; i--)
                {
                    paramThreads[i].threadID = i + 1;
                    paramThreads[i].proces = 5;
                    if (paramThreads[i].threadID == 2)
                    {
                        pthread_create(&thread_ids[i], NULL, th52, &paramThreads[i]);
                    }
                    else
                        pthread_create(&thread_ids[i], NULL, thFunc_cr, &paramThreads[i]);
                }

                for (int i = 0; i < NR_TH; i++)
                {
                    pthread_join(thread_ids[i], NULL);
                }

                info(END, 5, 0); // termin p5
            }
            else
            {
                // aici in p4
                waitpid(proc5, NULL, 0);
                for (int i = 0; i < NR_THS; i++)
                {
                    paramThs[i].threadID = i + 1;
                    paramThs[i].proces = 4;
                    pthread_create(&threadIDs[i], NULL, thFunc, &paramThs[i]);
                }
                for (int i = 0; i < NR_THS; i++)
                {
                    pthread_join(threadIDs[i], NULL);
                }

                info(END, 4, 0); // termin p4
            }
        }
        else
        { // aici in p2
            waitpid(proc4, NULL, 0);
            info(END, 2, 0); // termin p2
        }
    }
    else
    {
        // in p1
        waitpid(proc2, NULL, 0);
        proc3 = fork();
        if (proc3 == 0)
        {                      // in p3
            info(BEGIN, 3, 0); // incep p3
            proc6 = fork();
            if (proc6 == 0)
            {
                // aici in p6
                info(BEGIN, 6, 0); // incep p6
                info(END, 6, 0);   // termin p6
            }
            else
            {
                // aici in p3
                waitpid(proc6, NULL, 0);
                info(END, 3, 0); // termin p3
            }
        }
        else
        {
            // aici in p1
            waitpid(proc3, NULL, 0);
            proc7 = fork();
            if (proc7 == 0)
            {
                // aici in p7
                info(BEGIN, 7, 0); // incep p7
                info(END, 7, 0);   // termin p7
            }
            else
            {
                // aici in p1
                waitpid(proc7, NULL, 0);
                info(END, 1, 0); // termin p1
            }
        }
    }
    sem_destroy(&sem3);
    sem_destroy(&sem4);
    sem_destroy(&sem12);
    sem_destroy(sem21_52);
    sem_destroy(sem23_52);
    pthread_mutex_destroy(&mutexThs);
    pthread_cond_destroy(&varThs);

    return 0;
}