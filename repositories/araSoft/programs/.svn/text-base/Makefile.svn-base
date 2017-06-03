# 
# ARA Programs
#

include ${ARA_DAQ_DIR}/standard_definitions.mk

SUBDIRS = ARAd araRunControl ARAAcqd testing initialisation

all: subdirs

subdirs:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make ) ; \
	done

clean:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make clean ) ; \
	done
