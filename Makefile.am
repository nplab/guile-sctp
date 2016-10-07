netdir = $(GUILE_SITE)/net

lib_LTLIBRARIES = libguile-net-sctp.la
net_DATA = sctp.scm

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(GUILE_CFLAGS)

libguile_net_sctp_la_SOURCES = guile-sctp.c

XFILES = guile-sctp.x

BUILT_SOURCES = $(XFILES)

.SUFFIXES = .x

.c.x:
	guile-snarf -o $@ $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CFLAGS) $<

EXTRA_DIST = bootstrap sctp.scm
