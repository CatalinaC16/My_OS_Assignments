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
{ // valori pentru functia info, din ce proces vine si nr thread-ului
    int proces;
    int threadID;

} thread_str;

void *thFuncProc2(void *param)
{
    thread_str *th = (thread_str *)param;
    if (th->threadID == 1)
    {
        // thread-ul 1 din procesul 2 trebuie sa inceapa numai dupa ce threadul 2 din procesul 5 se termina
        sem_wait(sem21_52);
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
    }
    else if (th->threadID == 3)
    {
        // thread-ul 3 din procesul 2 semnalizaza cand termina ca threadul 2 din procesul 5 sa inceapa
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
        sem_post(sem23_52);
    }
    else
    {
        // restul threadurilor din procesul2, la care nu trebuie conditii
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
        // threadul 5 din proc2 asteapta ca th3 din proc2 sa termine
        sem_wait(sem23_52);
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
        // semnalizeaza terminarea ca th1 din proc 1 sa inceapa
        sem_post(sem21_52);
    }
    else if (th->threadID == 3)
    { // th4 trebuie sa inceapa inainte de th3 si trebuie sa se incheie dupa ce th3 se termina
        sem_wait(&sem4);
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
        sem_post(&sem3);
    }
    else if (th->threadID == 4)
    { // th4 deci incepe primul apoi semnalizeaza th3, care semnalizeaza t4 atunci cand se termina
        info(BEGIN, th->proces, th->threadID);
        sem_post(&sem4);
        sem_wait(&sem3);
        info(END, th->proces, th->threadID);
    }
    else
    {
        /// celelalte threaduri care sunt fara cerinte
        info(BEGIN, th->proces, th->threadID);
        info(END, th->proces, th->threadID);
    }
    return NULL;
}

void *thFunc(void *param)
{
    thread_str *th = (thread_str *)param;
    sem_wait(&sem6); // permit numai la cate 6 sa  ruleze simultan
    if (th->threadID == 12)
    {
        th12Flag = 1; // atunci cand intra si th12
    }
    info(BEGIN, th->proces, th->threadID);
    if (th12Flag == 1)
    { // inseamna ca th12 a intrat intre cele 6
        if (th->threadID == 12)
        {
            printf("eu intru in asteptare %d %d %d\n", th->threadID, gata, nrTotalThSimultan);
            // pun pe th12 sa astepte ca 5 threaduri sa intre in running
            sem_wait(&sem12);
        }
        else
        {
            nr++;
            printf("eu intru in asteptare %d %d %d \n", th->threadID, nr, nrTotalThSimultan);
            if (nr == 5)
            { // daca 5 threaduri pe langa th12 sunt in runnig, atunci semnalizez th12 sa iasa primul
                sem_post(&sem12);
            }
            sem_wait(&sem5); // pun la asteptare cele 5 threaduri care vin
        }
    }
    info(END, th->proces, th->threadID);
    if (th->threadID == 12)
    {
        // daca th12 a ajuns aici insemna ca a terminat
        gata = 1;
        th12Flag = 0;
    }

    if (gata == 1)
    { // semnalizez celelalte 5 threaduri sa termine si ele
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
    pthread_t thread_ids[NR_TH]; // threaduri proces 5

    thread_str paramThs[NR_THS];
    pthread_t threadIDs[NR_THS]; // threaduri proces 4

    thread_str paramThread25[NR_TH];
    pthread_t thread_id25[NR_TH]; // threaduri proces 2

    // initializare lacate si variabile conditionale
    pthread_mutex_init(&mutexThs, NULL);
    pthread_mutex_init(&mutexTh12, NULL);
    pthread_cond_init(&varThs, NULL);
    pthread_cond_init(&varTh12, NULL);

    // initializare semafoare
    sem_init(&sem3, 0, 0);
    sem_init(&sem4, 0, 0);
    sem_init(&sem12, 0, 0);
    sem_init(&sem6, 0, 6); // initializez cu 6 permisiuni
    sem_init(&sem5, 0, 0);

    // deschidere semafoare cu nume
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
                    pthread_create(&thread_ids[i], NULL, thFuncProc5, &paramThreads[i]); // creez threadurile pentru proc5
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
                waitpid(proc5, NULL, 0); // astept sa termine proc5 , copil al lui p4
                for (int i = 0; i < NR_THS; i++)
                {
                    paramThs[i].threadID = i + 1;
                    paramThs[i].proces = 4;
                    pthread_create(&threadIDs[i], NULL, thFunc, &paramThs[i]); // creez threadurile pentru proc4
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
                pthread_create(&thread_id25[i], NULL, thFuncProc2, &paramThread25[i]); // creez threadurile pentru proc2
            }
            waitpid(proc4, NULL, 0); // astept sa termine proc4, copil al lui P2
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
        waitpid(proc2, NULL, 0); // astept sa termine proc 2, copil al lui p1
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
                waitpid(proc6, NULL, 0); // astept sa termine procesul 6, copil al lui 3
                info(END, 3, 0);         // termin p3
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
                waitpid(proc7, NULL, 0); // astept ca sa termine procesul 7
                info(END, 1, 0);         // termin p1
            }
        }
    }
    // distrug semafoarele, lacatele si variabilele conditionale
    sem_destroy(&sem3);
    sem_destroy(&sem4);
    sem_destroy(&sem5);
    sem_destroy(&sem6);
    sem_destroy(&sem12);
    sem_destroy(sem21_52);
    sem_destroy(sem23_52);
    pthread_mutex_destroy(&mutexThs);
    pthread_cond_destroy(&varThs);

    return 0;
}