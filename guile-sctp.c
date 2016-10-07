/* 
 * TODO:
 * clean up code
 * add other functions
 * test on Solaris, FreeBSD, Linux.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libguile.h>
#include <errno.h>
#include <sys/errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif

/* calculate the size of a buffer large enough to hold any supported
   sockaddr type.  if the buffer isn't large enough, certain system
   calls will return a truncated address.  */

#ifdef HAVE_IPV6
#define MAX_ADDR_SIZE sizeof (struct sockaddr_in6)
#else
#define MAX_ADDR_SIZE sizeof (struct sockaddr_in)
#endif

#ifdef WORDS_BIGENDIAN
#define FLIP_NET_HOST_128(addr)
#else
#define FLIP_NET_HOST_128(addr)\
{\
  int i;\
  \
  for (i = 0; i < 8; i++)\
    {\
      scm_t_uint8 c = (addr)[i];\
      \
      (addr)[i] = (addr)[15 - i];\
      (addr)[15 - i] = c;\
    }\
}
#endif

/* convert a host ordered SCM integer to a 128 bit IPv6 address in
   network order.  */
static void
scm_to_ipv6 (scm_t_uint8 dst[16], SCM src)
{
	if (SCM_I_INUMP (src))
	{
		scm_t_signed_bits n = SCM_I_INUM (src);
		if (n < 0)
		scm_out_of_range (NULL, src);
#ifdef WORDS_BIGENDIAN
		memset (dst, 0, 16 - sizeof (scm_t_signed_bits));
		memcpy (dst + (16 - sizeof (scm_t_signed_bits)),
		        &n,
		        sizeof (scm_t_signed_bits));
#else
		memset (dst + sizeof (scm_t_signed_bits),
		        0,
		        16 - sizeof (scm_t_signed_bits));
		/* FIXME: this pair of ops is kinda wasteful -- should rewrite as
		   a single loop perhaps, similar to the handling of bignums. */
		memcpy (dst, &n, sizeof (scm_t_signed_bits));
		FLIP_NET_HOST_128 (dst);
#endif
	}
	else if (SCM_BIGP (src))
	{
		size_t count;
      
		if ((mpz_sgn (SCM_I_BIG_MPZ (src)) < 0)
		    || mpz_sizeinbase (SCM_I_BIG_MPZ (src), 2) > 128)
			scm_out_of_range (NULL, src);
      
		memset (dst, 0, 16);
		mpz_export (dst,
		            &count,
		            1, /* big-endian chunk ordering */
		            16, /* chunks are 16 bytes long */
		            1, /* big-endian byte ordering */
		            0, /* "nails" -- leading unused bits per chunk */
		            SCM_I_BIG_MPZ (src));
		scm_remember_upto_here_1 (src);
	}
	else
		scm_wrong_type_arg (NULL, 0, src);
}

/* convert fam/address/args into a sockaddr of the appropriate type.
   args is modified by removing the arguments actually used.
   which_arg and proc are used when reporting errors:
   which_arg is the position of address in the original argument list.
   proc is the name of the original procedure.
   size returns the size of the structure allocated.  */

