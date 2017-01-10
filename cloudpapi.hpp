#ifndef __CLOUDPAPI__
#define __CLOUDPAPI__ 1

#include <string>
#include <vector>

#include <papi.h>

#ifdef PYTHON_ON

#include <Python.h>
void PyExec(const char* pyfile);

#endif

#ifdef JVM_ON

#include <jni.h>
void JvmExec(const char* java_class_name);

#endif

#ifdef R_ON

#include<Rembedded.h>
#include<Rinterface.h>
extern int R_running_as_main_program;
void RExec(const char* rfile);

#endif

#ifdef RUBY_ON

#include<ruby.h>
void RubyExec(const char* rbfile);

#endif

void CCExec(const char* ccfile);

bool papi_exec(
  const std::string& embedded_exec_str,
  const std::string& cmd,
  const std::vector<int>& events);

int get_papi_event(const std::string& label);

#endif
