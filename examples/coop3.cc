#include <stdio.h>
#include <setjmp.h>

jmp_buf main_ctx, f1_ctx, f2_ctx;

void f1() {
    while (1) {
        printf("Function 1\n");
        longjmp(main_ctx, 1);
    }
}

void f2() {
    while (1) {
        printf("Function 2\n");
        longjmp(main_ctx, 2);
    }
}

int main() {
    int state;

    if ((state = setjmp(main_ctx)) == 0) {
        if (!setjmp(f1_ctx)) f1();
    } else if (state == 1) {
        if (!setjmp(f2_ctx)) f2();
    }

    while (1) {
        longjmp(f1_ctx, 1);  // Run a bit of f1
        longjmp(f2_ctx, 1);  // Run a bit of f2
    }

    return 0;
}
