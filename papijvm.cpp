#include "cloudpapi.hpp"

#include <vector>
#include <string>
#include <string.h>

const std::vector< std::string > classnameopt = { "--classname", "-c", "--performancecounter", "-pc" };

int main(int argc, char** argv) {
  std::string classname;
  std::vector<int> perfcounter;

  if(argc == 1) {
    perror("papijvm requires --classname and atleast 1 --performancecounter");
    exit(1);
  }

  for(int i = 0; i < argc, i++) {
    if(strncmp(argv[i], classnameopt[0].c_str(), classnameopt[0].size()) == 0) {
      classname = argv[i+1];
    }
    else if(strncmp(argv[i], classnameopt[1].c_str(), classnameopt[1].size()) == 0) {
      classname = argv[i+1];
    }
    else if(strncmp(argv[i], classnameopt[2].c_str(), classnameopt[2].size()) == 0) {
      perfcounter.push_back(get_papi_event(atoi(argv[i+1])));
    }
    else if(strncmp(argv[i], classnameopt[3].c_str(), classnameopt[3].size()) == 0) {
      perfcounter.push_back(get_papi_event(atoi(argv[i+1])));
    }
  }
  
  papi_exec("jvm", classname.c_str(), perfcounter);
  return 1;
}

