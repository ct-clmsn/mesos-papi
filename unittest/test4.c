#include <papi.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <Python.h>
 
static inline void PyExec(const char* pyfile) {
  FILE* fd = fopen(pyfile, "r");
  Py_Initialize();
  PyRun_SimpleFile(fd, pyfile);
  Py_Finalize();
}

#include <jni.h>

static inline void JvmExec(const char* java_class_name) {
  JavaVM* jvm;
  JNIEnv* env;
  JavaVMInitArgs vm_args;
  JavaVMOption options[1];

  vm_args.version = JNI_VERSION_1_2;
  vm_args.options = options;
  vm_args.nOptions = 0;
  vm_args.ignoreUnrecognized = JNI_TRUE;

  JNI_GetDefaultJavaVMInitArgs(&vm_args);
  JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args);

  jclass cls = env->FindClass(java_class_name);
  // [L -> array of strings
  jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");

  env->CallStaticVoidMethod(cls, mid, "");
  jvm->DestroyJavaVM();
}

#include<Rembedded.h>
#include<Rinterface.h>

extern int R_running_as_main_program;

static inline void RExec(const char* rfile) {
  char** args;
  (*args) = (char*) malloc(sizeof(char*)*6);
  char* argvbuf [] = { "--slave", "--no-save", "--silent", "-q", "-f" };

  for(size_t i = 0; i < 6; i++) {
    args[i] = (char*)malloc(sizeof(char) * 255);
    if(i < 5) {
      sprintf(args[i], "%s", argvbuf[i]);
    }
    else {
      sprintf(args[i], "%s", rfile);
    }
  }
  
  R_running_as_main_program = 1;
  Rf_initialize_R(6, args);
  Rf_mainloop();
  
  for(size_t i = 0; i < 6; i++) { 
    free(args[i]);
  }
  free(args);

}

#include<ruby.h>

static void RubyExec(const char* rbfile) {
  ruby_init();
  char** options;
  (*options) = (char*)malloc(sizeof(char*)*1);
  options[0] = (char*)malloc(sizeof(char*)*255); 
  sprintf(options[0], "%s", rbfile);
  void* node = ruby_options(1, options);
  int state = -1;
  if(ruby_executable_node(node, &state)) {
    state = ruby_exec_node(node);
  }

  free(options[0]);
  free(options);
}

void run() {
  int pfdsp[2], pfdsc[2];
  pipe(pfdsp); // parent fd
  pipe(pfdsc); // child fd
 
  pid_t pid = fork();
 
  if (pid == -1) {
    perror("fork");
    exit(0);
  }

  // child
  if (pid == 0){
    pid_t chld_pid = getpid();
    write(pfdsc[1], &chld_pid, sizeof(pid_t)); // parent fd

    int val = 0;
    read(pfdsp[0], &val, sizeof(int)); // parent fd

    //execv("python floatloop.py", NULL);
    //execv("java FloatLoop", NULL);
    execv("./floatloop", NULL);
    //PyExec("floatloop.py");
    //JvmExec("FloatLoop");
    //RExec("floatloop.R");
    //RubyExec("floatloop.rb");
  }
  // parent
  else {
    const int eventscount = 10;
    int events[eventscount] = { PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_FP_OPS, PAPI_FP_INS, PAPI_FAD_INS, PAPI_SP_OPS, PAPI_DP_OPS, PAPI_VEC_INS, PAPI_VEC_SP, /*PAPI_VEC_DP*/ PAPI_L1_DCA }, eventset = PAPI_NULL;

    int ok = PAPI_library_init(PAPI_VER_CURRENT);
    const PAPI_component_info_t* cmpinfo = PAPI_get_component_info( 0 );
    if(cmpinfo->attach == 0) { perror("no attaches\n"); }

    int status;
    int val = 1;

    long long values[eventscount];
    values[0] = values[1] = values[2] = values[3] = values[4] = values[5] = values[6] = values[7] = values[8] = values[9] = 0;
 
    pid_t child_pid = 0, w = 0;
    read(pfdsc[0], &child_pid, sizeof(pid_t)); // child fd

    ok = PAPI_create_eventset(&eventset);
    if(ok != PAPI_OK) { perror("ERROR eventset-1\n"); }

    ok = PAPI_assign_eventset_component(eventset, 0);
    if(ok != PAPI_OK) { perror("ERROR eventset-2\n"); }

    ok = PAPI_attach(eventset, (unsigned long)child_pid); 
    if(ok != PAPI_OK) { perror("ERROR attach\n"); }

    PAPI_add_events(eventset, events, eventscount);

    PAPI_start(eventset);

    write(pfdsp[1], &val, sizeof(int)); // parent fd

    do {
        w = waitpid(child_pid, &status, WUNTRACED | WCONTINUED); 
        if(w == -1) {
          perror("waitpid error");
          exit(EXIT_FAILURE);
        }
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));

    PAPI_stop(eventset, values);
    PAPI_detach(eventset);

    PAPI_shutdown();

    printf("results\n"); fflush(stdout);
    for(int i = 0; i < eventscount; i++) { printf("\t%d\t%lld\n", i, values[i]); }
  }
 
}
 
int main(int argc, char** argv) {
 
  run();
 
}

