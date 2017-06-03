#
# ARA utility library
#

include $(ARA_DAQ_DIR)/standard_definitions.mk

LIB_OBJS         =  util.o

Name = libARAutil
Library  = $(ARA_LIB_DIR)/$(Name).a
DynLib  = $(ARA_LIB_DIR)/$(Name).${DllSuf}


all: $(Library) $(DynLib)

$(Library): $(LIB_OBJS)
	@/bin/rm -f $(Library)	
	@echo "Creating $(Library) ..."
	ar -r $@ $^

$(DynLib): $(LIB_OBJS)
	@/bin/rm -f $(DynLib)	
	@echo "Creating $(DynLib) ..."
	$(LD) $(LIB_OBJS) $(LIBS) $(LDFLAGS) $(SOFLAGS) -o $(DynLib)
	@chmod 555 $(DynLib)

clean: objclean
	@/bin/rm -f $(Library) $(DynLib)





