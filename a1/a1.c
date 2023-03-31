#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

void listRec(const char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entry = readdir(dir);
    char fullPath[512];
    struct stat statbuf;    
  
    while ( entry != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0)
            {
                printf("%s\n", fullPath);
                if (S_ISDIR(statbuf.st_mode))
                {
                    listRec(fullPath);
                }
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
}
void list(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path");
        return;
    }
    printf("SUCCESS\n");
    struct dirent *entry = readdir(dir);
    while (entry != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            printf("%s/%s\n", path, entry->d_name);
        }
        entry = readdir(dir);
    }
    closedir(dir);
}

int main(int argc, char **argv)
{
    int recursiveFlag, pathFlag, filterFlag, listFlag;
    char *myPath = NULL, *filter = NULL;

    if (argc >= 2)
    {

        for (int i = 0; i < argc; i++)
        {

            if (strcmp(argv[i], "variant") == 0)
            {
                printf("12095\n");
            }
            else if (strcmp(argv[i], "recursive") == 0)
            {
                recursiveFlag = 1;
            }
            else if (strstr(argv[i], "path=") != NULL)
            {
                pathFlag = 1;
                myPath = (char *)malloc(sizeof(char) * (strlen(argv[i]) - 4));
                sscanf(argv[i], "path=%s", myPath);
            }
            else if (strstr(argv[i], "name_starts_with=") != NULL)
            {
                filterFlag = 1;
                filter = (char *)malloc(sizeof(char) * (strlen(argv[i]) - 16));
                sscanf(argv[i], "name_starts_with=%s", filter);
            }
            else if (strstr(argv[i], "has_perm_execute=") != NULL)
            {
                filterFlag = 1;
                filter = (char *)malloc(sizeof(char) * (strlen(argv[i]) - 16));
                sscanf(argv[i], "has_perm_execute=%s", filter);
            }
            else if (strcmp(argv[i], "list") == 0)
            {
                listFlag = 1;
            }
        }
        if (recursiveFlag == 1)
        {
            if (listFlag == 1 && pathFlag == 1)
            {
                DIR *nouDir = opendir(myPath);
                if (nouDir == NULL)
                {
                    perror("ERROR\ninvalid directory path");
                }
                else
                {
                    printf("SUCCESS\n");
                    listRec(myPath);
                }
            }
        }
        else
        {
            if (listFlag == 1 && pathFlag == 1)
            {

                list(myPath);
                if(filterFlag){
                    ///in cazul in care avem filtre
                }
            }
        }
        return 0;
    }
}