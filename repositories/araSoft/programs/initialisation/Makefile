
#Jonathan's first makefile

FpgaProg: FpgaProg.o usbInterface.o

	g++  FpgaProg.o usbInterface.o -o FpgaProg -L/usr/lib/ -lusb-1.0

FpgaProg.o: FpgaProg.cxx
	g++  -c FpgaProg.cxx -I/usr/include/libusb-1.0/

usbInterface.o: usbInterface.h usbInterface.cxx
	g++  -c usbInterface.cxx -I/usr/include/libusb-1.0/	

install:
	install FpgaProg ${ARA_DAQ_DIR}/programs/initialisation


clean: 
	@rm -f *.o
