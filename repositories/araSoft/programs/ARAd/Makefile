# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk



Targets = ARAd


all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o $(LDFLAGS) $(ARA_LIBS) -lpthread -lARAutil -lAraRunControl -lAraSoftConfig -lARAkvp -lz -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















