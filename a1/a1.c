#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

// LISTARE RECURSIVA atat cu cat si fara filtru in functie de flag-urile de filtre
/// atunci cand sunt SETATE pe 1

void listRecFilter(const char *path, char *filStart, int filterStr, int filterPerm)
{
    struct stat metaDate;                                /// este nevoie de o variabila in care sa pun metadatele fisierului
    DIR *director = opendir(path);                       /// deschid directorul curent, nu e nevoie sa il mai testez ca l-am testat in main sa nu fie NULL
    struct dirent *director_content = readdir(director); /// citesc primul element din folder ca sa intru in while
    char exactPath[1000];                                /// declar o dimensiune maxima a path-ului

    /// parcurg directorul cat timp exista elemente
    while (director_content != NULL)
    {
        /// nu mergem ooe curent/parinte pentru ca se face loop infinit
        if (strcmp(director_content->d_name, ".") != 0 && strcmp(director_content->d_name, "..") != 0)
        {
            // stabilesc calea completa
            snprintf(exactPath, 1000, "%s/%s", path, director_content->d_name);
            /// citesc metadatele
            if (lstat(exactPath, &metaDate) == 0)
            {
                if (filterStr == 1)
                {
                    /// numele trebuie sa inceapa cu secventa de caractere primita in argument
                    if (strncmp(director_content->d_name, filStart, strlen(filStart)) == 0)
                    {
                        printf("%s\n", exactPath);
                    }
                }
                else if (filterPerm == 1)
                {
                    /// elementul trebuie sa aiba permisiunea de executie pentru owner, extrag bitul de permisiune de executie pentru owner
                    if (metaDate.st_mode & S_IXUSR)
                    {
                        printf("%s\n", exactPath);
                    }
                }
                if (filterStr == 0 && filterPerm == 0)
                {
                    /// parcurgere recursiva fara filtre
                    printf("%s\n", exactPath);
                }

                if (S_ISDIR(metaDate.st_mode))
                {
                    /// in cazul in care trebuie sa mergem in adangime, avem folder
                    listRecFilter(exactPath, filStart, filterStr, filterPerm);
                }
            }
        }
        /// citesc urmatorul element din directorul curent
        director_content = readdir(director);
    }
    /// inchid directorul curent
    closedir(director);
}

// LISTARE NORMALA cu si fara filtre, in functie de flag-urile de filtre

void listFilter(const char *path, char *filStart, int filterStr, int filterPerm)
{
    struct stat metaDate; /// metadatele fisierului
    char exactPath[1000];
    /// deschid directorul curent
    DIR *director = opendir(path);
    if (director == NULL)
    {
        printf("ERROR\ninvalid directory path");
        return;
    }
    printf("SUCCESS\n");
    struct dirent *director_content = readdir(director); /// primul element din folder-ul deschis
    while (director_content != NULL)
    {
        /// parcurg folder-ul pana nu mai are elemente si nu merg pe parinte sau curent
        if (strcmp(director_content->d_name, ".") != 0 && strcmp(director_content->d_name, "..") != 0)
        {
            snprintf(exactPath, 1000, "%s/%s", path, director_content->d_name);
            if (filterStr == 1)
            {
                /// numele trebuie sa inceapa cu secventa de caractere primita in argument
                if (strncmp(director_content->d_name, filStart, strlen(filStart)) == 0)
                {
                    printf("%s\n", exactPath);
                }
            }
            else if (filterPerm == 1)
            {
                /// elementul trebuie sa aiba permisiunea de executie pentru owner, extrag bitul de permisiune de executie pentru owner
                if (lstat(exactPath, &metaDate) == 0)
                {
                    if (metaDate.st_mode & S_IXUSR)
                    {
                        printf("%s\n", exactPath);
                    }
                }
            }
            if (filterPerm == 0 && filterStr == 0)
            {
                /// listare normala fara filtre
                printf("%s\n", exactPath);
            }
        }

        /// citesc urmatorul element din folder
        director_content = readdir(director);
    }
    /// inchid directorul la sfarsit
    closedir(director);
}

