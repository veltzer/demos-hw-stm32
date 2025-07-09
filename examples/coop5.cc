#include <stdio.h>
#include <setjmp.h>

jmp_buf env_main, env1, env2;

void task1() {
    int i = 0;
    while (1) {
        printf("Task 1 - %d\n", i++);
        longjmp(env_main, 1);
    }
}

void task2() {
    int i = 0;
    while (1) {
        printf("Task 2 - %d\n", i++);
        longjmp(env_main, 2);
    }
}

int main() {
    int state;

    if ((state = setjmp(env_main)) == 0) {
        if (setjmp(env1) == 0) task1();
        if (setjmp(env2) == 0) task2();
    }

    while (1) {
        if (state == 1)
            longjmp(env2, 1);
        else
            longjmp(env1, 1);
    }

    return 0;
}

