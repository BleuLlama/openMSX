
SHAREDIR := $(HOME)/.openMSX/share/
RCROMS   := $(SHAREDIR)/systemroms/machines/rc2014/
RCMACH	 := $(SHAREDIR)/machines/

OPENMSX := $(HOME)/.openMSX/git/derived/x86_64-darwin-opt/bin/openmsx

all: run

run: copyfiles
	@echo == Running rc emulation
	$(OPENMSX) -machine rc2014 -control stdio 


copyfiles:
	@echo == Copying roms to $(SHAREDIR)
	mkdir -p $(RCROMS)
	cp roms/* $(RCROMS)
	mkdir -p $(RCMACH)
	cp machines/* $(RCMACH)

