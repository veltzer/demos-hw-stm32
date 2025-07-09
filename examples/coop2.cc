#include <stdio.h>
#include <setjmp.h>

jmp_buf buf1, buf2;

void func1() {
    while (1) {
        printf("Func1\n");
        longjmp(buf2, 1);
    }
}

void func2() {
    while (1) {
        printf("Func2\n");
        longjmp(buf1, 1);
    }
}

int main() {
    if (!setjmp(buf1)) func1();
    if (!setjmp(buf2)) func2();
    return 0;
}
