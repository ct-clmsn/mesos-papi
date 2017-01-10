MESOS_HOME=
MESOS_3PTY_HOME=$(MESOS_HOME)/3rdparty
MESOS_FLAGS=$(shell pkg-config mesos --cflags --libs) -I$(MESOS_3PTY_HOME)/glog-0.3.3/src/ -L$(MESOS_3PTY_HOME)/glog-0.3.3/ -L$(MESOS_3PTY_HOME)/libprocess -I$(MESOS_3PTY_HOME)/libprocess/include -I$(MESOS_3PTY_HOME)/stout/include -I$(MESOS_3PTY_HOME)/picojson-1.3.0

PYTHONCFLAGS=-I/usr/include/python2.6
JVMCFLAGS=-I/usr/local/java/include -I/usr/local/java/include/linux/ -L/usr/local/java/jre/lib/amd64/server/
RCFLAGS=-I$(HOME)/opt/lib64/R/include/
RUBYCFLAGS=-I$(HOME)/opt/include/ruby-2.2.0

RTSUPPORT=-DPYTHON_ON=1# -DJVM_ON=1 -DR_ON=1 -DRUBY_ON=1

cloudpapi.o:
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(PYTHONCFLAGS) $(JVMFLAGS) $(RCFLAGS) $(RUBYCFLAGS) $(MESOS_FLAGS) cloudpapi.cpp -c $(RTSUPPORT)

pycloudpapi.o:
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(PYTHONCFLAGS) $(MESOS_FLAGS) cloudpapi.cpp -c -DPYTHON_ON=1

jvmcloudpapi.o:
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(JVMCFLAGS) $(MESOS_FLAGS) cloudpapi.cpp -c -DJVM_ON=1

rcloudpapi.o:
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(RCFLAGS) $(MESOS_FLAGS) cloudpapi.cpp -c -DR_ON=1

rbcloudpapi.o:
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(RUBYCFLAGS) $(MESOS_FLAGS) cloudpapi.cpp -c -DRUBY_ON=1

papicc: cloudpapi.o
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) cloudpapi.o papicc.cpp -o papicc

papijvm: jvmcloudpapi.o
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) cloudpapi.o papijvm.cpp -o papijvm -lpapi -ljvm -lgmp -lcrypt

papir: rcloudpapi.o
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) cloudpapi.o papir.cpp -o papir -lpapi -lR -lRblas -lgmp -lcrypt

papirb: rbcloudpapi.o
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) cloudpapi.o papirb.cpp -o papirb -lpapi -lruby-static -lgmp -lcrypt

papipy: pycloudpapi.o
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) $(PYTHONCFLAGS) cloudpapi.o papipy.cpp -o papipy -lpapi -lpython2.6 -lgmp -lcrypt -DPYTHON_ON=1

PapiExecutor.o:
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) cloudpapi.o PapiExecutor.cpp -c

all: cloudpapi.o PapiExecutor.o
	g++ -std=c++11 -I$(HOME)/opt/include -L$(HOME)/opt/lib $(MESOS_FLAGS) cloudpapi.o PapiExecutor.o papiexec.cpp -o papiexec -lmesos -lpapi -lpython2.6 -lgmp -lcrypt

clean:
	rm papiexec *.o
	rm papipy papijvm papir papirb
