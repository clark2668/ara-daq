#
# ARA utility library
#

include $(ARA_DAQ_DIR)/standard_definitions.mk

LIB_OBJS         =  atriControl.o

Name = libAtriControl
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
	$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(LIB_OBJS) -lusb-1.0 -o $(DynLib)
	@chmod 555 $(DynLib)

clean: objclean
	@/bin/rm -f $(Library) $(DynLib)





