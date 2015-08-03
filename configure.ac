#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([monitoring-plugins], [0.1], [discostu@zoozer.de])
AM_INIT_AUTOMAKE
AC_OUTPUT(Makefile plugins/Makefile templates/Makefile)
AC_CONFIG_SRCDIR([plugins/check_procstat.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([strerror strspn])

AC_OUTPUT
