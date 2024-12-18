
EMBEDDED_IRX = 0
IOP_RESET = 1

BIN2C = $(PS2SDK)/bin/bin2c

SRC_DIRS = src src/cpu src/debug src/dos src/fpu src/gui src/hardware src/hardware/serialport \
		   src/hardware/mame src/ints src/misc src/shell

EE_BIN 	= PS2DOSBOX.ELF

FLAGS = -g -O2 -Wno-unused-variable -I./include -D__mips_eabi -DFPU_FLOAT

SRC_FILES_C = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
SRC_FILES_CPP = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))

EE_INCS += -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ports/include -I./src/platform/ps2
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ports/lib -L. -Xlinker -Map=output.map
EE_LIBS = -lgskit -ldmakit -laudsrv -lpad -lpatches -lhdd -lfileXio -lpoweroff -lmouse -lkbd -lm -lc

IRXS = iomanX_irx.o fileXio_irx.o usbhdfsd_irx.o usbd_irx.o libsd_irx.o audsrv_irx.o poweroff_irx.o ps2dev9_irx.o ps2atad_irx.o ps2hdd_irx.o ps2fs_irx.o

EE_OBJS = $(SRC_FILES_CPP:.cpp=.o)

ifeq ($(EMBEDDED_IRX), 1)
EE_OBJS += $(IRXS)
FLAGS += -DEMBEDDED_IRX
endif

ifeq ($(IOP_RESET), 1)
FLAGS += -DIOP_RESET
endif

EE_CFLAGS += $(FLAGS)
EE_CXXFLAGS += $(FLAGS)
			
all: $(EE_BIN)

_install: pack
	mkdir -p bin
	cp -f dosbox.conf bin
	cp -f $(EE_BIN:.ELF=-PACKED.ELF) ./bin/
	cp -f $(EE_BIN) ./bin/
ifeq ($(EMBEDDED_IRX), 0)
	cp -f $(PS2SDK)/iop/irx/iomanX.irx ./bin/IOMANX.IRX
	cp -f $(PS2SDK)/iop/irx/fileXio.irx ./bin/FILEXIO.IRX
	cp -f $(PS2SDK)/iop/irx/usbd.irx ./bin/USBD.IRX
	cp -f $(PS2SDK)/iop/irx/usbhdfsd.irx ./bin/USBHDFSD.IRX
	cp -f $(PS2SDK)/iop/irx/freesd.irx ./bin/FREESD.IRX
	cp -f $(PS2SDK)/iop/irx/audsrv.irx ./bin/AUDSRV.IRX
	cp -f $(PS2SDK)/iop/irx/ps2kbd.irx ./bin/PS2KBD.IRX
	cp -f $(PS2SDK)/iop/irx/ps2mouse.irx ./bin/PS2MOUSE.IRX
	cp -f $(PS2SDK)/iop/irx/poweroff.irx ./bin/POWEROFF.IRX
	cp -f $(PS2SDK)/iop/irx/ps2dev9.irx ./bin/PS2DEV9.IRX
	cp -f $(PS2SDK)/iop/irx/ps2atad.irx ./bin/PS2ATAD.IRX
	cp -f $(PS2SDK)/iop/irx/ps2hdd.irx ./bin/PS2HDD.IRX
	cp -f $(PS2SDK)/iop/irx/ps2fs.irx ./bin/PS2FS.IRX
endif
	
%_irx.c: $(PS2SDK)/iop/irx/%.irx
	$(BIN2C) $^ $*_irx.c $*_irx

pack:
	ps2-packer-lite $(EE_BIN) $(EE_BIN:.ELF=-PACKED.ELF)
run:
	ps2client -h 192.168.1.110 execee host:$(EE_BIN)
reset:
	ps2client -h 192.168.1.110 reset
listen:
	ps2client -h 192.168.1.110 listen
strip:
	$(EE_STRIP) $(EE_BIN)
test:
	$(EE_PREFIX)addr2line -e $(EE_BIN) 001140FC
clean:
	rm $(EE_BIN) $(EE_BIN:.ELF=-PACKED.ELF) $(EE_OBJS) $(IRX_OBJS) ./bin/*

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp
