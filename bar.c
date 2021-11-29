#include <stdio.h>
#include "syscall.h"

void main() {
  int i=0;
  while (1) {
    printf("bar %d\n",mlk_clock());
    for (i=0;i<1000000000;i++);

    }
}

