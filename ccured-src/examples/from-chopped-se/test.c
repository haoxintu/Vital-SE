#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

struct point {int x, y, z;};

void f(struct point * p, int k){
    if (k % 2)
        p->z++;
    if (k > 0)
        p->x++;
    else
        p->y++;
}

int main() {
  struct point p = {0, 0, 0};
  int j, k;
  klee_make_symbolic(&j, sizeof(j), "j");
  klee_make_symbolic(&k, sizeof(k), "k");

  f(&p, k);
  if (j > 0){
    if (p.y)
      abort();
  } else
      ;
  return 0;
}
