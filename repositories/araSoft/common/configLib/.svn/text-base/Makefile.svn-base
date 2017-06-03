#-------------------------------------------------------------------------
# Makefile for config library and test programs
#
# Author : G.J.Crone
# Stolen by : R. Nichol
# Pilfered by : P. Miocinovic, M2 Consulting
#
#-------------------------------------------------------------------------

include ${ARA_DAQ_DIR}/standard_definitions.mk

LIB_OBJS =  configLoad.o configStore.o configError.o configFileSpec.o

Name = libAraSoftConfig
DynLib  = $(ARA_LIB_DIR)/$(Name).${DllSuf}
Library = $(ARA_LIB_DIR)/$(Name).a

all: $(Library) $(DynLib)

$(Library): $(LIB_OBJS)
	@/bin/rm -f $(Library)	
	@echo "Linking $(Library) ..."
	@ar -r $@ $^

$(DynLib): $(LIB_OBJS)
	@/bin/rm -f $(DynLib)	
	@echo "Creating $(DynLib) ..."
	@$(LD) $(LDFLAGS) $(SYS_LIBS) $(SOFLAGS) $(LIB_OBJS) -lARAkvp -o $(DynLib)
	@chmod 555 $(DynLib)

clean: objclean
	@/bin/rm -f $(Library) $(DynLib)

