.PHONY=
DELIVERABLE=../../firmwareCore.a
OBJFILES=debug.o decode.o fpu.o ops.o ops2.o prim_ops.o sys.o \
	x86EmuAuxFuncs.o ibmPcBios_setRegs.o ibmPcBios_getRegs.o \
	ibmPcBios_coreFuncs.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

