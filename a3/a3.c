#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define PIPE_SCRIERE "RESP_PIPE_12095"
#define PIPE_CITIRE "REQ_PIPE_12095"
#define MEM_PARTAJATA "/6A9hlfK"

int main(void)
{
    int fd_citire = -1, fd_scriere = -1;
    size_t dimConnect;
    unsigned int var = 12095;
    unsigned int nrOcteti;
    char buffer_comanda[251];
    char mesConnect[] = "CONNECT!";
    char mesVariant[] = "VARIANT!";
    char mesValue[] = "VALUE!";
    char mesExit[] = "EXIT!";
    char mesShm[] = "CREATE_SHM!";
    char success[] = "SUCCESS!";
    char error[] = "ERROR!";
    char mesWShm[] = "WRITE_TO_SHM!";
    char mesMap[] = "MAP_FILE!";

    int fd_zona;
    void *zona = NULL;

    char nume_fisierPrimit[1024];
    int fd_fisierPrimit = -1;
    void *mapare = NULL;

    if (mkfifo(PIPE_SCRIERE, 0600) != 0)
    {
        printf("ERROR\ncannot create the response pipe\n");
    }
    fd_citire = open(PIPE_CITIRE, O_RDONLY);
    if (fd_citire == -1)
    {
        printf("ERROR\ncannot open the request pipe\n");
    }

    fd_scriere = open(PIPE_SCRIERE, O_WRONLY);

    dimConnect = strlen(mesConnect);
    if (write(fd_scriere, mesConnect, dimConnect) != -1)
    {
        printf("SUCCESS\n");
    }
    while (1)
    {
        memset(buffer_comanda, 0, sizeof(buffer_comanda));
        int octeti = 0;
        char c;

        while (octeti < 251 && read(fd_citire, &c, 1) == 1)
        {
            buffer_comanda[octeti++] = c;
            if (c == '!')
                break;
        }
        buffer_comanda[octeti] = '\0';
        if (strcmp(buffer_comanda, mesVariant) == 0)
        {
            write(fd_scriere, mesVariant, strlen(mesVariant) * sizeof(char));
            write(fd_scriere, &var, sizeof(var));
            write(fd_scriere, mesValue, strlen(mesValue) * sizeof(char));
        }
        else if (strcmp(buffer_comanda, mesExit) == 0)
        {
            close(fd_citire);
            close(fd_scriere);
            unlink(PIPE_SCRIERE);
            return 0;
        }
        else if (strcmp(buffer_comanda, mesShm) == 0)
        {
            read(fd_citire, &nrOcteti, sizeof(unsigned int));

            fd_zona = shm_open(MEM_PARTAJATA, O_CREAT | O_RDWR, 0664);
            write(fd_scriere, mesShm, strlen(mesShm) * sizeof(char));
            if (fd_zona < 0)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }

            if (ftruncate(fd_zona, nrOcteti * sizeof(char)) == -1)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }

            zona = mmap(0, nrOcteti, PROT_READ | PROT_WRITE, MAP_SHARED, fd_zona, 0);
            if (zona == MAP_FAILED)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                munmap(zona, nrOcteti);
                close(fd_zona);
                return 0;
            }
            else
            {
                write(fd_scriere, success, strlen(success) * sizeof(char));
            }
        }
        else if (strcmp(buffer_comanda, mesWShm) == 0)
        {
            unsigned int offset, value;
            read(fd_citire, &offset, sizeof(unsigned int));
            read(fd_citire, &value, sizeof(unsigned int));
            write(fd_scriere, mesWShm, strlen(mesWShm) * sizeof(char));
            if (offset < 0 || offset >= nrOcteti)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }
            if ((offset + sizeof(unsigned int)) > nrOcteti)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }
            memcpy(zona + offset, &value, sizeof(unsigned int));
            write(fd_scriere, success, strlen(success) * sizeof(char));
        }
        else if (strcmp(buffer_comanda, mesMap) == 0)
        {
            mapare = NULL;
            fd_fisierPrimit = -1;
            int octetiFisier = 0;
            char ch;

            while (octetiFisier < 251 && read(fd_citire, &ch, 1) == 1)
            {
                 if (ch == '!')
                    break;
                nume_fisierPrimit[octetiFisier++] = ch;
               
            }
            
            write(fd_scriere, mesMap, strlen(mesMap) * sizeof(char));
            fd_fisierPrimit = open(nume_fisierPrimit, O_RDONLY);
            if (fd_fisierPrimit == -1)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }
            size_t dimensiuneFisier = lseek(fd_fisierPrimit, 0, SEEK_END);
            lseek(fd_fisierPrimit, 0, SEEK_SET);
            mapare = mmap(NULL, dimensiuneFisier, PROT_READ, MAP_PRIVATE, fd_fisierPrimit, 0);
            if (mapare == MAP_FAILED)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                close(fd_fisierPrimit);
                return 0;
            }
            write(fd_scriere, success, strlen(success) * sizeof(char));
        }
    }
    return 0;
}