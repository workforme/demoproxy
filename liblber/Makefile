SYSTYPE:=`uname -s`
default: prebuild

prebuild:
	if [ $(SYSTYPE) = 'FreeBSD' ]; then\
		make -f makefile.bsd;\
	else\
		make -f makefile.linux;\
	fi;\

clean:
	rm -f *.o liblber.a etest dtest

.PHONY: default prebuild clean
