.PHONY=
DELIVERABLE=../__kthreads.a
OBJFILES= __korientation.o __kcpuPowerOn.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

