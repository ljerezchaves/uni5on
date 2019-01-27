# Custom Makefile for SVELTE

all:
	./waf

clean: lib-clean sim-clean

debug: lib-debug sim-config-debug sim-build

optmized: lib-optmz sim-config-optimized sim-build

lib-optimized: lib-clean lib-config-optimized lib-build

lib-debug: lib-clean lib-config-debug lib-build

lib-build:
	rm -f build/lib/libns3*-ofswitch13-*.so
	cd ofsoftswitch13-gtp && make -j4

lib-clean:
	cd ofsoftswitch13-gtp && git clean -fxd

lib-config-optimized:
	cd ofsoftswitch13-gtp && ./boot.sh && ./configure --enable-ns3-lib

lib-config-debug:
	cd ofsoftswitch13-gtp && ./boot.sh && ./configure --enable-ns3-lib CFLAGS='-g -O0' CXXFLAGS='-g -O0'

sim-build:
	./waf

sim-clean:
	./waf distclean

sim-config-optimized:
	./waf configure -d optimized --with-ofswitch13=./ofsoftswitch13-gtp/

sim-config-debug:
	./waf configure -d debug --with-ofswitch13=./ofsoftswitch13-gtp/ --enable-examples --enable-tests

sim-pull:
	git pull --recurse-submodules && git submodule update --recursive

