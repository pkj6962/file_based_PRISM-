.PHONY: all clean
.DEFAULT_GOAL := all

LIBS=-lrt -lm -lstdc++fs -lssl -lcrypto -llustreapi -lpthread
INCLUDES=-./include
CFLAGS=-std=c++17 -O0 -g -fpermissive

MPICXX=mpicxx

output = fb_danzer_obj

all: main

main: mpitracer.cc master.cc 
	$(MPICXX) $(CFLAGS) -o fb_danzer_obj mpitracer.cc master.cc $(LIBS)

clean:
	rm $(output)
