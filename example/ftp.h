#ifndef FTP_H
#define FTP_H

#define FTP_COMMAND_PORT 21

int fd;

int ftp_get_response(void);
void ftp_login(void);

#endif
