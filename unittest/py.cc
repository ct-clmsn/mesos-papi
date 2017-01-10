#include<stdio.h>
#include<Python.h>

int main(int argc, char** argv) {
  Py_Initialize();
  FILE* fd = fopen(argv[1], "r");
  PyRun_SimpleFile(fd, argv[1]);
  Py_Finalize();
  return 0;
}
