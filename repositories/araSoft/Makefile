# 
#
#

include ${ARA_DAQ_DIR}/standard_definitions.mk

SUBDIRS = common programs

ARA_INSTALL_LIBS=libARAkvp.a libARAkvp.so  libARAutil.a libARAutil.so libAraAtriCom.so libAraAtriCom.a libAraFx2Com.so libAraFx2Com.a libAraRunControl.a libAraRunControl.so libAraSoftConfig.a libAraSoftConfig.so libAtriControl.a libAtriControl.so libAraRunLog.a libAraRunLog.so

ARA_INSTALL_BINS=ARAd ARAAcqd 

all: subdirs

#configDocs:
#	@script/makeHtmledConfigFiles.sh

#docs: configDocs
#	@doxygen doc/doxy.config

subdirs:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make ) ; \
	done

clean:
	@for aDir in $(SUBDIRS); do \
	( cd $$aDir ; make clean ) ; \
	done


#scripts:
#	@for file in ${ANITA_FLIGHT_SOFT_DIR}/script/*[^~]; do \
#	( chmod a+x $$file; ln -sf $$file bin; ) ; \
#	done	

install:
	@for aBin in $(ARA_INSTALL_BINS); do \
	( echo "Installing $(BINDIR)/$$aBin"; $(INSTALL) -c -o $(OWNER) -g $(GROUP) -m 755 bin/$$aBin $(BINDIR) ) ; \
	done

	@for aLib in $(ARA_INSTALL_LIBS); do \
	( echo "Installing $(LIBDIR)/$$aLib"; $(INSTALL) -c -o $(OWNER) -g $(GROUP) -m 755 lib/$$aLib $(LIBDIR) ) ; \
	done

uninstall:
	@for aBin in $(ARA_INSTALL_BINS); do \
	( echo "Deleting $(BINDIR)/$$aBin"; rm -f $(BINDIR)/$$aBin ) ; \
	done

	@for aLib in $(ARA_INSTALL_LIBS); do \
	( echo "Deleting $(LIBDIR)/$$aLib"; rm -f $(LIBDIR)/$$aLib ) ; \
	done











