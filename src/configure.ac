dnl
dnl	aprsc
dnl
dnl	(c) Matti Aarnio, OH2MQK,  <oh2mqk@sral.fi>
dnl	(c) Heikki Hannikainen, OH7LZB
dnl
dnl	This program is licensed under the BSD license, which can be found
dnl	in the file LICENSE.
dnl

dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([aprsc.c])

dnl For automake
VERSION="`cat VERSION`"
PACKAGE=aprsc
dnl AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_PROG_MAKE_SET

AC_CONFIG_HEADERS([ac-hdrs.h])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL


dnl AC_PATH_PROG(LD, ld, ld)dnl
if test -z "$LD" ; then
  LD="$CC"
fi
AC_SUBST(LD,"$LD")

dnl If on i686, we'll need -march=i686 to get the atomic instructions
dnl On FreeBSD, the architecture is i386.
MACHINE="`uname -m`"
if test "$MACHINE" == "i686" -o "$MACHINE" == "i386"; then
  CFLAGS_ARCH="-march=i686"
fi
OS="`uname`"
if test "$OS" == "Darwin"; then
  CFLAGS_ARCH=""
fi
AC_SUBST(CFLAGS_ARCH)

dnl Check for GNU make
AX_CHECK_GNU_MAKE()

AC_ARG_ENABLE([static-libevent],
	[AS_HELP_STRING([--enable-static-libevent=path],
	[link against static libevent.a @<:@default=do not@:>@])],
	[static_libevent=$enableval],
	[])

dnl Check for libevent
AC_CHECK_HEADER([event2/event.h],,[AC_MSG_ERROR([libevent2 header not found - remember to install the development package too])])
AC_CHECK_HEADER([event2/http.h],,[AC_MSG_ERROR([libevent2 http header not found - remember to install the development package too])])
AC_CHECK_HEADER([event2/buffer.h],,[AC_MSG_ERROR([libevent2 buffer header not found - remember to install the development package too])])
AC_SUBST(LIBEVENT)
AC_CHECK_LIB(event,event_base_new,[LIBEVENT="-levent"],[AC_MSG_ERROR([libevent2 not found])])

AS_IF([test "x$static_libevent" != x],
	[LIBEVENT="$static_libevent"])

dnl Check for POSIX capabilities
AC_CHECK_HEADERS([sys/capability.h], AC_DEFINE([HAVE_CAPABILITY_H], [], [POSIX capabilities]))
AC_CHECK_HEADERS([sys/prctl.h], AC_DEFINE([HAVE_PRCTL_H], [], [Linux process control prctl.h]))
AC_SUBST(LIBCAP)
AC_CHECK_LIB(cap,cap_init,[LIBCAP="-lcap"])

dnl Check for 
AC_CHECK_HEADERS([sys/eventfd.h], AC_DEFINE([HAVE_EVENTFD_H], [], [Linux event fd support]))

dnl Checks for system headers
AC_CHECK_HEADERS([alloca.h],    AC_DEFINE([HAVE_ALLOCA_H]))
AC_CHECK_HEADERS([poll.h],      AC_DEFINE([HAVE_POLL_H]))
dnl AC_CHECK_FUNC(ppoll,,[Probably have ppoll of Linux])
AC_CHECK_HEADERS([sys/epoll.h], AC_DEFINE([HAVE_SYS_EPOLL_H]))
AC_CHECK_HEADERS([sys/event.h], AC_DEFINE([HAVE_SYS_EVENT_H]))
AC_CHECK_FUNC(kqueue, AC_DEFINE([HAVE_KQUEUE], [], [FreeBSD-style kqueue]))

dnl SCTP checks
AC_CHECK_HEADERS([netinet/sctp.h], AC_DEFINE([HAVE_NETINET_SCTP_H]))

dnl Checks for libraries.

dnl Checks for library functions.
AC_SUBST(LIBM)
AC_CHECK_FUNCS(atan2f,,
	       AC_CHECK_LIB(m, atan2f,
			    [LIBM="-lm"]))

AC_SUBST(LIBPTHREAD)
dnl AC_CHECK_FUNCS(pthread_exit,,
	       AC_CHECK_LIB(pthread, pthread_exit,
			    [LIBPTHREAD="-lpthread"])
dnl)

