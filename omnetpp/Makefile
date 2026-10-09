# Build the simulation (needs OMNeT++ 6.x in the PATH: source <omnetpp>/setenv)
# The src/Makefile is regenerated every time, so new source folders are always picked up.
all:
	cd src && opp_makemake -f --deep -o wsn -O out -I.
	$(MAKE) -C src MODE=release

clean:
	-$(MAKE) -C src clean MODE=release
	rm -f src/Makefile

.PHONY: all clean
