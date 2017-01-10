#ifndef __PAPIMESOSEXECUTOR__
#define __PAPIMESOSEXECUTOR__ 1

#include <string>
#include <vector>

#include <mesos/executor.hpp>

#include "cloudpapi.hpp"

class PapiExecutor : public mesos::Executor {

  public:
    PapiExecutor() {}
    virtual ~PapiExecutor() {}

    virtual void registered(
      mesos::ExecutorDriver* driver,
      const mesos::ExecutorInfo& executorInfo,
      const mesos::FrameworkInfo& frameworkInfo,
      const mesos::SlaveInfo& slaveInfo) {
    }

    virtual void reregistered(
      mesos::ExecutorDriver* driver,
      const mesos::SlaveInfo& slaveInfo) {
    }

    virtual void disconnected(
      mesos::ExecutorDriver* driver) {
    }

    virtual void launchTask(
      mesos::ExecutorDriver* driver,
      const mesos::TaskInfo& task) {

      if(!task.has_labels()) {
        perror("task has no labels associated with it");
      }

      const mesos::Labels labels = task.labels();
      const mesos::CommandInfo ci = task.command();

      std::string papiexec;
      std::vector<int> papi_events;

      for(int i = 0; i < labels.labels_size(); i++) {
        std::string label = labels.labels().Get(i).key();
        std::string value = labels.labels().Get(i).value();

        if(label == "papiexec") {
          papiexec = value;
        }

        if(label.find("PAPI") != std::string::npos) {
          papi_events.push_back(get_papi_event(label));
        }
      }

      std::string cmd_str = ci.value();

      for(int i = 0; i < ci.arguments_size(); i++) {
        cmd_str += " " + ci.arguments(i);
      }

      papi_exec(papiexec, cmd_str, papi_events);
    }

    virtual void killTask(
      mesos::ExecutorDriver* driver,
      const mesos::TaskID& taskId) {
    }

    virtual void frameworkMessage(
      mesos::ExecutorDriver* driver,
      const std::string& data) {
    }

    virtual void shutdown(
      mesos::ExecutorDriver* driver) {
    }

    virtual void error(
      mesos::ExecutorDriver* driver,
      const std::string& message) {
    }

};

#endif