// PARSE a fisierului SF
int parseSF(const char *path, int extr)
{
    int fis = -1;
    fis = open(path, O_RDONLY);
    if (fis == -1)
    {
        /// fisierul nu a fost deschis
        return -1;
    }

    /// 3 octeti in spate de la sfarsitul fisierului ca sa citesc dimensiunea header-ului, care e pe 2 octeti
    lseek(fis, -3, SEEK_END);
    unsigned short header_dimensionB;
    read(fis, &header_dimensionB, 2);

    /// urmatoarea valoare e a magic-ului, pe 1 octet
    char magic_val;
    read(fis, &magic_val, 1);
    if (magic_val != 'M')
    {
        /// nu e SF, pentru ca magic nu este M
        return -2;
    }

    lseek(fis, -header_dimensionB, SEEK_END); /// ma duc la inceputul header-ului si citesc version,nr de sectiuni si informatii sectiuni

    unsigned short version = 0;
    read(fis, &version, 2);
    if (version < 83 || version > 107)
    {
        /// nu e SF, pentru ca version NU este in intervalul [83,107]
        return -3;
    }

    unsigned char no_sect;
    read(fis, &no_sect, 1);
    if (no_sect < 2 || no_sect > 20)
    {
        /// nu e SF, pentru ca nr de sectiuni NU este in intervalul [2,20]
        return -4;
    }

    char sect_name[7];
    unsigned short sect_type;
    int sect_size, sect_offset;
    for (int i = 0; i < no_sect; i++)
    {
        read(fis, sect_name, 6); /// 6 octeti pentru nume sectiune
        sect_name[6] = '\0';
        read(fis, &sect_type, 2);   /// 2 octeti pentru tipul sectiunii
        read(fis, &sect_offset, 4); /// 4 octeti pentru offset-ul sectiunii
        read(fis, &sect_size, 4);   /// 4 octeti pentru dimensiunea sectiunii
        if (sect_type != 89 && sect_type != 47 && sect_type != 86)
        {
            /// nu e SF, pentru ca valorile valide sunt 89  sau 47 sau 86
            return -5;
        }
    }
    /// acum printez informatiile despre header si sectiuni
    lseek(fis, -header_dimensionB + 3, SEEK_END);
    if (extr == 0)
    {
        /// am declarat 2 siruri, la care am adaugat versiunea si nr de sectiuni ca sa printeze corect valorile
        char versionString[10];
        memset(versionString, 0, 10);
        char noSectString[10];
        memset(noSectString, 0, 10);

        sprintf(versionString, "%hu", version);
        sprintf(noSectString, "%d", no_sect);

        printf("SUCCESS\n");
        printf("version=%s\n", versionString);
        printf("nr_sections=%s\n", noSectString);
        for (int k = 0; k < no_sect; k++)
        {
            read(fis, sect_name, 6);
            sect_name[6] = '\0';
            read(fis, &sect_type, 2);
            read(fis, &sect_offset, 4);
            read(fis, &sect_size, 4);
            printf("section%d: %s %hu %d\n", k + 1, sect_name, sect_type, sect_size); /// printez valorile in forma dorita
        }
    }
    /// inchid fisierul si returnez 1, oentru ca este un fisier SF
    close(fis);
    return 1;
}

void finalParseSF(const char *path)
{
    int result = parseSF(path, 0);
    if (result == -1)
    {
        /// nu se poate deschide fisierul
        printf("ERROR\ninvalid file");
    }
    else if (result == -2)
    {
        /// magicul nu respecta cerinta SF-ului
        printf("ERROR\nwrong magic");
    }
    else if (result == -3)
    {
        /// versiunea nu respecta cerinta SF-ului
        printf("ERROR\nwrong version");
    }
    else if (result == -4)
    {
        /// nr de sectiune nu respecta cerinta SF-ului
        printf("ERROR\nwrong sect_nr");
    }
    else if (result == -5)
    {
        /// tipul sectiunii nu este conform cerintei
        printf("ERROR\nwrong sect_types");
    }
}

