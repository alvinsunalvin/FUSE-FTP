#include "ftp.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

int ftp_get_response(void) {
    char buf[256];
    int len = read(fd, buf, sizeof(buf));
    if (len < 3) return -1;
    buf[3] = '\0';
    return atoi(buf);
}

void ftp_login(void) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);
    
    struct sockaddr_in sin;
    char raw_ip[20];
    printf("Please enter the IP address of the FTP server:\n");
    scanf("%s", raw_ip);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(FTP_COMMAND_PORT);
    inet_pton(AF_INET, raw_ip, &sin.sin_addr);

    assert(connect(fd, (struct sockaddr*) (&sin), sizeof(sin)) != -1);
    assert(ftp_get_response() == 220);
}