CC      =g++
CFLAGS  =-g -Wall
LDFLAGS =-L/usr/lib/libzbd.so
LIBS    =-lzbd
OBJS    =main.o zns_controller.o
TARGET=Simulation

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

main.o:zns_controller.h main.cc
zns_controller.o:zns_controller.cc zns_controller.h

all: $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)
