#include "ftp.h"

#include <stdio.h>

int main(void) {
    ftp_login();

    FILE *f = fopen("a.tmp", "wb");
    printf("%d\n", ftp_get(fileno(f), "a.s"));
    fclose(f);

    return 0;
}