#include <stdio.h>
#include <unistd.h>

int main() {

    FILE* f = fopen("/dev/my_queue", "w");
    if (!f) {
        printf("can't open device\n");
        return 1;
    }

    fprintf(f, "1 from A\n");
    fflush(f);

    printf("<-\n");
    sleep(8);
    printf("->\n");

    fprintf(f, "2 from A\n");
    fflush(f);

    fclose(f);

    return 0;
}