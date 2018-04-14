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
#include <termios.h>

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
    char send_buf[256];
    char input_buf[50];
    printf("Please enter the IP address of the FTP server: ");
    gets(input_buf);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(FTP_COMMAND_PORT);
    inet_pton(AF_INET, input_buf, &sin.sin_addr);

    assert(connect(fd, (struct sockaddr*) (&sin), sizeof(sin)) != -1);
    assert(ftp_get_response() == 220);

    printf("Please enter the username: ");
    gets(input_buf);
    strcpy(send_buf, "USER ");
    strcat(send_buf, input_buf);
    strcat(send_buf, "\r\n");
    assert(send(fd, send_buf, strlen(send_buf), 0) > 0);
    assert(ftp_get_response() == 331);
    
    struct termios default_settings, password_settings;
    tcgetattr(fileno(stdin), &default_settings);
    password_settings = default_settings;
    password_settings.c_lflag &= ~ECHO;

    printf("Please enter the password: ");
    assert(tcsetattr(fileno(stdin), TCSAFLUSH, &password_settings) == 0);
    gets(input_buf);
    assert(tcsetattr(fileno(stdin), TCSANOW, &default_settings) == 0);
    printf("\n");
    strcpy(send_buf, "PASS ");
    strcat(send_buf, input_buf);
    strcat(send_buf, "\r\n");
    assert(send(fd, send_buf, strlen(send_buf), 0) > 0);
    assert(ftp_get_response() == 230);

    strcpy(send_buf, "SYST\r\n");
    assert(send(fd, send_buf, strlen(send_buf), 0) > 0);
    assert(ftp_get_response() == 215);
}