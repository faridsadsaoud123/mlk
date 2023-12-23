#include "syscall.h"
void main() {
   int p;

   p=spawn("./wait.so");
  while (1) {
    mlkprint("J'envoie un signal");
    mlk_signal(p);
    // mlk_sleep(5);
    // printf("salut");
  }
}