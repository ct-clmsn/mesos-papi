#include <papi.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

void run() {

  int pfdsp[2], pfdsc[2];
  pipe(pfdsp);
  pipe(pfdsc);

  pid_t pid = fork();

  if (pid == -1) {
    perror("fork");
    exit(0);
  }

  if (pid == 0){
    long long values [4];
    values[0] = values[1] = values[2] = values[3] = 0;

    //int events[4] = { PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_FP_OPS, PAPI_FP_INS }, eventset;
    //int retval = PAPI_library_init(PAPI_VER_CURRENT);

    //PAPI_create_eventset(&eventset);
    //PAPI_add_events(eventset, events, 4);

    int val = 0;
    read(pfdsp[0], &val, sizeof(int)); 

/*    pid_t chld_pid = getpid();
    PAPI_attach(eventset, chld_pid);
    PAPI_start(eventset);

    execv("floatloop", NULL);

    PAPI_stop(eventset, values);
    PAPI_detach(eventset); */

values[0] = 101;
values[1] = 102;
values[2] = 120;
values[3] = 100;

printf("boo\n"); fflush(stdout);
    printf("%d\n", write(pfdsc[1], values, sizeof(int)*4)); fflush(stdout);
printf("boo\n"); fflush(stdout);
    _exit(1);
  }
  else {
    int status;
    long long values [4];
    values[0] = values[1] = values[2] = values[3] = 0;

    int val = 1;
    write(pfdsp[1], &val, sizeof(int)); 

    //do {
    //  waitpid(pid, &status, WUNTRACED | WCONTINUED);
    //} while(!WIFEXITED(status) && !WIFSIGNALED(status));
    printf("%d\n", read(pfdsc[0], values, sizeof(int)*4)); fflush(stdout);

    printf("results\n"); fflush(stdout);
    for(int i = 0; i < 4; i++) { printf("\t%d\t%d\n", i, values[i]); }

  }

  //PAPI_shutdown();
}

/* init papi */
int main(int argc, char** argv) {

  run();

}
