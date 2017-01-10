#include <jni.h>

int main(int argc, char** argv) {
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

  jclass cls = env->FindClass("FloatLoop");
  // [L -> array of strings
  jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");

  env->CallStaticVoidMethod(cls, mid, "");

  jvm->DestroyJavaVM();
}
