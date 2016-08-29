LIBS += libwritefrudata.so
HOST_LIBS += libstrgfnhandler.so
libwritefrudata.so_OBJS  += frup.o writefrudata.o
libwritefrudata.so_VERSION = 1
libstrgfnhandler.so_OBJS += strgfnhandler.o
libstrgfnhandler.so_VERSION = 1

EXES += phosphor-read-eeprom

phosphor-read-eeprom_OBJS  += readeeprom.o argument.o
phosphor-read-eeprom_LDLIBS += mapper
phosphor-read-eeprom_EXTRA_LIBS  += writefrudata
libstrgfnhandler.so_EXTRA_LIBS  += writefrudata
libwritefrudata.so_LDLIBS += mapper
libwritefrudata.so_NEEDED  += libsystemd
phosphor-read-eeprom_NEEDED  += libsystemd

#### -----------------------------------------------------------------------####
#                                                                              #
##                       Compilare Regulas Sequi                              ##
#                                                                              #
#### -----------------------------------------------------------------------####
OPTFLAGS ?= -O3 -g -pipe -G -K
CFLAGS ?= $(OPTFLAGS)
CXXFLAGS ?= $(OPTFLAGS)
CFLAGS += -Wall -Werror -flto -fPIC
CXXFLAGS += --std=gnu++14 -Wall -Werror -flto -fPIC

__PKG_CONFIG = $(if $1,$(shell pkg-config $2 $1))
__EXTRA_LIB_RESOLV = $(if $1,$1)

define __BUILD_EXE
$1 : $$($1_OBJS) | $$(LIBS) $$(HOST_LIBS)
		$$(LINK.cpp) -o $$@ $$^ $(call __EXTRA_LIB_RESOLV,$(addprefix -l,$($1_EXTRA_LIBS))) -L. $(addprefix -l,$($1_LDLIBS)) $(call __PKG_CONFIG,$($1_NEEDED),--libs)

$(eval CXXFLAGS += $(call __PKG_CONFIG,$($1_NEEDED),--cflags))

#include $$($1_OBJS:.o=.d)
endef

$(foreach exe,$(EXES),$(eval $(call __BUILD_EXE,$(exe))))

define __BUILD_LIB
$1 : $$($1_OBJS) | $$(addsuffix .so,$$(addprefix lib,$$($1_EXTRA_LIBS)))
		$$(LINK.cpp) -fPIC -shared -Wl,-soname,$$@ -o $$(addsuffix .$($1_VERSION), $$@) $$^ $(call __EXTRA_LIB_RESOLV,$(addprefix -l,$($1_EXTRA_LIBS))) -L. $(addprefix -l,$($1_LDLIBS)) $(call __PKG_CONFIG,$($1_NEEDED),--libs)
		ln -sf $$(addsuffix .$($1_VERSION), $$@) $1

$(eval CXXFLAGS += $(call __PKG_CONFIG,$($1_NEEDED),--cflags))

#include $$($1_OBJS:.o=.d)
endef

$(foreach lib,$(LIBS),$(eval $(call __BUILD_LIB,$(lib))))
$(foreach lib,$(HOST_LIBS),$(eval $(call __BUILD_LIB,$(lib))))

.PHONY: clean
clean:
		$(RM) $(foreach exe,$(EXES),$(exe) $($(exe)_OBJS)) \
			  $(foreach lib,$(LIBS),$(addsuffix .$($(lib)_VERSION), $(lib)) $(lib) $($(lib)_OBJS)) \
			  $(foreach lib,$(HOST_LIBS),$(addsuffix .$($(lib)_VERSION), $(lib)) $(lib) $($(lib)_OBJS))

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
		install -m 0755 $(foreach lib,$(HOST_LIBS),$(addsuffix .$($(lib)_VERSION), $(lib))) $(DESTDIR)$(LIBDIR)/host-ipmid && \
		install -m 0755 $(foreach lib,$(LIBS),$(addsuffix .$($(lib)_VERSION), $(lib))) $(DESTDIR)$(LIBDIR) \
		$(foreach lib,$(HOST_LIBS), && ln -sf $(addsuffix .$($(lib)_VERSION), $(lib)) $(DESTDIR)$(LIBDIR)/host-ipmid/$(lib)) \
		$(foreach lib,$(LIBS), && ln -sf $(addsuffix .$($(lib)_VERSION), $(lib)) $(DESTDIR)$(LIBDIR)/$(lib)) \
		)

.DEFAULT_GOAL: all
.PHONY: all
all: $(EXES) $(LIBS)
