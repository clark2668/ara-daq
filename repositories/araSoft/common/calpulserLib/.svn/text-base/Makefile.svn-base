#
# ARA utility library
#

include $(ARA_DAQ_DIR)/standard_definitions.mk

LIB_OBJS         =  calpulser.o icecal.o ace2.o icecalace2.o icecali2c.o

Name = libAraCalPulser
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





