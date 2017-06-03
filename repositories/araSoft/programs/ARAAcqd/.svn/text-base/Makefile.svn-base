# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk



Targets = ARAAcqd


all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o $(LDFLAGS) $(ARA_LIBS) -lusb-1.0 -lARAutil -lAraRunControl -lAraRunLog -lAtriControl -lAraFx2Com -lAraAtriCom -lAraSoftConfig -lARAkvp -lAraCalPulser -lusb-1.0 -lz -lpthread -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















