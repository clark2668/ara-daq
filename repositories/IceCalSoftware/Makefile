# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk

Targets = icecald atatd
icecald : OBJECTS = icecald.o icecal.o ace2.o atriComponents.o icecali2c.o icecalace2.o
atatd : OBJECTS = atatd.o atat.o
all: $(Targets)

icecald : icecald.o icecal.o ace2.o atriComponents.o icecali2c.o icecalace2.o
	@echo "<**Linking**> $@ (from $(OBJECTS)) ..."
	$(LD) $(OBJECTS) $(LDFLAGS) $(ARA_LIBS) -lARAutil -lAraRunControl -lAtriControl -lAraFx2Com  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

atatd : atatd.o atat.o
	@echo "<**Linking**> $@ (from $(OBJECTS)) ..."
	$(LD) $(OBJECTS) $(LDFLAGS) $(ARA_LIBS) -lARAutil -lAraRunControl -lAtriControl -lAraFx2Com  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















