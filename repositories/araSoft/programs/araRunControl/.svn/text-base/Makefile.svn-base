# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk



Targets = araRunControl startNewRun startPedestalRun startNewRunFromCommandLine stopRun stopRunFromCommandLine checkRunIsDone


all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) -lAraRunControl -lARAutil -lreadline $(ARA_LIBS) -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ARA_DAQ_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















