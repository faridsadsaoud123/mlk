#include <stdio.h>
#include "syscall.h"

void main() {
  int i=0;
  while (1) {
    printf("foo %d\n",mlk_clock());
//    for (i=0;i<100000000;i++);
    mlk_sleep(5);
    }
}

