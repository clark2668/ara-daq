#
#
#

ARA_LIB_DIR=${ARA_DAQ_DIR}/lib
ARA_BIN_DIR=${ARA_DAQ_DIR}/bin

CC            = gcc
LD            = gcc
ObjSuf	      = o
SrcSuf        = c
ExeSuf        =
OPT           =  -Wall --debug -g # --debug --pedantic-errors
SOFLAGS       = -shared
DllSuf        = so

#Hack so that the software compiles on MAC OS X
SYSTEM:= $(shell uname -s)
CHIP:= $(shell uname -m)      # only used by Solaris at this time...

ifeq ($(ARCH),macosx)
# MacOS X with cc (GNU cc 2.95.2 and gcc 3.3)
CXX           = gcc
CXXFLAGS      = $(OPT2) -pipe -Wall -W -Woverloaded-virtual
LD            = gcc
LDFLAGS       = $(OPT2) -mmacosx-version-min=$(MACOSXTARGET)
# The SOFLAGS will be used to create the .dylib,
# the .so will be created separately
ifeq ($(subst $(MACOSX_MINOR),,1234),1234)
DllSuf        = so
else
DllSuf        = dylib
endif
UNDEFOPT      = dynamic_lookup
ifneq ($(subst $(MACOSX_MINOR),,12),12)
UNDEFOPT      = suppress
LD            = gcc
endif
SOFLAGS       = -dynamiclib -single_module -undefined $(UNDEFOPT) -install_name $(CURDIR)/
endif





NOOPT         =
EXCEPTION     = 

#Toggle as necessary
PROFILER=
#PROFILER=-lprofiler -ltcmalloc

ifdef USE_FAKE_DATA_DIR
FAKEFLAG = -DUSE_FAKE_DATA_DIR
else
FAKEFLAG =
endif

SYS_LIBS = -lz


INCLUDES     = -I$(ARA_DAQ_DIR) -I$(ARA_DAQ_DIR)/includes -I$(ARA_DAQ_DIR)/common -I/usr/local/include/libusb-1.0 -I/usr/include/libusb-1.0/  #-I/usr/local/include/ARA
CCFLAGS      = $(EXCEPTION) $(OPT) -fPIC $(INCLUDES) $(FAKEFLAG) $(SYSCCFLAGS)
LDFLAGS      = $(EXCEPTION)  $(SYS_LIBS) -L$(ARA_LIB_DIR)  
ARA_LIBS     =  -lm -lz $(PROFILER)
BZ_LIB =  -lbz2
FFTW_LIB =  -lfftw3

INSTALL=install
GROUP=staff
OWNER=root
BINDIR=/usr/local/bin
LIBDIR=/usr/local/lib

all: $(Target)

%.$(ObjSuf) : %.$(SrcSuf) Makefile
	@echo "<**Compiling****> "$<
	$(CC) $(CCFLAGS) -c $< -o  $@

objclean:
	@-rm -f *.o




















