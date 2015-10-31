CXX ?= $(CROSS_COMPILE)gcc

FRU_WRITE_AND_PARSER_LIB = libwritefrudata.so
FRU_WRITE_AND_PARSER_OBJS = frup.o writefrudata.o

INC_FLAGS += $(shell pkg-config --cflags --libs libsystemd) -I. -O2 --std=gnu++14
LIB_FLAGS += $(shell pkg-config  --libs libsystemd) -rdynamic

DESTDIR ?= /
SBINDIR ?= /usr/sbin
INCLUDEDIR ?= /usr/include
LIBDIR ?= /usr/lib

all: $(FRU_WRITE_AND_PARSER_LIB)

%.o: %.c
	$(CXX) -fpic -c $< $(CXXFLAGS) $(INC_FLAGS) $(IPMID_PATH) -o $@

%.o: %.C
	$(CXX) -fpic -c $< $(CXXFLAGS) $(INC_FLAGS) $(IPMID_PATH) -o $@

$(FRU_WRITE_AND_PARSER_LIB): $(FRU_WRITE_AND_PARSER_OBJS)
	$(CXX) $^ -shared $(LDFLAGS) $(LIB_FLAGS) -o $@

clean:
	rm -f $(FRU_WRITE_AND_PARSER_OBJS) $(FRU_WRITE_AND_PARSER_LIB)

install:
		install -m 0755 -d $(DESTDIR)$(LIBDIR)/host-ipmid
		install -m 0755 $(FRU_WRITE_AND_PARSER_LIB) $(DESTDIR)$(LIBDIR)/host-ipmid
