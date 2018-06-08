#ifndef FTP_H
#define FTP_H

#define FTP_COMMAND_PORT 21
#define DATA_BUF_LEN 1024
#define DIR_BUF_LEN 32768

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

struct sockaddr_in sin;
char data_buf[DATA_BUF_LEN];
char send_buf[256];
char recv_buf[256];
char input_buf[50];
char dir_buf[DIR_BUF_LEN];
int sfd;

int ftp_get_response(void);
void ftp_login(void);
int ftp_data_socket(const char* type);
int ftp_get(int fd, const char* filename);
int ftp_put(int fd, const char* filename);
int ftp_mkdir(char* dirname);     // relative path
int ftp_rm(char* filename);     // remove a file
int ftp_rmdir(char* dirname);     // remove a dir, relative
int ftp_cd(char* dirname);       // dir must be abusolute path
int ftp_mv(const char *from, const char *to);
int ftp_dir(const char *path, char* buf);
int ftp_pwd(char* buf);

pthread_mutex_t ftp_mutex;

#endif
