SYSTYPE:=`uname -s`
LOCAL_SRC:=`pwd`
EXTRA_LIB_DIR="`pwd`/extra-lib"


all: prebuild

prebuild:
	if [ -d extra-lib ]; then \
		echo "extra-lib exist"; \
	else \
		mkdir extra-lib; \
		if [ $? != 0 ]; then \
			echo "mkdir extra-lib failed"; \
			exit 1; \
		fi; \
	fi;\
	tar zxvf libevent-2.0.20-stable.tar.gz
	echo $(EXTRA_LIB_DIR)
	cd libevent-2.0.20-stable && ./configure --prefix=$(EXTRA_LIB_DIR) && make && make install
	cp -r libevent-2.0.20-stable/extra-lib/ ./extra-lib
	if [ $(SYSTYPE) = 'FreeBSD' ]; then\
		make -f makefile.bsd;\
	else\
		make -f makefile.linux;\
	fi;\

clean:
	rm -f *.o ldapproxy test
	@(cd ./libstk; make clean;)
	@cd ./liblber; make clean;
	@cd ./confparser; if [ $(SYSTYPE) = 'FreeBSD' ]; then make -f freebsd.mk clean; else make -f linux.mk clean; fi;

.PHONY: all prebuild clean
