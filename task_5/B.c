#include <stdio.h>
#include <unistd.h>

int main() {

    FILE* f = fopen("/dev/my_queue", "w");
    if (!f) {
        printf("can't open device\n");
        return 1;
    }

    fprintf(f, "1 from B\n");
    fflush(f);
    printf("<B>\n");

    fclose(f);

    return 0;
}