void extractFrSF(const char *path, int section, int line)
{
    if (parseSF(path, 1) != 1)
    {
        printf("ERROR\ninvalid file");
        return;
    }

    int fis = open(path, O_RDONLY); /// nu mai trebuie sa verific fis ca l-am verificat in parse

    lseek(fis, -3, SEEK_END); /// cu 3 octeti in spate de la sfarsitul fisierului
    unsigned short header_size;
    read(fis, &header_size, 2); /// citesc dimensiunea headerului de 2 octeti

    char magic;
    read(fis, &magic, 1);               /// citesc apoi magic-ul de 1 octet
    lseek(fis, -header_size, SEEK_END); /// merg la inceputul header-ului

    unsigned short version = 0;
    read(fis, &version, 2);
    unsigned char no_sect;
    read(fis, &no_sect, 1);

    char sect_name[7];
    unsigned short sect_type;
    int sect_size, sect_offset;
    if (no_sect < section || section <= 0)
    {
        printf("ERROR\ninvalid section");
        return;
    }
    for (int i = 0; i < no_sect; i++)
    {
        /// parcurg intai informatiile despre sectiune si dupa merg in body la offset-ul sectiunii
        read(fis, sect_name, 6);
        read(fis, &sect_type, 2);
        read(fis, &sect_offset, 4);
        read(fis, &sect_size, 4);

        if ((i + 1) == section)
        {
            /// daca sectiunea coincide cu valoarea primita, duc cursorul la offset-ul sectiunii
            lseek(fis, sect_offset, SEEK_SET);
            char c;
            int nrLinie = 0, lungimeLinie, j = 0, start = -1;

            while (j < sect_size)
            { ///  cat timp j mai mic decat dimensiunea sectiunii
                read(fis, &c, 1);
                if (c == '\n')
                {
                    /// a gasit un \n care indica o linie
                    nrLinie++;
                    if (line == nrLinie)
                    {
                        /// daca este linia cautata, atunci stabilim lunigimea liniei ca  diferenta dintre
                        /// pozitia la care se afla cursorul si de unde a inceput linia
                        lungimeLinie = j - start;
                        char *linie = (char *)malloc(sizeof(char) * (lungimeLinie + 1));
                        /// am setat cursorul fisierului de la inceputul sectiunii plus nr de octeti pana la startul linie plus 1
                        lseek(fis, sect_offset + start + 1, SEEK_SET);
                        /// citesc o linie
                        read(fis, linie, lungimeLinie);
                        linie[lungimeLinie] = '\0';
                        printf("SUCCESS\n");

                        for (int k = lungimeLinie - 2; k >= 0; k--)
                        {
                            /// scriu linia inversata
                            printf("%c", linie[k]);
                        }

                        free(linie);
                        return;
                    }
                    start = j;
                }
                j++;
            }
            /// aici este cazul in care linia cautata este ultima linie care nu e delimitata de \n, si continua cu octetii aceia de umplutura dintre sectiuni
            if (nrLinie + 1 == line)
            {
                lungimeLinie = j - start;
                char *linie = (char *)malloc(sizeof(char) * lungimeLinie);
                lseek(fis, sect_offset + start, SEEK_SET);
                read(fis, linie, lungimeLinie);
                linie[lungimeLinie] = '\0';
                printf("SUCCESS\n");
                for (int k = lungimeLinie - 1; k >= 0; k--)
                {
                    /// scriu linia inversata
                    printf("%c", linie[k]);
                }
                free(linie);
                return;
            }
        }
    }
}
int gasesteSF(const char *path)
{
    int fis = -1;
    fis = open(path, O_RDONLY); /// deschid fisierul

    lseek(fis, -3, SEEK_END);
    unsigned short header_size;
    read(fis, &header_size, 2); /// citesc dimensiunea header-ului
    lseek(fis, -header_size + 2, SEEK_END);

    unsigned char no_sect;
    read(fis, &no_sect, 1); /// citesc numarul de sectiuni

    char sect_name[7];
    unsigned short sect_type;
    int sect_size, sect_offset;

    for (int i = 0; i < no_sect; i++)
    {
        read(fis, sect_name, 6);
        read(fis, &sect_type, 2);
        read(fis, &sect_offset, 4);
        read(fis, &sect_size, 4);
        off_t pos = lseek(fis, 0, SEEK_CUR);

        char c;
        int nrLinie = 0, j = 0;
        lseek(fis, sect_offset, SEEK_SET);

        while (j < sect_size)
        {
            read(fis, &c, 1);
            if (c == '\n')
            {
                ++nrLinie;
            }
            if (nrLinie >= 13)
            {
                return 1;
            }
            j++;
        }
        if (nrLinie + 1 >= 13)
        {
            return 1;
        }
        lseek(fis, pos, SEEK_SET);
    }
    return -1;
}
void findallSFs(const char *path)
{
    DIR *director = opendir(path);
    struct dirent *director_content = readdir(director);
    char exactPath[1000];
    struct stat metaDate;
    while (director_content != NULL)
    {
        if (strcmp(director_content->d_name, ".") != 0 && strcmp(director_content->d_name, "..") != 0)
        {
            snprintf(exactPath, 1000, "%s/%s", path, director_content->d_name);
            if (lstat(exactPath, &metaDate) == 0)
            {

                if (parseSF(exactPath, 1) == 1)
                {
                    if (gasesteSF(exactPath) == 1)
                        printf("%s\n", exactPath);
                }

                if (S_ISDIR(metaDate.st_mode))
                {
                    findallSFs(exactPath);
                }
            }
        }
        director_content = readdir(director);
    }
    closedir(director);
}

