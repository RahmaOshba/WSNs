# Build the packet-level simulation (needs OMNeT++ and INET 4.6:
#   source <omnetpp>/setenv ; source <inet>/setenv   -> INET_ROOT)
all:
	@test -n "$(INET_ROOT)" || (echo "INET_ROOT is not set: source <inet>/setenv first" && false)
	cd src && opp_makemake -f --deep --make-so -o pktwsn -O out -I. -I$(INET_ROOT)/src -L$(INET_ROOT)/src -lINET -KINET_PROJ=$(INET_ROOT)
	$(MAKE) -C src MODE=release

clean:
	-$(MAKE) -C src clean MODE=release
	rm -f src/Makefile

.PHONY: all clean