static struct sockaddr *
scm_fill_sockaddr (int fam, SCM address, SCM *args, int which_arg, const char *proc, size_t *size)
#define FUNC_NAME proc
{
	switch (fam) {
		case AF_INET:
		{
			struct sockaddr_in *soka;
			unsigned long addr;
			int port;

			SCM_VALIDATE_ULONG_COPY (which_arg, address, addr);
			SCM_VALIDATE_CONS (which_arg + 1, *args);
			port = scm_to_int (SCM_CAR (*args));
			*args = SCM_CDR (*args);
			soka = (struct sockaddr_in *) scm_malloc (sizeof (struct sockaddr_in));

#if HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
			soka->sin_len = sizeof (struct sockaddr_in);
#endif
			soka->sin_family = AF_INET;
			soka->sin_addr.s_addr = htonl (addr);
			soka->sin_port = htons (port);
			/* this is a hack for Linux */
			if (addr == INADDR_ANY)
				*size = 0;
			else
				*size = sizeof (struct sockaddr_in);
			return (struct sockaddr *) soka;
		}
#ifdef HAVE_IPV6
		case AF_INET6:
		{
			/* see RFC2553.  */
			int port;
			struct sockaddr_in6 *soka;
			unsigned long flowinfo = 0;
			unsigned long scope_id = 0;

			SCM_VALIDATE_CONS (which_arg + 1, *args);
			port = scm_to_int (SCM_CAR (*args));
			*args = SCM_CDR (*args);
			if (scm_is_pair (*args)) {
				SCM_VALIDATE_ULONG_COPY (which_arg + 2, SCM_CAR (*args), flowinfo);
				*args = SCM_CDR (*args);
				if (scm_is_pair (*args)) {
					SCM_VALIDATE_ULONG_COPY (which_arg + 3, SCM_CAR (*args), scope_id);
					*args = SCM_CDR (*args);
				}
			}
			soka = (struct sockaddr_in6 *) scm_malloc (sizeof (struct sockaddr_in6));

#if HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
			soka->sin6_len = sizeof (struct sockaddr_in6);
#endif
			soka->sin6_family = AF_INET6;
			scm_to_ipv6 (soka->sin6_addr.s6_addr, address);
			soka->sin6_port = htons (port);
			soka->sin6_flowinfo = flowinfo;
#ifdef HAVE_SIN6_SCOPE_ID
			soka->sin6_scope_id = scope_id;
#endif
			/* this is a hack for Linux */
			if (IN6_ARE_ADDR_EQUAL(&soka->sin6_addr, &in6addr_any))
				*size = 0;
			else
				*size = sizeof (struct sockaddr_in6);
			return (struct sockaddr *) soka;
		}
#endif
		default:
			scm_out_of_range (proc, scm_from_int (fam));
	}
}
#undef FUNC_NAME

/* convert a 128 bit IPv6 address in network order to a host ordered SCM integer.  */
/* Copied from libguile/socket.c */

static SCM
scm_from_ipv6 (const scm_t_uint8 *src)
{
	SCM result = scm_i_mkbig ();
	mpz_import (SCM_I_BIG_MPZ (result), 
	            1,  /* chunk */
	            1,  /* big-endian chunk ordering */
	            16, /* chunks are 16 bytes long */
	            1,  /* big-endian byte ordering */
	            0,  /* "nails" -- leading unused bits per chunk */
	            src);
	return scm_i_normbig (result);
}

/* Put the components of a sockaddr into a new SCM vector.  */
/* copied from libguile/socket.c */

static SCM_C_INLINE_KEYWORD SCM
_scm_from_sockaddr (const struct sockaddr *address, unsigned addr_size, const char *proc)
{
	short int fam = address->sa_family;
	SCM result =SCM_EOL;

	switch (fam) {
		case AF_INET:
		{
			const struct sockaddr_in *nad = (struct sockaddr_in *) address;

			result = scm_c_make_vector (3, SCM_UNSPECIFIED);

			SCM_SIMPLE_VECTOR_SET(result, 0, scm_from_short (fam));
			SCM_SIMPLE_VECTOR_SET(result, 1, scm_from_ulong (ntohl (nad->sin_addr.s_addr)));
			SCM_SIMPLE_VECTOR_SET(result, 2, scm_from_ushort (ntohs (nad->sin_port)));
		}
		break;
#ifdef HAVE_IPV6
		case AF_INET6:
		{
			const struct sockaddr_in6 *nad = (struct sockaddr_in6 *) address;

			result = scm_c_make_vector (5, SCM_UNSPECIFIED);
			SCM_SIMPLE_VECTOR_SET(result, 0, scm_from_short (fam));
			SCM_SIMPLE_VECTOR_SET(result, 1, scm_from_ipv6 (nad->sin6_addr.s6_addr));
			SCM_SIMPLE_VECTOR_SET(result, 2, scm_from_ushort (ntohs (nad->sin6_port)));
			SCM_SIMPLE_VECTOR_SET(result, 3, scm_from_uint32 (nad->sin6_flowinfo));
#ifdef HAVE_SIN6_SCOPE_ID
			SCM_SIMPLE_VECTOR_SET(result, 4, scm_from_ulong (nad->sin6_scope_id));
#else
			SCM_SIMPLE_VECTOR_SET(result, 4, SCM_INUM0);
#endif
		}
		break;
#endif
		default:
			result = SCM_UNSPECIFIED;
			scm_misc_error (proc, "unrecognised address family: ~A",
			scm_list_1 (scm_from_int (fam)));

	}
	return result;
}


