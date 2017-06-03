# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk



Targets = xvcd xvcd_sa

all: $(Targets)


xvcd: % : xvcd.o fx2jtag.o
	@echo "<**Linking**> $@ ..."
	$(LD) $^ $(LDFLAGS) $(ARA_LIBS) -lusb-1.0 -lARAutil -lAraRunControl -lAtriControl -lAraFx2Com -lAraAtriCom  -lAraSoftConfig -lARAkvp -lusb-1.0 -lz -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

xvcd_sa: % : xvcd.o fx2jtag.o fx2standalone10.o
	@echo "<**Linking**> $@ ..."
	$(LD) $^ $(LDFLAGS) -lusb-1.0 -o $@

clean: objclean
	@-rm -f $(Targets) 




















