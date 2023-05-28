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
    char buffer_comanda[251]; // comanda nu poate sa aiba mai mult de 251 octeti
    // mesajele pentru comunicare intre cele 2 pipe-uri
    char mesConnect[] = "CONNECT!";
    char mesVariant[] = "VARIANT!";
    char mesValue[] = "VALUE!";
    char mesExit[] = "EXIT!";
    char mesShm[] = "CREATE_SHM!";
    char success[] = "SUCCESS!";
    char error[] = "ERROR!";
    char mesWShm[] = "WRITE_TO_SHM!";
    char mesMap[] = "MAP_FILE!";
    char mesRead[] = "READ_FROM_FILE_OFFSET!";
    char mesReadSF[] = "READ_FROM_FILE_SECTION!";
    char mesReadLogic[] = "READ_FROM_LOGICAL_SPACE_OFFSET!";

    int fd_zona = -1;
    void *zona = NULL;

    char nume_fisierPrimit[251];
    int fd_fisierPrimit = -1;
    void *mapare = NULL;
    size_t dimensiuneFisier;

    if (mkfifo(PIPE_SCRIERE, 0600) != 0) // creez pipe-ul de scriere/raspuns
    {
        printf("ERROR\ncannot create the response pipe\n");
    }
    fd_citire = open(PIPE_CITIRE, O_RDONLY); // deschid pipe-ul de citire/cerere
    if (fd_citire == -1)
    {
        printf("ERROR\ncannot open the request pipe\n");
    }

    fd_scriere = open(PIPE_SCRIERE, O_WRONLY); // deschid pipe-ul de scriere/raspuns

    dimConnect = strlen(mesConnect);
    if (write(fd_scriere, mesConnect, dimConnect * sizeof(char)) != -1) // a deschis/creat cu succes
    {
        printf("SUCCESS\n");
    }
    while (1)
    {
        memset(buffer_comanda, 0, sizeof(buffer_comanda)); // la fiecare iteratie buffer-ul trebuie sa fie gol
        int octeti = 0;
        char c;
        // citesc text comanda pana ajunge la "!"
        while (octeti < 251 && read(fd_citire, &c, 1) == 1)
        {
            buffer_comanda[octeti++] = c;
            if (c == '!')
                break;
        }
        buffer_comanda[octeti] = '\0'; // cand s-a citit comanda se adauga terminatorul
        if (strcmp(buffer_comanda, mesVariant) == 0)
        { // primesc comanda de VARIANT, cu pipe-ul de raspuns trimit VARIANT 12095 VALUE
            write(fd_scriere, mesVariant, strlen(mesVariant) * sizeof(char));
            write(fd_scriere, &var, sizeof(var));
            write(fd_scriere, mesValue, strlen(mesValue) * sizeof(char));
        }
        else if (strcmp(buffer_comanda, mesExit) == 0)
        { // primesc comanda de EXIT si inchid cele 2 pipe-uri si sterg pipe-ul de raspuns
            close(fd_citire);
            close(fd_scriere);
            unlink(PIPE_SCRIERE);
            return 0;
        }
        else if (strcmp(buffer_comanda, mesShm) == 0)
        { // primesc comanda CREATE_SHM si numarul de octeti ai zonei
            read(fd_citire, &nrOcteti, sizeof(unsigned int));

            fd_zona = shm_open(MEM_PARTAJATA, O_CREAT | O_RDWR, 0664); // creez zona
            write(fd_scriere, mesShm, strlen(mesShm) * sizeof(char));
            if (fd_zona < 0)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }

            if (ftruncate(fd_zona, nrOcteti * sizeof(char)) == -1) // redimensionez zona la 5020043
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }

            zona = mmap(0, nrOcteti, PROT_READ | PROT_WRITE, MAP_SHARED, fd_zona, 0); // mapez zona
            if (zona == MAP_FAILED)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                // munmap(zona, nrOcteti);
                // close(fd_zona);
                return 0;
            }
            else
            {
                // trimit mesaj de SUCCESS in caz favorabil, altfel ERROR
                write(fd_scriere, success, strlen(success) * sizeof(char));
            }
        }
        else if (strcmp(buffer_comanda, mesWShm) == 0)
        { // primesc comanda WRITE_TO_SHM, un offset si o valoare
            unsigned int offset, value;
            read(fd_citire, &offset, sizeof(unsigned int));
            read(fd_citire, &value, sizeof(unsigned int));
            write(fd_scriere, mesWShm, strlen(mesWShm) * sizeof(char));
            if (offset < 0 || offset >= nrOcteti)
            {
                // verific daca offset-ul este in regiunea de memorie partajata, intre 0 si 5020043
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }
            if ((offset + sizeof(unsigned int)) > nrOcteti)
            {
                // sau daca nu are loc in memorie de la offset ul respectiv
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }
            memcpy(zona + offset, &value, sizeof(unsigned int)); // pun in memorie la adr offset+inceputul zonei, valoarea primita si trimit SUCCESS
            write(fd_scriere, success, strlen(success) * sizeof(char));
        }
        else if (strcmp(buffer_comanda, mesMap) == 0)
        {
            // primesc comanda MAP_FILE si un nume de fisier
            int octetiFisier = 0;
            char ch;

            while (octetiFisier < 251 && read(fd_citire, &ch, 1) == 1)
            { // exact ca la comanda fiind field de tip string se termina cu "!"
                if (ch == '!')
                    break;
                nume_fisierPrimit[octetiFisier++] = ch;
            }
            nume_fisierPrimit[octetiFisier] = '\0';

            write(fd_scriere, mesMap, strlen(mesMap) * sizeof(char));
            fd_fisierPrimit = open(nume_fisierPrimit, O_RDONLY); // deschid fisierul
            if (fd_fisierPrimit == -1)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                return 0;
            }
            dimensiuneFisier = lseek(fd_fisierPrimit, 0, SEEK_END); // iau dimensiune fisier
            lseek(fd_fisierPrimit, 0, SEEK_SET);
            mapare = mmap(0, dimensiuneFisier, PROT_READ, MAP_SHARED, fd_fisierPrimit, 0); // il mapez in memorie
            if (mapare == MAP_FAILED)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
                close(fd_fisierPrimit);
                return 0;
            }
            write(fd_scriere, success, strlen(success) * sizeof(char));
        }
        else if (strcmp(buffer_comanda, mesRead) == 0)
        { // primesc comanda READ_FROM_FILE_OFFSET, un offset si un numar de octeti
            unsigned int offsetFis, octetiFisier;
            read(fd_citire, &offsetFis, sizeof(unsigned int));
            read(fd_citire, &octetiFisier, sizeof(unsigned int));
            write(fd_scriere, mesRead, strlen(mesRead) * sizeof(char));
            if ((offsetFis + octetiFisier) >= dimensiuneFisier)
            {
                // daca offset adunat cu octetii este inafara dimensiunii fisierului
                write(fd_scriere, error, strlen(error) * sizeof(char));
            }
            else
            {
                // octetii pe care ii citesc ii pun la inceputul zonei de memorie si trimit mesaj de success
                memcpy(zona, (char *)mapare + offsetFis, octetiFisier);
                write(fd_scriere, success, strlen(success) * sizeof(char));
            }
        }
        else if (strcmp(buffer_comanda, mesReadSF) == 0)
        { // primesc mesaj READ_FROM_FILE_SECTION, o sectiune si un offset din sectiune si un nr de octeti
            unsigned int section_no = 0, offset_section = 0, octetiSectiune = 0;
            read(fd_citire, &section_no, sizeof(unsigned int));
            read(fd_citire, &offset_section, sizeof(unsigned int));
            read(fd_citire, &octetiSectiune, sizeof(unsigned int));
            write(fd_scriere, mesReadSF, strlen(mesReadSF) * sizeof(char));
            // am body si header, deci merg la final ca sa iau nr de sectiuni si dimensiune header
            unsigned int header_size = 0, no_sect = 0;
            unsigned int dim = dimensiuneFisier;
            memcpy(&header_size, (char *)mapare + dim - (unsigned int)3, 2);           // dimensiune header
            memcpy(&no_sect, (char *)mapare + dim - header_size + (unsigned int)2, 1); // nr de sectiuni din fisier
            if (section_no < 1 || section_no > no_sect)
            {
                // daca nu e o sectiune corecta
                write(fd_scriere, error, strlen(error) * sizeof(char));
            }
            else
            {
                unsigned int sect_offset = 0;
                unsigned int sect = (section_no - 1) * 16;                                                              // ma duc la inceputul sectiunii, trebuie sa le parcurg pe celelalte
                memcpy(&sect_offset, (char *)mapare + dim - header_size + (unsigned int)3 + sect + (unsigned int)8, 4); // calculez offset-ul de unde sa iau octetii
                memcpy(zona, (char *)mapare + sect_offset + offset_section, octetiSectiune);                            // copiez octetii la inceputul zonei partajate
                write(fd_scriere, success, strlen(success) * sizeof(char));
            }
        }
        else if (strcmp(buffer_comanda, mesReadLogic) == 0)
        {
            // primesc comanda READ_FROM_LOGICAL_SPACE_OFFSET, un offset logic si un numar de octeti
            int ok = 0;
            unsigned int logical_offset, octetiLog;
            read(fd_citire, &logical_offset, sizeof(unsigned int));
            read(fd_citire, &octetiLog, sizeof(unsigned int));
            write(fd_scriere, mesReadLogic, strlen(mesReadLogic) * sizeof(char));
            unsigned int header_size = 0, no_sect = 0;
            unsigned int dim = dimensiuneFisier;
            // le iau de la sfarsitul SF
            memcpy(&header_size, (char *)mapare + dim - (unsigned int)3, 2);           // dimensiune header
            memcpy(&no_sect, (char *)mapare + dim - header_size + (unsigned int)2, 1); // nr de sectiuni din fisier
            unsigned int sect_size = 0, logic = 0, sect_offset = 0, offLog = 0, prev = 0;
            for (unsigned int i = 0; i < no_sect; i++)
            { // parcurg fiecare sectiune
                prev = logic;
                memcpy(&sect_size, (char *)mapare + dim - header_size + 15 + i * 16, 4);

                if (sect_size <= 2048)
                {
                    // daca sunt mai putin de 2048 octeti inseamna ca maresc numai cu 2048
                    logic += 2048;
                }
                else
                {
                    // daca sunt mai mult de 2048 octeti stabilesc cate de 2048 sa las ptr fiecare
                    int aux = sect_size / 2048;
                    int rest = sect_size % 2048;
                    if (rest != 0)
                    {
                        // numarul nu e exact, inseamna ca mai trece intr-o sectiune de cate 2048
                        logic += (aux + 1) * 2048;
                    }
                    else
                    {
                        // numar exact inseamna ca sunt exact aux*2048
                        logic += aux * 2048;
                    }
                }
                if (logical_offset <= logic && logical_offset > prev)
                {
                    // daca ne aflam cu offset-ul primit intre offset ul logic celui de dinainte si al celui curent, stabilesc sectiunea
                    memcpy(&sect_offset, (char *)mapare + dim - header_size + (unsigned int)3 + i * 16 + (unsigned int)8, 4);
                    offLog = logical_offset - prev;
                    ok = 1;
                    break;
                }
            }
            if (ok == 0 || sect_size < octetiLog)
            {
                write(fd_scriere, error, strlen(error) * sizeof(char));
            }
            memcpy(zona, (char *)mapare + sect_offset + offLog, octetiLog); // copiez octetii din sectiunea gasita+offset ul la care se afla
            write(fd_scriere, success, strlen(success) * sizeof(char));
        }
    }
    return 0;
}