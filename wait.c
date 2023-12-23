#include "syscall.h"


void main() {
   while (1) {
        mlkprint("J'attend un signal");
        mlk_wait();
        mlk_sleep(1);
        mlkprint("je recois un signal");
    }
}