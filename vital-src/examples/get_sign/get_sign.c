/*
 * First KLEE tutorial: testing a small function
 */

#include "klee/klee.h"
#include <stdio.h>
/*
int get_sign(int x) {
  if (x == 0)
     return 0;

  if (x < 0)
     return -1;
  else
     return 1;
}
*/
void get_sign(int x) {
  if (x != 0){
     if (x > 100) {
         if (x < 200)
            printf("path 1\n");
         else
             printf("path 2\n");
     }
     else
        printf("path 3\n");
  }
  else
     printf("path 4\n");
}

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  get_sign(a);
  return 0;
}