#ifdef HAVE_SCTP_RECVMSG
SCM_DEFINE (net_sctp_recvmsg, "sctp-recvmsg!", 2, 3, 0,
            (SCM sock, SCM str, SCM flags, SCM start, SCM end),
	    "Return data from the socket port @var{sock} and also\n"
	    "information about where the data was received from.\n"
	    "@var{sock} must already be bound to the address from which\n"
	    "data is to be received.  @code{str}, is a string into which the\n"
	    "data will be written.  The size of @var{str} limits the amount\n"
	    "of data which can be received: in the case of packet protocols,\n"
	    "if a packet larger than this limit is encountered then some\n"
	    "data will be irrevocably lost.\n\n"
	    "The value returned is a list containing:\n"
	    "- the number of bytes read from the socket\n"
	    "- an address object in the same form as returned by @code{accept}\n"
	    "- the flags returned by the sctp_recvmsg call\n"
	    "- a list containing the SID, SSN, PPID, TSN and CUM_TSN\n"
	    "The @var{start} and @var{end} arguments specify a substring of\n"
	    "@var{str} to which the data should be written.\n\n"
	    "Note that the data is read directly from the socket file\n"
	    "descriptor: any unread buffered port data is ignored.")
#define FUNC_NAME s_net_sctp_recvmsg
{
	int rv;
	int fd;
	int flg;
	char *buf;
	size_t offset;
	size_t cend;
	SCM address, s_sinfo;
	socklen_t addr_size = MAX_ADDR_SIZE;
	socklen_t optlen;
	char max_addr[MAX_ADDR_SIZE];
	struct sockaddr *addr = (struct sockaddr *) max_addr;
	struct sctp_sndrcvinfo sinfo;
	struct sctp_event_subscribe events;

	SCM_VALIDATE_OPFPORT (1, sock);
	fd = SCM_FPORT_FDES (sock);

	SCM_VALIDATE_STRING (2, str);
	scm_i_get_substring_spec (scm_c_string_length (str), start, &offset, end, &cend);

	if (SCM_UNBNDP (flags))
		flg = 0;
	else
		SCM_VALIDATE_ULONG_COPY (3, flags, flg);

	addr->sa_family = AF_UNSPEC;

	/* enable the sctp_data_io_event */
	bzero((void *)&events, sizeof(struct sctp_event_subscribe));
	optlen = (socklen_t)sizeof(struct sctp_event_subscribe);
	getsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &events, &optlen);
	events.sctp_data_io_event = 1;
	optlen = (socklen_t)sizeof(struct sctp_event_subscribe);
	setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &events, optlen);

	bzero((void *)&sinfo, sizeof(struct sctp_sndrcvinfo));
	
	buf = scm_i_string_writable_chars (str);
	rv = sctp_recvmsg (fd, buf + offset, cend - offset, addr, &addr_size, &sinfo, &flg);
	scm_i_string_stop_writing ();
	
	if (rv == -1)
		SCM_SYSERROR;
	if (addr->sa_family != AF_UNSPEC)
		address = _scm_from_sockaddr (addr, addr_size, FUNC_NAME);
	else
		address = SCM_BOOL_F;

	s_sinfo = scm_list_5(scm_from_uint16 (sinfo.sinfo_stream),
	                     scm_from_uint16 (sinfo.sinfo_ssn),
	                     scm_from_uint32 (sinfo.sinfo_ppid),
	                     scm_from_uint32 (sinfo.sinfo_tsn),
	                     scm_from_uint32 (sinfo.sinfo_cumtsn));

	scm_remember_upto_here_1 (str);
	return scm_list_4 (scm_from_int (rv), address, scm_from_int (flg), s_sinfo); 
}
#undef FUNC_NAME
#endif

#ifdef HAVE_SCTP_SENDMSG
SCM_DEFINE (net_sctp_sendmsg, "sctp-sendmsg", 8, 0, 1,
            (SCM sock, SCM message, SCM ppid, SCM stream_no, SCM ttl, SCM context, SCM fam_or_sockaddr, SCM address, SCM args_and_flags),
	    "Transmit the string @var{message} on the socket port\n"
	    "@var{sock}.  The\n"
	    "parameters @var{ppid}, @var{stream_no}, @var{ttl} and  @var{context}\n"
	    "are the corresponding SCTP parameters. The\n"
	    "destination address is specified using the @var{fam},\n"
	    "@var{address} and\n"
	    "@var{args_and_flags} arguments, in a similar way to the\n"
	    "@code{connect} procedure.  @var{args_and_flags} contains\n"
	    "the usual connection arguments optionally followed by\n"
	    "a flags argument, which is a value or\n"
	    "bitwise OR of MSG_UNORDERED, MSG_ABORT, MSG_EOF etc.\n\n"
	    "The value returned is the number of bytes transmitted\n"
	    "Note that the data is written directly to the socket\n"
	    "file descriptor:\n"
	    "any unflushed buffered port data is ignored.")
