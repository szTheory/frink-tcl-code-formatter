# Process this file with autoconf to produce a configure script.
AC_INIT([frink],[2.2.2],[lindsay.marshall@ncl.ac.uk])
AC_CONFIG_SRCDIR([blocks.c])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
# Checks for programs.
AC_PROG_CC

CFLAGS="-ansii -g -Wall"

AC_PROG_INSTALL
AC_PROG_AWK

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strchr strpbrk])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
