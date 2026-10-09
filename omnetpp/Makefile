# Build the simulation (needs OMNeT++ 6.x in the PATH: source <omnetpp>/setenv)
all: src/Makefile
	$(MAKE) -C src MODE=release

src/Makefile:
	cd src && opp_makemake -f --deep -o wsn -O out -I.

clean:
	-$(MAKE) -C src clean MODE=release
	rm -f src/Makefile

.PHONY: all clean
