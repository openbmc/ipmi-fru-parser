AM_CPPFLAGS = -I$(top_srcdir)
check_PROGRAMS =

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# Build/add sample_unittest to test suite
check_PROGRAMS += test_writefrudata
test_writefrudata_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)
test_writefrudata_CXXFLAGS = $(PTHREAD_CFLAGS)
test_writefrudata_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS)
test_writefrudata_SOURCES = test_writefrudata.cpp
