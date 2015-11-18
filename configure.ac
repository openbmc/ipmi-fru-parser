AC_INIT([ipmi-fru-parser], [0.1], [https://github.com/openbmc/ipmi-fru-parser/issues], [ipmi-fru-parser], [https://github.com/openbmc/ipmi-fru-parser])
AC_PREREQ([2.68])
AC_CONFIG_SRCDIR([frup.c])
AC_PROG_CC
AC_CHECK_FUNCS([memmove memset strerror])
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([host-ipmid/ipmid-api.h])
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_14([noext], [mandatory])
AM_PROG_AR
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
LT_INIT
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
PKG_CHECK_MODULES([libsystemd], [libsystemd >= 221])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