#define FUNC_NAME s_net_sctp_sendmsg
{
	int rv;
	int fd;
	int flg;
	struct sockaddr *soka;
	size_t size;
	scm_t_uint16 c_stream_no;
	scm_t_uint32 c_ppid, c_ttl, c_context;

	sock = SCM_COERCE_OUTPORT (sock);
	SCM_VALIDATE_FPORT (1, sock);
	SCM_VALIDATE_STRING (2, message);
	c_ppid      = scm_to_uint32 (ppid);
	c_stream_no = scm_to_uint16 (stream_no);
	c_ttl       = scm_to_uint32 (ttl);
	c_context   = scm_to_uint32 (context);
	fd = SCM_FPORT_FDES (sock);

	if (!scm_is_number (fam_or_sockaddr)) {
		/* FAM_OR_SOCKADDR must actually be a `socket address' object.  This
		   means that the following arguments, i.e. ADDRESS and those listed in
		   ARGS_AND_FLAGS, are the `MSG_' flags.  */
		soka = scm_to_sockaddr (fam_or_sockaddr, &size);
		if (address != SCM_UNDEFINED)
			args_and_flags = scm_cons (address, args_and_flags);
	} else
		soka = scm_fill_sockaddr (scm_to_int (fam_or_sockaddr), address, &args_and_flags, 7, FUNC_NAME, &size);
		
	if (scm_is_null (args_and_flags))
		flg = 0;
	else {
		SCM_VALIDATE_CONS (9, args_and_flags);
		flg = SCM_NUM2ULONG (9, SCM_CAR (args_and_flags));
	}

	rv = sctp_sendmsg (fd,
	                   scm_i_string_chars (message),
	                   scm_c_string_length (message),
	                   soka,
	                   size,
	                   c_ppid,
	                   flg,
	                   c_stream_no,
	                   c_ttl,
	                   c_context);
	if (rv == -1) {
		int save_errno = errno;
		free (soka);
		errno = save_errno;
		SCM_SYSERROR;
	}
	free (soka);
	scm_remember_upto_here_1 (message);
	return scm_from_int (rv);
}
#undef FUNC_NAME
#endif


void
net_sctp_init (void)
{

/* protocol numbers */
#ifdef IPPROTO_SCTP
  scm_c_define ("IPPROTO_SCTP", scm_from_int (IPPROTO_SCTP));
#endif
#ifdef SCTP_UNORDERED
  scm_c_define ("SCTP_UNORDERED", scm_from_int (SCTP_UNORDERED));
#endif
#ifdef SCTP_ADDR_OVER
  scm_c_define ("SCTP_ADDR_OVER", scm_from_int (SCTP_ADDR_OVER));
#endif
#ifdef SCTP_ABORT
  scm_c_define("SCTP_ABORT", scm_from_int (SCTP_ABORT));
#endif
#ifdef SCTP_EOF
  scm_c_define ("SCTP_EOF", scm_from_int (SCTP_EOF));
#endif
#ifdef SCTP_EOR
  scm_c_define ("SCTP_EOR", scm_from_int (SCTP_EOR));
#endif
#ifdef MSG_NOTIFICATION
  scm_c_define ("MSG_NOTIFICATION", scm_from_int (MSG_NOTIFICATION));
#endif
#ifdef MSG_EOR
  scm_c_define ("MSG_EOR", scm_from_int (MSG_EOR));
#endif
#ifdef SCTP_PR_SCTP_TTL
  scm_c_define ("SCTP_PR_SCTP_TTL", scm_from_int (SCTP_PR_SCTP_TTL));
#endif
#ifdef SCTP_PR_SCTP_BUF
  scm_c_define ("SCTP_PR_SCTP_BUF", scm_from_int (SCTP_PR_SCTP_BUF));
#endif
#ifdef SCTP_NODELAY
  scm_c_define ("SCTP_NODELAY", scm_from_int (SCTP_NODELAY));
#endif
#include "guile-sctp.x"
}
