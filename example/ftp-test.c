#include "ftp.h"

#include <stdio.h>

int main(void) {
    ftp_login();
    char dir[32768];

    //FILE *f = fopen("a.txt", "rb");
    //printf("%d\n", ftp_cd("/home/zyh/hello"));
    //printf("%d\n", ftp_mv("a.txt", "p.txt"));
    printf("%d\n", ftp_dir(".", dir));
    printf("%d\n", ftp_pwd(dir));
    //printf("%d\n", ftp_put(fileno(f), "c.txt"));
    //printf("%d\n", ftp_rmdir("mydir"));
    //fclose(f);

    return 0;
}
