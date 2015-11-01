LIBS += libwritefrudata.so
libwritefrudata.so_OBJS += frup.o writefrudata.o
libwritefrudata.so_NEEDED += libsystemd

#### -----------------------------------------------------------------------####
#                                                                              #
##                       Compilare Regulas Sequi                              ##
#                                                                              #
#### -----------------------------------------------------------------------####
OPTFLAGS ?= -O3 -g -pipe
CFLAGS ?= $(OPTFLAGS)
CXXFLAGS ?= $(OPTFLAGS)
CFLAGS += -Wall -flto -fPIC
CXXFLAGS += --std=gnu++14 -Wall -flto -fPIC -I.

__PKG_CONFIG = $(if $1,$(shell pkg-config $2 $1))

define __BUILD_EXE
$1 : $$($1_OBJS) $$(LIBS)
		$$(LINK.cpp) -o $$@ $$^ $(call __PKG_CONFIG,$($1_NEEDED),--libs)

$(eval CXXFLAGS += $(call __PKG_CONFIG,$($1_NEEDED),--cflags))

#include $$($1_OBJS:.o=.d)
endef

$(foreach exe,$(EXES),$(eval $(call __BUILD_EXE,$(exe))))

define __BUILD_LIB
$1 : $$($1_OBJS)
		$$(LINK.cpp) -shared -o $$@ $$^ $(call __PKG_CONFIG,$($1_NEEDED),--libs)

$(eval CXXFLAGS += $(call __PKG_CONFIG,$($1_NEEDED),--cflags))

#include $$($1_OBJS:.o=.d)
endef

$(foreach lib,$(LIBS),$(eval $(call __BUILD_LIB,$(lib))))

.PHONY: clean
clean:
		$(RM) $(foreach exe,$(EXES),$(exe) $($(exe)_OBJS)) \
			  $(foreach lib,$(LIBS),$(lib) $($(lib)_OBJS))

DESTDIR ?= /
BINDIR ?= /usr/bin
LIBDIR ?= /usr/lib

.PHONY: install
install:
		$(if $(EXES),\
		install -m 0755 -d $(DESTDIR)$(BINDIR) && \
		install -m 0755 $(EXES) $(DESTDIR)$(BINDIR) \
		)
		$(if $(LIBS),\
		install -m 0755 -d $(DESTDIR)$(LIBDIR)/host-ipmid && \
		install -m 0755 $(LIBS) $(DESTDIR)$(LIBDIR)/host-ipmid \
		)

.DEFAULT_GOAL: all
.PHONY: all
all: $(EXES) $(LIBS)
