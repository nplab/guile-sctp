dnl Process this file with autoconf to produce a configure script.

AC_INIT(guile-sctp, 1.2.4)
AC_CONFIG_SRCDIR(guile-sctp.c)
AM_INIT_AUTOMAKE

AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_LIBTOOL

GUILE_FLAGS
GUILE_SITE_DIR

AC_CHECK_HEADERS(sys/types.h sys/socket.h netinet/in.h netinet/sctp.h)
AC_C_BIGENDIAN

AC_MSG_CHECKING(for working IPv6 support)
AC_CACHE_VAL(guile_cv_have_ipv6,
            [AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
                             #include <sys/types.h>
                             #endif
                             #ifdef HAVE_NETINET_IN_H
                             #include <netinet/in.h>
                             #endif
                             #ifdef HAVE_SYS_SOCKET_H
                             #include <sys/socket.h>
                             #endif],
                            [struct sockaddr_in6 a;
                             a.sin6_family = AF_INET6;],
                            guile_cv_have_ipv6=yes,
                            guile_cv_have_ipv6=no)])
AC_MSG_RESULT($guile_cv_have_ipv6)
if test $guile_cv_have_ipv6 = yes; then
  AC_DEFINE(HAVE_IPV6, 1, [Define if you want support for IPv6.])
fi

# included in rfc2553 but not in older implementations, e.g., glibc 2.1.3.
AC_MSG_CHECKING(whether sockaddr_in6 has sin6_scope_id)
AC_CACHE_VAL(guile_cv_have_sin6_scope_id,
             [AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
                              #include <sys/types.h>
                              #endif
                              #ifdef HAVE_NETINET_IN_H
                              #include <netinet/in.h>
                              #endif],
                             [struct sockaddr_in6 sok;
                              sok.sin6_scope_id = 0;],
                             guile_cv_have_sin6_scope_id=yes,
                             guile_cv_have_sin6_scope_id=no)])
AC_MSG_RESULT($guile_cv_have_sin6_scope_id)
if test $guile_cv_have_sin6_scope_id = yes; then
  AC_DEFINE(HAVE_SIN6_SCOPE_ID, 1,
    [Define this if your IPv6 has sin6_scope_id in sockaddr_in6 struct.])
fi

AC_CHECK_MEMBERS([struct sockaddr.sa_len],,,
                 [#ifdef HAVE_SYS_TYPES_H
                  #include <sys/types.h>
                  #endif
                  #ifdef HAVE_SYS_SOCKET_H
                  #include <sys/socket.h>
                  #endif])

AC_CHECK_MEMBERS([struct sockaddr_in.sin_len],,,
                 [#ifdef HAVE_SYS_TYPES_H
                  #include <sys/types.h>
                  #endif
                  #ifdef HAVE_NETINET_IN_H
                  #include <netinet/in.h>
                  #endif])

AC_CHECK_MEMBERS([struct sockaddr_in6.sin6_len],,,
                 [#ifdef HAVE_SYS_TYPES_H
                  #include <sys/types.h>
                  #endif
                  #ifdef HAVE_NETINET_IN_H
                  #include <netinet/in.h>
                  #endif])

AC_CHECK_LIB(sctp, sctp_sendmsg)
AC_CHECK_LIB(sctp, sctp_recvmsg)
AC_CHECK_FUNCS(sctp_sendmsg sctp_recvmsg,,)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
