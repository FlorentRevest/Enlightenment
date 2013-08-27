clockdir = $(MDIR)/clock
clock_DATA = src/modules/clock/e-module-clock.edj \
	     src/modules/clock/module.desktop

EXTRA_DIST += $(clock_DATA)

clockpkgdir = $(MDIR)/clock/$(MODULE_ARCH)
clockpkg_LTLIBRARIES = src/modules/clock/module.la

src_modules_clock_module_la_LIBADD = $(MOD_LIBS)
src_modules_clock_module_la_CPPFLAGS = $(MOD_CPPFLAGS)
src_modules_clock_module_la_LDFLAGS = $(MOD_LDFLAGS)
src_modules_clock_module_la_SOURCES = src/modules/clock/e_mod_main.h \
			  src/modules/clock/e_mod_main.c \
			  src/modules/clock/e_mod_config.c

PHONIES += clock install-clock
clock: $(clockpkg_LTLIBRARIES) $(clock_DATA)
install-clock: install-clockDATA install-clockpkgLTLIBRARIES