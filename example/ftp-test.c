#include "ftp.h"

#include <stdio.h>

int main(void) {
    ftp_login();

    FILE *f = fopen("a.c", "wb");
    printf("%d\n", ftp_get(fileno(f), "a.tar.gz"));
    fclose(f);

    return 0;
}