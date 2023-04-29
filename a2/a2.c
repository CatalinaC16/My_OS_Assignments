#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "a2_helper.h"

#define NR_TH 4
sem_t sem3, sem4;
typedef struct thread_str
{ /// valori pentru functia info
    int proces;
    int threadID;

} thread_str;

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
int main(int argc, char **argv)
{
    thread_str paramThreads[NR_TH];
    pthread_t thread_ids[NR_TH];
    sem_init(&sem3, 0, 0);
    sem_init(&sem4, 0, 0);
    init();
    pid_t proc2 = -1, proc3 = -1, proc4 = -1, proc5 = -1, proc6 = -1, proc7 = -1;
    info(BEGIN, 1, 0); // incep p1 - procesul principal
    proc2 = fork();
    if (proc2 == 0)
    {                      // aici in p2
        info(BEGIN, 2, 0); // incep p2
        proc4 = fork();
        if (proc4 == 0)
        {
            info(BEGIN, 4, 0); // incep p4
            proc5 = fork();
            if (proc5 == 0)
            {
                // aici in p5
                info(BEGIN, 5, 0); // incep p5
                for (int i = NR_TH - 1; i >= 0; i--)
                {
                    paramThreads[i].threadID = i + 1;
                    paramThreads[i].proces = 5;
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

    return 0;
}
