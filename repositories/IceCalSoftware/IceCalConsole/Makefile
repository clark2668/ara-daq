CC = g++
DEBUG = -g
CFLAGS = -Wall -Werror -ansi -pedantic -c
LFLAGS = -Wall

# path to the include directory for icecalapi.h
I_ICECALAPI =-I../IceCalAPI/

# path to the shared library directory for libicecal.so
L_ICECALAPI =-L../IceCalAPI/ -licecal

all:
	$(CC) $(CFLAGS) $(I_ICECALAPI) IceCalConsole.cpp -o IceCalConsole.o
	$(CC) $(LFLAGS) $(L_ICECALAPI) IceCalConsole.o -o IceCalCLI 

clean:
	rm IceCalCLI
	rm IceCalConsole.o