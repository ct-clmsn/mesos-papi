#include <stdlib.h>
#include <stdio.h>
int main() {

   float res = 0.0;

   for(int i = 0; i < 10000000; i++) {
     res += 1.0;
   }

   printf("%f\n", res);
   fflush(stdout);
}
