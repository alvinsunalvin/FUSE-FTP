#include "ftp.h"

#include <stdio.h>

int main(void) {
    ftp_login();

    FILE *f = fopen("r.txt", "rb");
    //printf("%d\n", ftp_cd("/home/zyh/hello"));
    printf("%d\n", ftp_put(fileno(f), "c.txt"));
    //printf("%d\n", ftp_rmdir("mydir"));
    fclose(f);

    FILE *g = fopen("q.txt", "wb");
    printf("%d\n", ftp_get(fileno(g), "b.txt"));
    fclose(g);
    
    return 0;
}
