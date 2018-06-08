#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#define TMP_FUSE_DIR "/tmp/fuse-ftp" // -> '/'
#define FTP_BASE_DIR "/home/zyh"     // for development
void map_to_cache_path(const char *path, char *cache_path)
{
    // TODO: relative path
    assert(path[0] == '/');
    strcpy(cache_path, TMP_FUSE_DIR);
    strcat(cache_path, path);
    cache_path[strlen(TMP_FUSE_DIR) + strlen(path)] = '\0';
}

void map_to_ftp_path(const char *path, char *ftp_path)
{
    // TODO: relative path
    assert(path[0] == '/');
    int len = strlen(FTP_BASE_DIR);
    strcpy(ftp_path, FTP_BASE_DIR);
    strcat(ftp_path, path);
    int path_len = strlen(path);
    // if (!is_dir || ftp_path[len + path_len - 1] == '/') {
        ftp_path[len + path_len] = '\0';
    // } else {
    //     ftp_path[len + path_len] = '/';
    //     ftp_path[len + path_len + 1] = '\0';
    // }
}

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
                if (mkdir(DirName, 0755) == -1)
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

int cut_ftp_path(char *ftp_path, char *path) {
    strcpy(path, ftp_path + strlen(FTP_BASE_DIR));
    return 0;
}

#endif