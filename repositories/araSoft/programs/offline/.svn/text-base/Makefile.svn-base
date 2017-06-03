# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk



Targets = fakeEventData


all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o $(LDFLAGS) $(ARA_LIBS) -lusb-1.0 -lARAutil -lAraRunControl -lAtriControl -lAraFx2Com -lAraAtriCom  -lAraSoftConfig -lARAkvp -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















