# Makefile wrapper for waf

all:
	./waf -j4

# free free to change this part to suit your requirements
configure:
	./waf configure --with-ofswitch13=/mnt/hgfs/luciano/Documents/Unicamp/doutorado/codigos/ofsoftswitch13/