dnl Solaris resolver solution:
AC_SUBST(LIBGETNAMEINFO)
AC_CHECK_FUNCS(getnameinfo,,
	       AC_CHECK_LIB(nsl, getnameinfo,
			    [LIBGETNAMEINFO="-lnsl"]))

dnl
dnl We check for various libraries
dnl  - SysVr4 style of "-lsocket" at first (unless in libc)
dnl    The hallmark is  connect()  routine (we presume)
dnl
AC_SUBST(LIBSOCKET)dnl
ac_cv_libsocket_both=1
AC_CHECK_FUNC(connect, ac_cv_libsocket_both=0)
AC_CHECK_FUNC(gethostbyname, ac_cv_libsocket_both=0)
if test "$ac_cv_libsocket_both" = 1 ; then
  # Check cache
  if test "$ac_cv_func_socket_lxnet" = yes ; then
    AC_MSG_RESULT([need -lxnet library (cached)])
    LIBSOCKET="-lnsl -lsocket -lxnet"
  else
  if test "$ac_cv_func_socket_lsocket" = yes ; then
    AC_MSG_RESULT([need -lsocket library (cached)])
    LIBSOCKET="-lsocket"
    if test "$ac_cv_func_gethostbyname_lnsl" = yes ; then
        LIBSOCKET="-lnsl -lsocket"
    fi
  else
    # Well, will this work ?  SysVR4, but not Sun Solaris ?
    AC_CHECK_LIB(xnet, connect, [LIBSOCKET="-lnsl -lsocket -lxnet"
                                 ac_cv_func_socket_lsocket=no
                                 ac_cv_func_socket_lxnet=yes],[
      AC_CHECK_LIB(socket, connect, [LIBSOCKET="-lsocket"
                                     ac_cv_func_socket_lsocket=yes],
                                     ac_cv_func_socket_lsocket=no)
      if test "$ac_cv_func_socket_lsocket" = yes ; then
        t_oldLibs="$LIBS"
        LIBS="$LIBS -lsocket"
        AC_TRY_LINK([],[gethostbyname();], ,[
          LIBS="$LIBS -lnsl" # Add this Solaris library..
          AC_TRY_LINK([],[gethostbyname();],[
                        LIBSOCKET="-lsocket -lnsl"
                        ac_cv_func_gethostbyname_lnsl=yes
                ], [
                   AC_MSG_ERROR([Weird, '$LIBS' not enough to find  gethostnyname() ?!])
                ])
          ])
        LIBS="$t_oldLibs"
      fi
    ])
  fi
  fi
fi

# See about the routines that possibly exist at the libraries..
LIBS="$t_oldLibs $LIBSOCKET"
AC_CHECK_FUNCS(socket socketpair)
LIBS="$t_oldLibs"

if test "$ac_cv_func_socket" = no -a "$LIBSOCKET" != ""; then
  LIBS="$LIBS $LIBSOCKET"
  AC_TRY_LINK([],[socket();], ac_cv_func_socket=yes)
  if test $ac_cv_func_socket = yes; then
    AC_DEFINE(HAVE_SOCKET)
    AC_MSG_RESULT([Has  socket()  when using  $LIBSOCKET])
  fi
  LIBS="$t_oldLibs"
fi
if test "$ac_cv_func_socketpair" = no -a "$LIBSOCKET" != ""; then
  LIBS="$LIBS $LIBSOCKET"
  AC_TRY_LINK([],[socketpair();], ac_cv_func_socketpair=yes)
  if test $ac_cv_func_socketpair = yes; then
    AC_DEFINE(HAVE_SOCKETPAIR)
    AC_MSG_RESULT([Has  socketpair()  when using  $LIBSOCKET])
  fi
  LIBS="$t_oldLibs"
fi

ac_with_ipv6_replacement_libc=0
AC_ARG_WITH(ipv6-replacement-libc, [  --with-ipv6-replacement-libc If system has broken  getaddrinfo() and friends],
        ac_with_ipv6_replacement_libc=1)

LIBS="$LIBSOCKET $LIBS"
AC_CHECK_FUNCS(getnameinfo gai_strerror inet_ntop inet_pton)
LIBS="$t_oldLibs"


