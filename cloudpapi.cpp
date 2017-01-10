#include "cloudpapi.hpp"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <stout/option.hpp>

#include <map>
#include <functional>

#ifdef PYTHON_ON

#include <Python.h>

void PyExec(const char* pyfile) {
  FILE* fd = fopen(pyfile, "r");
  Py_Initialize();
  PyRun_SimpleFile(fd, pyfile);
  Py_Finalize();
}

Option< std::function< void(const char*) > > pyexec = Option< std::function< void(const char*) > >::some(PyExec);

#else
Option< std::function< void(const char*) > > pyexec = Option< std::function< void(const char*) > >::none();

#endif

#ifdef JVM_ON

#include <jni.h>

void JvmExec(const char* java_class_name) {
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

Option< std::function< void(const char*) > > jvmexec = Option< std::function< void (const char*) > >::some(JvmExec);

#else
Option< std::function< void(const char*) > > jvmexec = Option< std::function< void (const char*) > >::none();

#endif

#ifdef R_ON

#include<Rembedded.h>
#include<Rinterface.h>

extern int R_running_as_main_program;

void RExec(const char* rfile) {
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

Option< std::function< void (const char*) > > rexec = Option< std::function< void (const char*) > >::some(RExec);

#else
Option< std::function< void (const char*) > > rexec = Option<std::function< void (const char*) > >::none();

#endif

#ifdef RUBY_ON

#include<ruby.h>

void RubyExec(const char* rbfile) {
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

Option< std::function< void (const char*) > > rubyexec = Option< std::function< void (const char*) > >::some(RubyExec);

#else
Option< std::function< void (const char*) > > rubyexec = Option< std::function< void (const char*) > >::none();

#endif

void CCExec(const char* ccfile) {
  execv(ccfile, NULL);
}

Option< std::function< void (const char*) > > ccexec = Option< std::function< void (const char*) > >::some(CCExec);

static std::map< std::string, Option< std::function< void (const char*) > > > embedded_exec_map = {
  { "python", pyexec },
  { "jvm", jvmexec },
  { "r", rexec },
  { "ruby", rubyexec },
  { "cc", ccexec }
};

bool papi_exec(
  const std::string& embedded_exec_str,
  const std::string& cmd,
  const std::vector<int>& events) {

  auto exec_itr = embedded_exec_map.find(embedded_exec_str);
  if(exec_itr != embedded_exec_map.end()) {
    perror("unable to find an interpreter in PapiExecutor supporting this application");
    return false;
  }

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

    if(exec_itr->second.isSome()) {
      exec_itr->second.get()(cmd.c_str());
    }
  }
  // parent
  else {
    const int eventscount = events.size();
    int papievents[eventscount];
    long long values[eventscount];

    for(int i = 0; i < events.size(); i++) {
      papievents[i] = events[i];
      values[i] = 0;
    }

    int papieventset = PAPI_NULL;
    int ok = PAPI_library_init(PAPI_VER_CURRENT);

    const PAPI_component_info_t* cmpinfo = PAPI_get_component_info( 0 );
    if(cmpinfo->attach == 0) { perror("no attaches\n"); }

    int status;
    int val = 1;

    pid_t child_pid = 0, w = 0;
    read(pfdsc[0], &child_pid, sizeof(pid_t)); // child fd

    ok = PAPI_create_eventset(&papieventset);
    if(ok != PAPI_OK) { perror("ERROR eventset-1\n"); }

    ok = PAPI_assign_eventset_component(papieventset, 0);
    if(ok != PAPI_OK) { perror("ERROR eventset-2\n"); }

    ok = PAPI_attach(papieventset, (unsigned long)child_pid);
    if(ok != PAPI_OK) { perror("ERROR attach\n"); }

    PAPI_add_events(papieventset, papievents, eventscount);

    PAPI_start(papieventset);

    write(pfdsp[1], &val, sizeof(int)); // parent fd

    do {
        w = waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
        if(w == -1) {
          perror("waitpid error");
          exit(EXIT_FAILURE);
        }
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));

    PAPI_stop(papieventset, values);
    PAPI_detach(papieventset);

    PAPI_shutdown();

    printf("results\n"); fflush(stdout);
    for(int i = 0; i < eventscount; i++) { printf("\t%d\t%lld\n", i, values[i]); }
  }

  return true;
}

static std::map<std::string, int> PAPI_FEATURES = {
{ "PAPI_L1_DCM", PAPI_L1_DCM }, /*Level 1 data cache misses */
{ "PAPI_L1_ICM", PAPI_L1_ICM }, /*Level 1 instruction cache misses */
{ "PAPI_L2_DCM", PAPI_L2_DCM }, /*Level 2 data cache misses */
{ "PAPI_L2_ICM", PAPI_L2_ICM }, /*Level 2 instruction cache misses */
{ "PAPI_L3_DCM", PAPI_L3_DCM }, /*Level 3 data cache misses */
{ "PAPI_L3_ICM", PAPI_L3_ICM }, /*Level 3 instruction cache misses */
{ "PAPI_L1_TCM", PAPI_L1_TCM }, /*Level 1 total cache misses */
{ "PAPI_L2_TCM", PAPI_L2_TCM }, /*Level 2 total cache misses */
{ "PAPI_L3_TCM", PAPI_L3_TCM }, /*Level 3 total cache misses */
{ "PAPI_CA_SNP", PAPI_CA_SNP }, /*Snoops */
{ "PAPI_CA_SHR", PAPI_CA_SHR }, /*Request for shared cache line (SMP) */
{ "PAPI_CA_CLN", PAPI_CA_CLN }, /*Request for clean cache line (SMP) */
{ "PAPI_CA_INV", PAPI_CA_INV }, /*Request for cache line Invalidation (SMP) */
{ "PAPI_CA_ITV", PAPI_CA_ITV }, /*Request for cache line Intervention (SMP) */
{ "PAPI_L3_LDM", PAPI_L3_LDM }, /*Level 3 load misses */
{ "PAPI_L3_STM", PAPI_L3_STM }, /*Level 3 store misses */
{ "PAPI_BRU_IDL", PAPI_BRU_IDL }, /*Cycles branch units are idle */
{ "PAPI_FXU_IDL", PAPI_FXU_IDL },     /*Cycles integer units are idle */
{ "PAPI_FPU_IDL", PAPI_FPU_IDL },     /*Cycles floating point units are idle */
{ "PAPI_LSU_IDL", PAPI_LSU_IDL },     /*Cycles load/store units are idle */
{ "PAPI_TLB_DM", PAPI_TLB_DM }, /*Data translation lookaside buffer misses */
{ "PAPI_TLB_IM", PAPI_TLB_IM }, /*Instr translation lookaside buffer misses */
{ "PAPI_TLB_TL", PAPI_TLB_TL }, /*Total translation lookaside buffer misses */
{ "PAPI_L1_LDM", PAPI_L1_LDM }, /*Level 1 load misses */
{ "PAPI_L1_STM", PAPI_L1_STM }, /*Level 1 store misses */
{ "PAPI_L2_LDM", PAPI_L2_LDM }, /*Level 2 load misses */
{ "PAPI_L2_STM", PAPI_L2_STM }, /*Level 2 store misses */
{ "PAPI_BTAC_M", PAPI_BTAC_M }, /*BTAC miss */
{ "PAPI_PRF_DM", PAPI_PRF_DM }, /*Prefetch data instruction caused a miss */
{ "PAPI_L3_DCH", PAPI_L3_DCH }, /*Level 3 Data Cache Hit */
{ "PAPI_TLB_SD", PAPI_TLB_SD }, /*Xlation lookaside buffer shootdowns (SMP) */
{ "PAPI_CSR_FAL", PAPI_CSR_FAL },     /*Failed store conditional instructions */
{ "PAPI_CSR_SUC", PAPI_CSR_SUC },   /*Successful store conditional instructions */
{ "PAPI_CSR_TOT", PAPI_CSR_TOT },     /*Total store conditional instructions */
{ "PAPI_MEM_SCY", PAPI_MEM_SCY },     /*Cycles Stalled Waiting for Memory Access */
{ "PAPI_MEM_RCY", PAPI_MEM_RCY },     /*Cycles Stalled Waiting for Memory Read */
{ "PAPI_MEM_WCY", PAPI_MEM_WCY },     /*Cycles Stalled Waiting for Memory Write */
{ "PAPI_STL_ICY", PAPI_STL_ICY },     /*Cycles with No Instruction Issue */
{ "PAPI_FUL_ICY", PAPI_FUL_ICY },     /*Cycles with Maximum Instruction Issue */
{ "PAPI_STL_CCY", PAPI_STL_CCY },     /*Cycles with No Instruction Completion */
{ "PAPI_FUL_CCY", PAPI_FUL_CCY },     /*Cycles with Maximum Instruction Completion */
{ "PAPI_HW_INT", PAPI_HW_INT }, /*Hardware interrupts */
{ "PAPI_BR_UCN", PAPI_BR_UCN }, /*Unconditional branch instructions executed */
{ "PAPI_BR_CN ", PAPI_BR_CN }, /*Conditional branch instructions executed */
{ "PAPI_BR_TKN", PAPI_BR_TKN }, /*Conditional branch instructions taken */
{ "PAPI_BR_NTK", PAPI_BR_NTK }, /*Conditional branch instructions not taken */
{ "PAPI_BR_MSP", PAPI_BR_MSP }, /*Conditional branch instructions mispred */
{ "PAPI_BR_PRC", PAPI_BR_PRC }, /*Conditional branch instructions corr. pred */
{ "PAPI_FMA_INS", PAPI_FMA_INS },     /*FMA instructions completed */
{ "PAPI_TOT_IIS", PAPI_TOT_IIS },     /*Total instructions issued */
{ "PAPI_TOT_INS", PAPI_TOT_INS },     /*Total instructions executed */
{ "PAPI_INT_INS", PAPI_INT_INS },     /*Integer instructions executed */
{ "PAPI_FP_INS", PAPI_FP_INS }, /*Floating point instructions executed */
{ "PAPI_LD_INS", PAPI_LD_INS }, /*Load instructions executed */
{ "PAPI_SR_INS", PAPI_SR_INS }, /*Store instructions executed */
{ "PAPI_BR_INS", PAPI_BR_INS }, /*Total branch instructions executed */
{ "PAPI_VEC_INS", PAPI_VEC_INS },     /*Vector/SIMD instructions executed (could include integer) */
{ "PAPI_RES_STL", PAPI_RES_STL },     /*Cycles processor is stalled on resource */
{ "PAPI_FP_STAL", PAPI_FP_STAL },     /*Cycles any FP units are stalled */
{ "PAPI_SYC_INS", PAPI_SYC_INS },     /*Sync. inst. executed */
{ "PAPI_L1_DCH", PAPI_L1_DCH }, /*L1 D Cache Hit */
{ "PAPI_L2_DCH", PAPI_L2_DCH }, /*L2 D Cache Hit */
{ "PAPI_L1_DCA", PAPI_L1_DCA }, /*L1 D Cache Access */
{ "PAPI_L2_DCA", PAPI_L2_DCA }, /*L2 D Cache Access */
{ "PAPI_L3_DCA", PAPI_L3_DCA }, /*L3 D Cache Access */
{ "PAPI_L1_DCR", PAPI_L1_DCR }, /*L1 D Cache Read */
{ "PAPI_L2_DCR", PAPI_L2_DCR }, /*L2 D Cache Read */
{ "PAPI_L3_DCR", PAPI_L3_DCR }, /*L3 D Cache Read */
{ "PAPI_L1_DCW", PAPI_L1_DCW }, /*L1 D Cache Write */
{ "PAPI_L2_DCW", PAPI_L2_DCW }, /*L2 D Cache Write */
{ "PAPI_L3_DCW", PAPI_L3_DCW }, /*L3 D Cache Write */
{ "PAPI_L1_ICH", PAPI_L1_ICH }, /*L1 instruction cache hits */
{ "PAPI_L2_ICH", PAPI_L2_ICH }, /*L2 instruction cache hits */
{ "PAPI_L3_ICH", PAPI_L3_ICH }, /*L3 instruction cache hits */
{ "PAPI_L1_ICA", PAPI_L1_ICA }, /*L1 instruction cache accesses */
{ "PAPI_L2_ICA", PAPI_L2_ICA }, /*L2 instruction cache accesses */
{ "PAPI_L3_ICA", PAPI_L3_ICA }, /*L3 instruction cache accesses */
{ "PAPI_L1_ICR", PAPI_L1_ICR }, /*L1 instruction cache reads */
{ "PAPI_L2_ICR", PAPI_L2_ICR }, /*L2 instruction cache reads */
{ "PAPI_L3_ICR", PAPI_L3_ICR }, /*L3 instruction cache reads */
{ "PAPI_L1_ICW", PAPI_L1_ICW }, /*L1 instruction cache writes */
{ "PAPI_L2_ICW", PAPI_L2_ICW }, /*L2 instruction cache writes */
{ "PAPI_L3_ICW", PAPI_L3_ICW }, /*L3 instruction cache writes */
{ "PAPI_L1_TCH", PAPI_L1_TCH }, /*L1 total cache hits */
{ "PAPI_L2_TCH", PAPI_L2_TCH }, /*L2 total cache hits */
{ "PAPI_L3_TCH", PAPI_L3_TCH }, /*L3 total cache hits */
{ "PAPI_L1_TCA", PAPI_L1_TCA }, /*L1 total cache accesses */
{ "PAPI_L2_TCA", PAPI_L2_TCA }, /*L2 total cache accesses */
{ "PAPI_L3_TCA", PAPI_L3_TCA }, /*L3 total cache accesses */
{ "PAPI_L1_TCR", PAPI_L1_TCR }, /*L1 total cache reads */
{ "PAPI_L2_TCR", PAPI_L2_TCR }, /*L2 total cache reads */
{ "PAPI_L3_TCR", PAPI_L3_TCR }, /*L3 total cache reads */
{ "PAPI_L1_TCW", PAPI_L1_TCW }, /*L1 total cache writes */
{ "PAPI_L2_TCW", PAPI_L2_TCW }, /*L2 total cache writes */
{ "PAPI_L3_TCW", PAPI_L3_TCW }, /*L3 total cache writes */
{ "PAPI_FML_INS", PAPI_FML_INS },     /*FM ins */
{ "PAPI_FAD_INS", PAPI_FAD_INS },     /*FA ins */
{ "PAPI_FDV_INS", PAPI_FDV_INS },     /*FD ins */
{ "PAPI_FSQ_INS", PAPI_FSQ_INS },     /*FSq ins */
{ "PAPI_FNV_INS", PAPI_FNV_INS },     /*Finv ins */
{ "PAPI_FP_OPS", PAPI_FP_OPS }, /*Floating point operations executed */
{ "PAPI_SP_OPS", PAPI_SP_OPS }, /* Floating point operations executed; optimized to count scaled single precision vector operations */
{ "PAPI_DP_OPS", PAPI_DP_OPS }, /* Floating point operations executed; optimized to count scaled double precision vector operations */
{ "PAPI_VEC_SP", PAPI_VEC_SP }, /* Single precision vector/SIMD instructions */
{ "PAPI_VEC_DP", PAPI_VEC_DP }, /* Double precision vector/SIMD instructions */
{ "PAPI_END", PAPI_END } };    /*This should always be last! */
//{ "PAPI_REF_CYC", PAPI_REF_CYC},  /* Reference clock cycles */

int get_papi_event(const std::string& label) {
  auto itr = PAPI_FEATURES.find(label);
  return (itr != PAPI_FEATURES.end()) ? itr->second : -1;
}
