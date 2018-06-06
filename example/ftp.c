#include <string.h>
#include "ftp.h"

int ftp_get_response(void) {
    int len = read(sfd, recv_buf, sizeof(recv_buf));
    if (len < 3) return -1;
    recv_buf[3] = '\0';
    return atoi(recv_buf);
}

void ftp_login(void) {
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd >= 0);
    
    #define IP "162.211.224.25"
    #ifdef IP
        strcpy(input_buf, IP);
    #else
        printf("Please enter the IP address of the FTP server: ");
        gets(input_buf);
    #endif
    sin.sin_family = AF_INET;
    sin.sin_port = htons(FTP_COMMAND_PORT);
    inet_pton(AF_INET, input_buf, &sin.sin_addr);

    assert(connect(sfd, (struct sockaddr*) (&sin), sizeof(sin)) != -1);
    assert(ftp_get_response() == 220);

    #define USERNAME "zyh"
    #ifdef USERNAME
        strcpy(input_buf, USERNAME);
    #else
        printf("Please enter the username: ");
        gets(input_buf);
    #endif
    strcpy(send_buf, "USER ");
    strcat(send_buf, input_buf);
    strcat(send_buf, "\r\n");
    assert(send(sfd, send_buf, strlen(send_buf), 0) > 0);
    assert(ftp_get_response() == 331);
    
    struct termios default_settings, password_settings;
    tcgetattr(fileno(stdin), &default_settings);
    password_settings = default_settings;
    password_settings.c_lflag &= ~ECHO;

    #define PASSWD "zyh123"
    #ifdef PASSWD
        strcpy(input_buf, PASSWD);
    #else
        printf("Please enter the password: ");
        assert(tcsetattr(fileno(stdin), TCSAFLUSH, &password_settings) == 0);
        gets(input_buf);
        assert(tcsetattr(fileno(stdin), TCSANOW, &default_settings) == 0);
        printf("\n");
    #endif
    strcpy(send_buf, "PASS ");
    strcat(send_buf, input_buf);
    strcat(send_buf, "\r\n");
    assert(send(sfd, send_buf, strlen(send_buf), 0) > 0);
    assert(ftp_get_response() == 230);

    strcpy(send_buf, "SYST\r\n");
    assert(send(sfd, send_buf, strlen(send_buf), 0) > 0);
    assert(ftp_get_response() == 215);
}

int ftp_data_socket(const char* type) {
    strcpy(send_buf, "TYPE ");
    strcat(send_buf, type);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) return -1;
    if (ftp_get_response() != 200) return -1;

    strcpy(send_buf, "PASV\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) return -1;
    if (ftp_get_response() != 227) return -1;
    int ip0, ip1, ip2, ip3, port0, port1;
    sscanf(recv_buf + 4, "Entering Passive Mode (%d,%d,%d,%d,%d,%d", &ip0, &ip1, &ip2, &ip3, &port0, &port1);
    uint16_t pasv_port = port0 * 256 + port1;

    int dfd = socket(AF_INET, SOCK_STREAM, 0);
    if (dfd < 0) return -1;

    struct sockaddr_in din = sin;
    din.sin_port = htons(pasv_port);
    if (connect(dfd, (struct sockaddr*) (&din), sizeof(din)) == -1) goto failed;
    return dfd;

failed:
    close(dfd);
    return -1;
}

int ftp_get(int fd, const char* filename) {
    int dfd = ftp_data_socket("I");
    if (dfd == -1) return -1;

    strcpy(send_buf, "RETR ");
    strcat(send_buf, filename);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 150) goto failed;

    int file_len;
    int offset = 0;
    sscanf(recv_buf + 46 + strlen(filename), "%d", &file_len);
    while (offset < file_len) {
        int len = recv(dfd, data_buf, DATA_BUF_LEN, 0);
        if (len <= 0) goto failed;
        pwrite(fd, data_buf, len, offset);
        offset += len;
    }
    
    close(dfd);
    if (ftp_get_response() != 226) return -1;
    return 0;

failed:
    close(dfd);
    return -1;
}

int ftp_put(int fd, const char* filename) {
    int dfd = ftp_data_socket("I");
    if (dfd == -1) return -1;

    strcpy(send_buf, "STOR ");
    strcat(send_buf, filename);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 150) goto failed;

    int len;
    int offset = 0;
    do {
        len = pread(fd, data_buf, DATA_BUF_LEN, offset);
        if (len > 0) {
            int recv_len = send(dfd, data_buf, len, 0);
            if (recv_len <= 0) goto failed;
            offset += len;
        }
    } while (len > 0);

    close(dfd);
    if (ftp_get_response() != 226) return -1;
    return 0;

failed:
    close(dfd);
    return -1;
}

int ftp_mkdir(char* dirname) {
    strcpy(send_buf, "MKD ");
    strcat(send_buf, dirname);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 257) goto failed;

    return 0;

failed:
    return -1;
}

int ftp_rm(char* filename) {
    strcpy(send_buf, "DELE ");
    strcat(send_buf, filename);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 250) goto failed;

    return 0;

failed:
    return -1;
}

int ftp_rmdir(char* dirname) {
    strcpy(send_buf, "RMD ");
    strcat(send_buf, dirname);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 250) goto failed;

    return 0;

failed:
    return -1;
}

int ftp_cd(char* dirname) {
    strcpy(send_buf, "CWD ");
    strcat(send_buf, dirname);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 250) goto failed;

    return 0;

failed:
    return -1;
}

int ftp_mv(const char *from, const char *to)
{
    strcpy(send_buf, "RNFR ");
    strcat(send_buf, from);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 350) goto failed;

    strcpy(send_buf, "RNTO ");
    strcat(send_buf, to);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 250) goto failed;

    return 0;

failed:
    return -1;
}

int ftp_dir(const char *path, char* buf)
{
    int dfd = ftp_data_socket("A");
    if (dfd == -1) return -1;

    strcpy(send_buf, "LIST ");
    strcat(send_buf, path);
    strcat(send_buf, "\r\n");
    if (send(sfd, send_buf, strlen(send_buf), 0) <= 0) goto failed;
    if (ftp_get_response() != 150) goto failed;

    int offset = 0, i;
    int len = recv(dfd, dir_buf, DIR_BUF_LEN, 0);
    if (len <= 0) goto failed;
    memcpy(buf, dir_buf + 62, len - 62);

    close(dfd);
    if (ftp_get_response() != 226) return -1;
    return 0;

failed:
    close(dfd);
    return -1;
}
