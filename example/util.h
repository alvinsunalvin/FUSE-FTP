#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

int createMultiLevelDir(char *sPathName)
{
    char DirName[256];
    int i, len;

    strcpy(DirName, sPathName);
    len = strlen(DirName);
    // if ('/' != DirName[len - 1])
    // {
    //     strcat(DirName, "/");
    //     len++;
    // }
    for (i = 1; i < len; i++)
    {
        if ('/' == DirName[i])
        {
            DirName[i] = '\0';
            if (access(DirName, F_OK) != 0)
            {
                if (mkdir(DirName, 0777) == -1)
                {
                    perror("mkdir() failed!");
                    return -1;
                }
            }
            DirName[i] = '/';
        }
    }

    return 0;
}

#endif