t_oldLibs="$LIBS"
LIBS="$LIBS $LIBSOCKET"

#  Following stuff may exist in our LIBC, or in separate -linet6
# AC_CHECK_FUNCS(inet_ntop inet_pton getaddrinfo getnameinfo gai_strerror)
#  If not found in LIBC, try to use  -linet6,  if it fails too,
#  THEN call the AC_REPLACE_FUNCS()
if test "$ac_cv_func_gai_strerror" = no ; then
    LIBS="$LIBS -linet6"
    AC_TRY_LINK([],[gai_strerror();], ac_cv_func_gai_strerror=yes)
    LIBS="$t_oldLibs"
    if test "$ac_cv_func_gai_strerror" = yes; then
        LIBSOCKET="$LIBSOCKET -linet6"
        AC_MSG_RESULT([Has IPv6 API  gai_strerror()  when using  $LIBSOCKET])
    else
      AC_LIBOBJ([inet_ntop])
      AC_LIBOBJ([inet_pton])
      AC_LIBOBJ([getnameinfo])
      AC_LIBOBJ([gai_strerror])
    fi
fi

LIBS="$t_oldLibs"

AC_SUBST(LIBS,"$LIBS")

dnl Check for the __sync_fetch_and_add builtin
dnl the check needs to use long long x, since we're incrementing a long long in the stats.
AC_CACHE_CHECK([for __sync_fetch_and_add], ac_cv_func_sync_fetch_and_add,
[AC_TRY_LINK([],[long long x;__sync_fetch_and_add(&x,1);],ac_cv_func_sync_fetch_and_add=yes,ac_cv_func_sync_fetch_and_add=no)])
if test "$ac_cv_func_sync_fetch_and_add" = yes; then
  AC_DEFINE(HAVE_SYNC_FETCH_AND_ADD,1,[Define if you have the __sync_fetch_and_add function])
fi

dnl Check for clock_gettime, and a library to have it
AC_SUBST(LIBRT)
AC_SEARCH_LIBS([clock_gettime], [rt], [
  AC_DEFINE(HAVE_CLOCK_GETTIME, 1,[Have clock_gettime])
])

if test "$ac_cv_search_clock_gettime" = "-lrt"; then
  LIBRT="-lrt"
fi

dnl Check for setenv and putenv, some might not have one
AC_CHECK_FUNCS(setenv putenv)

dnl Check for zlib
AC_SUBST(LIBZ)
AC_SEARCH_LIBS([deflate], [z], [
  AC_CHECK_HEADERS([zlib.h], AC_DEFINE(HAVE_LIBZ, 1, [Have libz]))
])

if test "$ac_cv_search_deflate" = "-lz"; then
  LIBZ="-lz"
fi

dnl Check for openssl
AC_ARG_WITH(openssl, [  --with-openssl[=DIR]    Include OpenSSL support (requires OpenSSL >= 0.9.7)], [with_openssl=$withval], [with_openssl=no])
AC_SUBST(LIBSSL)
AC_SUBST(LIBCRYPTO)
if test "$with_openssl" != "no" ; then
  AC_CHECK_HEADERS([openssl/ssl.h])
  AC_SEARCH_LIBS(TLSv1_server_method, ssl, AC_DEFINE(HAVE_TLSV1_SERVER_METHOD, 1, [OpenSSL 0.9.7 or later]))
  AC_SEARCH_LIBS(X509_free, crypto, AC_DEFINE(HAVE_X509_FREE, 1, [OpenSSL 0.9.7 or later]))
  
  if test "$ac_cv_search_TLSv1_server_method" = "-lssl"; then
    LIBSSL="-lssl"
  fi
  if test "$ac_cv_search_X509_free" = "-lcrypto"; then
    LIBCRYPTO="-lcrypto"
  fi
fi

dnl Output files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

dnl If GNU make is not the default, deal with it
if test "$_cv_gnu_make_command" != "" -a "$_cv_gnu_make_command" != "make" ; then
	mv Makefile GNUmakefile
	echo all clean distclean install: > Makefile ;
	echo \	$_cv_gnu_make_command \$@ >> Makefile;
fi

