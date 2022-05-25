#include <stdio.h>
#include <unistd.h>

int main() {

    int x = 32;
    printf("stack ptr: %p\n", (void*)&x);

    FILE* f = fopen("/proc/mmaneg", "w");
    fprintf(f, "findpage %llx\n", (void*)&x);

    sleep(3);

    return 0;
}