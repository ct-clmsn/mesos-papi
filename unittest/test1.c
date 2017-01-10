#include <papi.h>
#include <stdio.h>

/* init papi */
int main(int argc, char** argv) {
  const int ncounters = PAPI_num_counters(); 
  printf("%d\n", ncounters);
  uint events[2] = { PAPI_TOT_INS, PAPI_TOT_CYC };
  long long values [2];
  PAPI_start_counters(events, 2);
  int retval = PAPI_stop_counters(values, 2);
}
