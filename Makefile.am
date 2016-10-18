AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = phosphor-read-eeprom

phosphor_read_eeprom_SOURCES = readeeprom.cpp argument.cpp
phosphor_read_eeprom_LDFLAGS = $(SYSTEMD_LIBS) $(libmapper_LIBS)
phosphor_read_eeprom_CXXFLAGS = $(SYSTEMD_CFLAGS) $(libmapper_CFLAGS)
phosphor_read_eeprom_LDADD = libwritefrudata.la

libwritefrudatadir = ${libdir}
libwritefrudata_LTLIBRARIES = libwritefrudata.la
libwritefrudata_la_SOURCES = frup.c writefrudata.cpp
libwritefrudata_la_LDFLAGS = $(SYSTEMD_LIBS) $(libmapper_LIBS) -version-info 1:0:0 -shared
libwritefrudata_la_CXXFLAGS = $(SYSTEMD_CFLAGS) $(libmapper_CFLAGS)

libstrgfnhandlerdir = ${libdir}/host-ipmid
libstrgfnhandler_LTLIBRARIES = libstrgfnhandler.la
libstrgfnhandler_la_SOURCES = strgfnhandler.cpp
libstrgfnhandler_la_LDFLAGS = -version-info 1:0:0 -shared
libstrgfnhandler_la_LIBADD = libwritefrudata.la
