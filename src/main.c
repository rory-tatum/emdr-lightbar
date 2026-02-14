#include <stdio.h>
#include "main.h"

const char *hello_message(void) {
    return "Hello, World!";
}

int main(void) {
    printf("%s\n", hello_message());
    return 0;
}
