#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([range-fcgi], [0.1.0], [vigith@gmail.com])
AM_INIT_AUTOMAKE(range-fcgi, 2.6)
AC_OUTPUT(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src/range-fcgi.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(crange, range_expand, [], [exit 1])
AC_CHECK_LIB(pthread, pthread_create, [], [exit 1])
AC_CHECK_LIB(apr-1, apr_initialize, [], [exit 1])
AC_CHECK_LIB(fcgi, FCGX_InitRequest, [], [exit 1])

LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h fcgiapp.h fcgi_stdio.h ])

AC_DEFUN([AC_CHECK_APR],[
AC_PATH_PROG(aprconfig,apr-config)
if test [ -z "$aprconfig" ]
then
    AC_PATH_PROG(aprconfig, apr-1-config)
fi
if test [ -z "$aprconfig" ]
then
    AC_MSG_ERROR([apr-config/apr-1-config executable not found])
else
    AC_MSG_CHECKING(apr includes)
    APR_CFLAGS=`${aprconfig} --cflags --cppflags --includes`
    AC_MSG_RESULT($APR_CFLAGS)
    AC_SUBST(APR_CFLAGS)
    AC_MSG_CHECKING(apr libraries)
    APR_LIBS=`${aprconfig} --link-libtool --libs`
    APR_LIBS_LD=`${aprconfig} --link-ld --libs`
    AC_MSG_RESULT($APR_LIBS)
    AC_SUBST(APR_LIBS)
    AC_SUBST(APR_LIBS_LD)
    CPPFLAGS="$CPPFLAGS -I${APR_CFLAGS}"
fi
])

AC_CHECK_APR

AC_CHECK_HEADER([libcrange.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit bzero])

AC_OUTPUT
