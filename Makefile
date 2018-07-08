# Custom Makefile for SVELTE

all:
	./waf

configure:
	cd ofsoftswitch13-gtp; ./boot.sh; ./configure --enable-ns3-lib; make
	./waf configure --with-ofswitch13=./ofsoftswitch13-gtp/