int main(int argc, char **argv)
{
    int recursiveFlag = 0, pathFlag = 0, filterStrFlag = 0, listFlag = 0, filterFlag = 0, parseFlag = 0;
    char *myPath = NULL, *filter = NULL;
    int line, section, extractFlag = 0, lineFlag = 0, sectionFlag = 0, findFlag;

    if (argc >= 2)
    {
        for (int i = 0; i < argc; i++)
        {
            if (strcmp(argv[i], "variant") == 0)
            {
                printf("12095\n");
                return 0;
            }
            else if (strcmp(argv[i], "recursive") == 0)
            {
                recursiveFlag = 1;
            }
            else if (strcmp(argv[i], "parse") == 0)
            {
                parseFlag = 1;
            }
            else if (strstr(argv[i], "path=") != NULL)
            {
                pathFlag = 1;
                myPath = (char *)malloc(sizeof(char) * (strlen(argv[i]) - 4));
                sscanf(argv[i], "path=%s", myPath);
            }
            else if (strstr(argv[i], "name_starts_with=") != NULL)
            {
                filterStrFlag = 1;
                filter = (char *)malloc(sizeof(char) * (strlen(argv[i]) - 16));
                sscanf(argv[i], "name_starts_with=%s", filter);
            }
            else if (strcmp(argv[i], "has_perm_execute") == 0)
            {
                filterFlag = 1;
            }
            else if (strcmp(argv[i], "list") == 0)
            {
                listFlag = 1;
            }
            else if (strcmp(argv[i], "extract") == 0)
            {
                extractFlag = 1;
            }
            else if (strstr(argv[i], "section=") != NULL)
            {
                sscanf(argv[i], "section=%d", &section);
                sectionFlag = 1;
            }
            else if (strstr(argv[i], "line=") != NULL)
            {
                sscanf(argv[i], "line=%d", &line);
                lineFlag = 1;
            }
            else if (strcmp(argv[i], "findall") == 0)
            {
                findFlag = 1;
            }
        }
        if (extractFlag == 1 && pathFlag == 1 && lineFlag == 1 && sectionFlag == 1)
        {
            extractFrSF(myPath, section, line);
        }
        else if (findFlag == 1 && pathFlag == 1)
        {
            DIR *nouDir = opendir(myPath);
            if (nouDir == NULL)
            {
                printf("ERROR\ninvalid directory path");
            }
            else
            {
                printf("SUCCESS\n");
                findallSFs(myPath);
            }
            free(nouDir);
        }
        else if (recursiveFlag == 1)
        {
            if (listFlag == 1 && pathFlag == 1)
            {
                DIR *nouDir2 = opendir(myPath);
                if (nouDir2 == NULL)
                {
                    printf("ERROR\ninvalid directory path");
                }
                else
                {
                    printf("SUCCESS\n");
                    listRecFilter(myPath, filter, filterStrFlag, filterFlag);
                }
                free(nouDir2);
            }
        }
        else
        {
            if (listFlag == 1 && pathFlag == 1)
            {
                listFilter(myPath, filter, filterStrFlag, filterFlag);
            }
            else if (pathFlag == 1 && parseFlag == 1)
            {
                finalParseSF(myPath);
            }
        }
    }
    free(myPath);
    free(filter);
    return 0;
}
