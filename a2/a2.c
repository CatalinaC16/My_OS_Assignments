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

sem_t sem3, sem4, sem12, sem6, sem5;
sem_t *sem23_52, *sem21_52;
pthread_mutex_t mutexThs, mutexTh12;
pthread_cond_t varThs, varTh12;

int nrTotalThSimultan = 0, gata = 0, nr = 0;
int th12Flag = 0, flg23 = 0;

typedef struct thread_str
{ /// valori pentru functia info
    int proces;
    int threadID;

} thread_str;

void *thFuncProc2(void *param)
{
    thread_str *th = (thread_str *)param;
    if (th->threadID == 1)
    {
        sem_wait(sem21_52);
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
    }
    else if (th->threadID == 3)
    {
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
        sem_post(sem23_52);
    }
    else
    {
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
    }
    return NULL;
}
void *thFuncProc5(void *param)
{
    thread_str *th = (thread_str *)param;
    if (th->threadID == 2)
    {
        sem_wait(sem23_52);
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
        sem_post(sem21_52);
    }
    else if (th->threadID == 3)
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

void *thFunc(void *param)
{
    thread_str *th = (thread_str *)param;
    sem_wait(&sem6);
    if (th->threadID == 12)
    {
        th12Flag = 1;
    }
    info(BEGIN, th->proces, th->threadID);
    if (th12Flag == 1)
    {
        if (th->threadID == 12)
        {
            printf("eu intru in asteptare %d %d %d\n", th->threadID, gata, nrTotalThSimultan);
            sem_wait(&sem12);
        }
        else
        {
            nr++;
            printf("eu intru in asteptare %d %d %d \n", th->threadID, nr, nrTotalThSimultan);
            if (nr == 5)
            {
                sem_post(&sem12);
            }
            sem_wait(&sem5);
        }
    }
    info(END, th->proces, th->threadID);
    if (th->threadID == 12)
    {
        gata = 1;
        th12Flag = 0;
    }

    if (gata == 1)
    {
        gata = 0;
        sem_post(&sem5);
        sem_post(&sem5);
        sem_post(&sem5);
        sem_post(&sem5);
        sem_post(&sem5);
    }
    sem_post(&sem6);

    return NULL;
}
int main(int argc, char **argv)
{
    init();
    thread_str paramThreads[NR_TH];
    pthread_t thread_ids[NR_TH];

    thread_str paramThs[NR_THS];
    pthread_t threadIDs[NR_THS];

    thread_str paramThread25[NR_TH];
    pthread_t thread_id25[NR_TH];

    pthread_mutex_init(&mutexThs, NULL);
    pthread_mutex_init(&mutexTh12, NULL);
    pthread_cond_init(&varThs, NULL);
    pthread_cond_init(&varTh12, NULL);

    sem_init(&sem3, 0, 0);
    sem_init(&sem4, 0, 0);
    sem_init(&sem12, 0, 0);
    sem_init(&sem6, 0, 6);
    sem_init(&sem5, 0, 0);

    sem_unlink("sem1");
    sem_unlink("sem2");
    sem23_52 = sem_open("sem1", O_CREAT, 0644, 0);
    sem21_52 = sem_open("sem2", O_CREAT, 0644, 0);
    pid_t proc2 = -1, proc3 = -1, proc4 = -1, proc5 = -1, proc6 = -1, proc7 = -1;
    info(BEGIN, 1, 0); // incep p1 - procesul principal
    proc2 = fork();
    if (proc2 == 0)
    {
        // aici in p2
        info(BEGIN, 2, 0); // incep p2
        proc4 = fork();    // procesul 4
        if (proc4 == 0)
        {
            info(BEGIN, 4, 0); // incep p4
            proc5 = fork();
            if (proc5 == 0)
            {
                // aici in p5
                info(BEGIN, 5, 0); // incep p5
                for (int i = 0; i < NR_TH; i++)
                {
                    paramThreads[i].threadID = i + 1;
                    paramThreads[i].proces = 5;
                    pthread_create(&thread_ids[i], NULL, thFuncProc5, &paramThreads[i]);
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
            for (int i = 0; i < NR_TH; i++)
            {
                paramThread25[i].threadID = i + 1;
                paramThread25[i].proces = 2;
                pthread_create(&thread_id25[i], NULL, thFuncProc2, &paramThread25[i]);
            }
            waitpid(proc4, NULL, 0);
            for (int i = 0; i < NR_TH; i++)
            {
                pthread_join(thread_id25[i], NULL);
            }

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