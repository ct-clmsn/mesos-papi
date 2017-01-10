#include "PapiExecutor.hpp"
#include <mesos/executor.hpp>

int main(int argc, char** argv) {
  PapiExecutor executor;
  mesos::MesosExecutorDriver driver(&executor);
  return driver.run() == mesos::Status::DRIVER_STOPPED ? 0 : 1;
}
