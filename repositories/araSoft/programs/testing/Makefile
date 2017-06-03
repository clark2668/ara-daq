# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk

CPPFLAGS=$(CCFLAGS)

Targets = sendAtriControlPacket sendFx2ControlPacket atriGetDaughters \
	  atriInitializeDaughters atriSetVdly atriSetThresholds atriSetVadj \
	  atriReadThresholdScalars atriDoThresholdScan dbWriteIdentify \
	  wbw wbr dbi2cr dbi2cw atriDoSurfaceThresholdScan \
	  atriReadWilkinsonSpeed atriSetReadoutDelay fxprogram \
          atriReadEventStatistics 



dbWriteIdentify : LD=g++

all: $(Targets)

$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o $(LDFLAGS) -lARAutil -lAraRunControl -lAtriControl -lAraFx2Com  -lAraAtriCom -lARAkvp -lAraSoftConfig $(ARA_LIBS) -I${ARA_DAQ_DIR}/programs/ARAAcqd -lusb-1.0 -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















