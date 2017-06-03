CC = g++
CFLAGS = -Wall -Werror -ansi -pedantic -c

all: icecalapi

icecalapi: icecalapi.o
	$(CC) -shared -Wl,-soname,libicecal.so.1 -o libicecal.so.1.0 icecalapi.o
	ln -sf libicecal.so.1.0 libicecal.so
	ln -sf libicecal.so.1.0 libicecal.so.1

icecalapi.o: icecalapi.cpp icecalapi.h
	$(CC) $(CFLAGS) -fPIC icecalapi.cpp

clean:
	rm libicecal.so.1.0
	rm libicecal.so.1
	rm libicecal.so
	rm icecalapi.o