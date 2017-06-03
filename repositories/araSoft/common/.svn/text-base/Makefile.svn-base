# 
# ARA utility libraries
#

include ${ARA_DAQ_DIR}/standard_definitions.mk

SUBDIRS =utilLib kvpLib configLib araRunControlLib atriControlLib fx2ComLib atriComLib araRunLogLib calpulserLib

all: subdirs

subdirs:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make ) ; \
	done

clean:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make clean ) ; \
	done
