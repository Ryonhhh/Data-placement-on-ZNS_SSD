CC      =g++
CFLAGS  =-g -Wall -static
LDFLAGS =-L /home/wht/Data-placement-on-ZNS_SSD/libzbd-2.0.3/lib/.libs/
LIBS    =-lzbd
OBJS    =main.o zns_controller.o workload.o zns_simulation.o
TARGET  =main

all: main

main: main.cc zns_controller.cc workload.cc zns_simulation.cc
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)


clean:
	rm -f ./*.o
	rm -f ./main
	rm -f ./zns_controller
