CXX ?= $(CROSS_COMPILE)g++

IPMI_FRU_PARSER_LIB = libifp.so
IPMI_FRU_PARSER_OBJS = frup.o

INC_FLAGS += $(shell pkg-config --cflags --libs libsystemd) -I. -O2 --std=gnu++11
LIB_FLAGS += $(shell pkg-config  --libs libsystemd) -rdynamic
#IPMID_PATH ?= -DHOST_IPMI_LIB_PATH=\"/usr/lib/host-ipmid/\" 

all: $(IPMI_FRU_PARSER_LIB)

%.o: %.c
	$(CXX) -fpic -c $< $(CXXFLAGS) $(INC_FLAG) $(IPMID_PATH) -o $@

$(IPMI_FRU_PARSER_LIB): $(IPMI_FRU_PARSER_OBJS)
	$(CXX) $^ -shared $(LDFLAGS) $(LIB_FLAGS) -o $@

clean:
	rm -f $(IPMI_FRU_PARSER_OBJS) $(IPMI_FRU_PARSER_LIB)
