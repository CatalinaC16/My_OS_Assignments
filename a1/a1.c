#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

// LISTARE RECURSIVA

void listRecFilter(const char *path, char *filStart, int filterStr, int filterPerm)
{
    DIR *dir = opendir(path);
    struct dirent *entry = readdir(dir);
    char fullPath[512];
    struct stat statbuf;
    while (entry != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0)
            {
                if (filterStr == 1)
                {
                    if (strncmp(entry->d_name, filStart, strlen(filStart)) == 0)
                    {
                        printf("%s\n", fullPath);
                    }
                }
                else if (filterPerm == 1)
                {
                    if (statbuf.st_mode & S_IXUSR)
                    {
                        printf("%s\n", fullPath);
                    }
                }
                if (filterStr == 0 && filterPerm == 0)
                {
                    printf("%s\n", fullPath);
                }

                if (S_ISDIR(statbuf.st_mode))
                {
                    listRecFilter(fullPath, filStart, filterStr, filterPerm);
                }
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
}

// LISTARE NORMALA

void listFilter(const char *path, char *filStart, int filterStr, int filterPerm)
{
    char fullPath[512];
    struct stat statbuf;
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        printf("ERROR\ninvalid directory path");
        return;
    }
    printf("SUCCESS\n");
    struct dirent *entry = readdir(dir);
    while (entry != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            if (filterStr == 1)
            {
                if (strncmp(entry->d_name, filStart, strlen(filStart)) == 0)
                {
                    printf("%s/%s\n", path, entry->d_name);
                }
            }
            else if (filterPerm == 1)
            {
                snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
                if (lstat(fullPath, &statbuf) == 0)
                {
                    if (statbuf.st_mode & S_IXUSR)
                    {
                        printf("%s/%s\n", path, entry->d_name);
                    }
                }
            }
            if (filterPerm == 0 && filterStr == 0)
            {
                printf("%s/%s\n", path, entry->d_name);
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
}

// PARSE
int parseSF(const char *path, int extr)
{
    int fis = -1;
    fis = open(path, O_RDONLY);
    if (fis == -1)
    {
        return -1;
    }

    lseek(fis, -3, SEEK_END);
    unsigned short header_size;
    read(fis, &header_size, 2);

    char magic;
    read(fis, &magic, 1);
    if (magic != 'M')
    {
        return -2;
    }

    lseek(fis, -header_size, SEEK_END);

    unsigned short version = 0;
    read(fis, &version, 2);
    if (version < 83 || version > 107)
    {
        return -3;
    }

    unsigned char no_sect;
    read(fis, &no_sect, 1);
    if (no_sect < 2 || no_sect > 20)
    {
        return -4;
    }

    char sect_name[6];
    unsigned short sect_type;
    int sect_size, sect_offset;
    for (int i = 0; i < no_sect; i++)
    {
        read(fis, sect_name, 6);
        read(fis, &sect_type, 2);
        read(fis, &sect_offset, 4);
        read(fis, &sect_size, 4);
        if (sect_type != 89 && sect_type != 47 && sect_type != 86)
        {
            return -5;
        }
    }
    lseek(fis, -header_size, SEEK_END);
    lseek(fis, 3, SEEK_CUR);
    if (extr == 0)
    {
        char versionString[10];
        char noSectString[10];

        sprintf(versionString, "%hu", version);
        sprintf(noSectString, "%d", no_sect);

        printf("SUCCESS\n");
        printf("version=%s\n", versionString);
        printf("nr_sections=%s\n", noSectString);
        for (int i = 0; i < no_sect; i++)
        {
            int size = read(fis, sect_name, 6);
            sect_name[size] = '\0';
            read(fis, &sect_type, 2);
            read(fis, &sect_offset, 4);
            read(fis, &sect_size, 4);
            printf("section%d: %s %hu %d\n", i + 1, sect_name, sect_type, sect_size);
        }
    }
    close(fis);
    return 1;
}
void finalParseSF(const char *path)
{
    int result = parseSF(path, 0);
    if (result == -1)
    {
        printf("ERROR\ninvalid file");
    }
    else if (result == -2)
    {
        printf("ERROR\nwrong magic");
    }
    else if (result == -3)
    {
        printf("ERROR\nwrong version");
    }
    else if (result == -4)
    {
        printf("ERROR\nwrong sect_nr");
    }
    else if (result == -5)
    {
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

    int fis = open(path, O_RDONLY);

    lseek(fis, -3, SEEK_END);
    unsigned short header_size;
    read(fis, &header_size, 2);

    char magic;
    read(fis, &magic, 1);
    lseek(fis, -header_size, SEEK_END);

    unsigned short version = 0;
    read(fis, &version, 2);
    unsigned char no_sect;
    read(fis, &no_sect, 1);

    char sect_name[6];
    unsigned short sect_type;
    int sect_size, sect_offset;
    if (no_sect < section || section == 0)
    {
        printf("ERROR\ninvalid section");
        return;
    }
    for (int i = 0; i < no_sect; i++)
    {
        read(fis, sect_name, 6);
        read(fis, &sect_type, 2);
        read(fis, &sect_offset, 4);
        read(fis, &sect_size, 4);

        if ((i + 1) == section)
        {
            lseek(fis, sect_offset, SEEK_SET);
            char c;
            int nrLinie = 0, lungimeLinie, j = 0, start = -1;

            while (j < sect_size)
            {
                read(fis, &c, 1);
                if (c == '\n')
                {
                    nrLinie++;
                    if (line == nrLinie)
                    {
                        lungimeLinie = j - start;
                        char *linie = (char *)malloc(sizeof(char) * lungimeLinie);
                        lseek(fis, sect_offset + start, SEEK_SET);
                        read(fis, linie, lungimeLinie);
                        linie[lungimeLinie] = '\0';
                        printf("SUCCESS\n");
                        for (int k = lungimeLinie - 1; k >= 0; k--)
                        {
                            printf("%c", linie[k]);
                        }
                        free(linie);
                        return;
                    }
                    start = j;
                }

                j++;
            }
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
                    printf("%c", linie[k]);
                }
                free(linie);
                return;
            }
        }
    }
}


int main(int argc, char **argv)
{
    int recursiveFlag = 0, pathFlag = 0, filterStrFlag = 0, listFlag = 0, filterFlag = 0, parseFlag = 0;
    char *myPath = NULL, *filter = NULL;
    int line, section, extractFlag = 0, lineFlag = 0, sectionFlag = 0;

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

        }
        if (extractFlag == 1 && pathFlag == 1 && lineFlag == 1 && sectionFlag == 1)
        {
            extractFrSF(myPath, section, line);
        }
        else if (recursiveFlag == 1)
        {
            if (listFlag == 1 && pathFlag == 1)
            {
                DIR *nouDir = opendir(myPath);
                if (nouDir == NULL)
                {
                    printf("ERROR\ninvalid directory path");
                }
                else
                {
                    printf("SUCCESS\n");
                    listRecFilter(myPath, filter, filterStrFlag, filterFlag);
                }
                free(nouDir);
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
