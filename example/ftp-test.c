#include "ftp.h"

#include <stdio.h>

int main(void) {
    ftp_login();

    FILE *f = fopen("a.txt", "rb");
    printf("%d\n", ftp_cd("/home/zyh/hello"));
    printf("%d\n", ftp_put(fileno(f), "c.txt"));
    //printf("%d\n", ftp_rmdir("mydir"));
    fclose(f);

    return 0;
}
