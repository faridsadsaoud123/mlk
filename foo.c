#include <stdio.h>

void main() {
  int i=0;
  while (1) {
    printf("foo\n");
    for (i=0;i<100000000;i++);
    }
}

