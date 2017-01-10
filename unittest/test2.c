#include <papi.h>
#include <stdio.h>

/* init papi */
int main(int argc, char** argv) {
  int events[2] = { PAPI_TOT_INS, PAPI_TOT_CYC }, eventset;
  long long values [2];

  int retval = PAPI_library_init(PAPI_VER_CURRENT);

  const int ncounters = PAPI_num_counters(); 
  printf("%d\n", ncounters);

  PAPI_create_eventset(&eventset);
  PAPI_add_events(eventset, events, 2);

  PAPI_start(eventset);
  PAPI_stop(eventset, values);

  for(int i = 0; i < 2; i++) { printf("\t%d\n", values[i]); }

  PAPI_shutdown();
}
