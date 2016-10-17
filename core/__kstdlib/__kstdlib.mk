# __kcxxabi will soon be transformed into a compiler specific directory which
# will auto-build the appropriate C++ runtime for the current compiler.

__kcxxabi.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kstdlib/$(ZCXXABI)/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kstdlib/$(ZCXXABI); $(MAKE)

__kstdlib.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kstdlib/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kstdlib; $(MAKE)

