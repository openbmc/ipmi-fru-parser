CXX ?= $(CROSS_COMPILE)gcc

IPMI_FRU_PARSER_LIB = libifp.so
IPMI_FRU_PARSER_OBJS = frup.o


INC_FLAGS += -I. -O2 --std=gnu++11 -DIPMI_FRU_PARSER_DEBUG
#INC_FLAGS += -I. -O2 --std=gnu++11
LIB_FLAGS += -rdynamic

all: $(IPMI_FRU_PARSER_LIB)

%.o: %.c
	$(CXX) -fpic -c $< $(CXXFLAGS) $(INC_FLAGS) $(IPMID_PATH) -o $@

$(IPMI_FRU_PARSER_LIB): $(IPMI_FRU_PARSER_OBJS)
	$(CXX) $^ -shared $(LDFLAGS) $(LIB_FLAGS) -o $@

clean:
	rm -f $(IPMI_FRU_PARSER_OBJS) $(IPMI_FRU_PARSER_LIB)
