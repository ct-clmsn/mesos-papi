#include "cloudpapi.hpp"

#include <vector>
#include <string>
#include <string.h>

const std::vector< std::string > filenameopt = { "--filename", "-f", "--performancecounter", "-pc" };

int main(int argc, char** argv) {
  std::string filename;
  std::vector<int> perfcounter;

  if(argc == 1) {
    perror("papipy requires --filename and atleast 1 --performancecounter");
    exit(1);
  }

  for(int i = 0; i < argc; i++) {
    if(strncmp(argv[i], filenameopt[0].c_str(), filenameopt[0].size()) == 0) {
      filename = argv[i+1];
    }
    else if(strncmp(argv[i], filenameopt[1].c_str(), filenameopt[1].size()) == 0) {
      filename = argv[i+1];
    }
    else if(strncmp(argv[i], classnameopt[2].c_str(), classnameopt[2].size()) == 0) {
      perfcounter.push_back(get_papi_event(atoi(argv[i+1])));
    }
    else if(strncmp(argv[i], classnameopt[3].c_str(), classnameopt[3].size()) == 0) {
      perfcounter.push_back(get_papi_event(atoi(argv[i+1])));
    }
  }
  
  papi_exec("python", classname.c_str(), perfcounter);
  return 1;
}

