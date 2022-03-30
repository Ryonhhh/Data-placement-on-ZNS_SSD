CC      =g++
CFLAGS  =-g -Wall -static
LDFLAGS =-L /home/wls/Data-placement-on-ZNS_SSD/libzbd-2.0.3/lib/.libs/
LIBS    =-lzbd
OBJS    =main.o zns_controller.o
TARGET  =main

all: main

main: main.cc zns_controller.cc zns_controller.h
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)


clean:
	rm -f ./*.o
	rm -f ./main
