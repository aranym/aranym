/*
 *	ARAnyM ethernet driver.
 *
 *	based on dummy.xif skeleton 12/14/94, Kay Roemer.
 *
 *  written by Standa & Joy @ ARAnyM team
 *
 *  GPL
 */

# include "global.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "netinfo.h"

# include "mint/sockio.h"

# include <osbind.h>

#define MY_DEBUG 0

#define AUTO_IP_CONFIGURE 0

#if AUTO_IP_CONFIGURE
#include "inet4/in.h"
unsigned long inet_aton(const char *cp, struct in_addr *addr);
#endif

#include "ethernet_nfapi.h"

#define XIF_NAME	"ARAnyM Eth driver v0.4"

/* old handler */
extern void (*old_interrupt)(void);

/* interrupt wrapper routine */
void my_interrupt (void);

/* the C routine handling the interrupt */
void _cdecl aranym_interrupt(void);

long driver_init (void);


/*
 * Our interface structure
 */
static struct netif if_ara;

/*
 * Prototypes for our service functions
 */
static long	ara_open	(struct netif *);
static long	ara_close	(struct netif *);
static long	ara_output	(struct netif *, BUF *, const char *, short, short);
static long	ara_ioctl	(struct netif *, short, long);
static long	ara_config	(struct netif *, struct ifopt *);


/* ================================================================ */
static unsigned long nfEtherFsId;

/* NatFeat opcodes */
static long _NF_getid = 0x73004e75L;
static long _NF_call  = 0x73014e75L;

#ifndef _natfeat_h_
#define _natfeat_h_


#ifndef CDECL
#  if __PUREC__
#    define CDECL cdecl
#  else
#    define CDECL
#  endif
#endif

/* NatFeat common defines */
#define nfGetID(n)  (((long CDECL (*)(const char *))&_NF_getid)n)
#define nfCall(n)   (((long CDECL (*)(long, ...))&_NF_call)n)

#define nf_stderr(text)	\
	(((long CDECL (*)(long, const char *))&_NF_call)(nfGetID(("NF_STDERR")), (text)))

#endif /* _natfeat_h_ */
/* ================================================================ */

static inline unsigned long
get_nfapi_version()
{
	return nfCall((ETH(GET_VERSION)));
}

static inline unsigned long
get_int_level()
{
	return nfCall((ETH(XIF_INTLEVEL)));
}

static inline void
get_hw_addr( char *buffer, int len )
{
	nfCall((ETH(XIF_GET_MAC), 0L /* ethX */, buffer, (unsigned long)len));
}

static inline void
nfInterrupt ( short in_use )
{
	nfCall((ETH(XIF_IRQ), (unsigned long)in_use));
}

static inline short
read_packet_len ()
{
	return nfCall((ETH(XIF_READLENGTH), 0L /* ethX */));
}

static inline void
read_block (char *cp, short len)
{
	nfCall((ETH(XIF_READBLOCK), 0L /* ethX */, cp, (unsigned long)len));
}

static inline void
send_block (char *cp, short len)
{
	nfCall((ETH(XIF_WRITEBLOCK), 0L /* ethX */, cp, (unsigned long)len));
}

static void
aranym_install_int (void)
{
# define vector(x)      (x / 4)
	old_interrupt = Setexc(vector(0x60) + get_int_level(), (long) my_interrupt);
}


/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
ara_open (struct netif *nif)
{
	DEBUG (("araeth: open (nif = %08lx)", (long)nif));
	return nfCall((ETH(XIF_START), 0L /* ethX */));
}

/*
 * Opposite of ara_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long
ara_close (struct netif *nif)
{
	return nfCall((ETH(XIF_STOP), 0L /* ethX */));
}

/*
 * This routine is responsible for enqueing a packet for later sending.
 * The packet it passed in `buf', the destination hardware address and
 * length in `hwaddr' and `hwlen' and the type of the packet is passed
 * in `pktype'.
 *
 * `hwaddr' is guaranteed to be of type nif->hwtype and `hwlen' is
 * garuanteed to be equal to nif->hwlocal.len.
 *
 * `pktype' is currently one of (definitions in if.h):
 *	PKTYPE_IP for IP packets,
 *	PKTYPE_ARP for ARP packets,
 *	PKTYPE_RARP for reverse ARP packets.
 *
 * These constants are equal to the ethernet protocol types, ie. an
 * Ethernet driver may use them directly without prior conversion to
 * write them into the `proto' field of the ethernet header.
 *
 * If the hardware is currently busy, then you can use the interface
 * output queue (nif->snd) to store the packet for later transmission:
 *	if_enqueue (&nif->snd, buf, buf->info).
 *
 * `buf->info' specifies the packet's delivering priority. if_enqueue()
 * uses it to do some priority queuing on the packets, ie. if you enqueue
 * a high priority packet it may jump over some lower priority packets
 * that were already in the queue (ie that is *no* FIFO queue).
 *
 * You can dequeue a packet later by doing:
 *	buf = if_dequeue (&nif->snd);
 *
 * This will return NULL is no more packets are left in the queue.
 *
 * The buffer handling uses the structure BUF that is defined in buf.h.
 * Basically a BUF looks like this:
 *
 * typedef struct {
 *	long buflen;
 *	char *dstart;
 *	char *dend;
 *	...
 *	char data[0];
 * } BUF;
 *
 * The structure consists of BUF.buflen bytes. Up until BUF.data there are
 * some header fields as shown above. Beginning at BUF.data there are
 * BUF.buflen - sizeof (BUF) bytes (called userspace) used for storing the
 * packet.
 *
 * BUF.dstart must always point to the first byte of the packet contained
 * within the BUF, BUF.dend points to the first byte after the packet.
 *
 * BUF.dstart should be word aligned if you pass the BUF to any MintNet
 * functions! (except for the buf_* functions itself).
 *
 * BUF's are allocated by
 *	nbuf = buf_alloc (space, reserve, mode);
 *
 * where `space' is the size of the userspace of the BUF you need, `reserve'
 * is used to set BUF.dstart = BUF.dend = BUF.data + `reserve' and mode is
 * one of
 *	BUF_NORMAL for calls from kernel space,
 *	BUF_ATOMIC for calls from interrupt handlers.
 *
 * buf_alloc() returns NULL on failure.
 *
 * Usually you need to pre- or postpend some headers to the packet contained
 * in the passed BUF. To make sure there is enough space in the BUF for this
 * use
 *	nbuf = buf_reserve (obuf, reserve, where);
 *
 * where `obuf' is the BUF where you want to reserve some space, `reserve'
 * is the amount of space to reserve and `where' is one of
 *	BUF_RESERVE_START for reserving space before BUF.dstart
 *	BUF_RESERVE_END for reserving space after BUF.dend
 *
 * Note that buf_reserve() returns pointer to a new buffer `nbuf' (possibly
 * != obuf) that is a clone of `obuf' with enough space allocated. `obuf'
 * is no longer existant afterwards.
 *
 * However, if buf_reserve() returns NULL for failure then `obuf' is
 * untouched.
 *
 * buf_reserve() does not modify the BUF.dstart or BUF.dend pointers, it
 * only makes sure you have the space to do so.
 *
 * In the worst case (if the BUF is to small), buf_reserve() allocates a new
 * BUF and copies the old one to the new one (this is when `nbuf' != `obuf').
 *
 * To avoid this you should reserve enough space when calling buf_alloc(), so
 * buf_reserve() does not need to copy. This is what MintNet does with the BUFs
 * passed to the output function, so that copying is never needed. You should
 * do the same for input BUFs, ie allocate the packet as eg.
 *	buf = buf_alloc (nif->mtu+sizeof (eth_hdr)+100, 50, BUF_ATOMIC);
 *
 * Then up to nif->mtu plus the length of the ethernet header bytes long
 * frames may ne received and there are still 50 bytes after and before
 * the packet.
 *
 * If you have sent the contents of the BUF you should free it by calling
 *	buf_deref (`buf', `mode');
 *
 * where `buf' should be freed and `mode' is one of the modes described for
 * buf_alloc().
 *
 * Functions that can be called from interrupt:
 *	buf_alloc (..., ..., BUF_ATOMIC);
 *	buf_deref (..., BUF_ATOMIC);
 *	if_enqueue ();
 *	if_dequeue ();
 *	if_input ();
 *	eth_remove_hdr ();
 *	addroottimeout (..., ..., 1);
 */

static long
ara_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;

	/*
	 * Attach eth header. MintNet provides you with the eth_build_hdr
	 * function that attaches an ethernet header to the packet in
	 * buf. It takes the BUF (buf), the interface (nif), the hardware
	 * address (hwaddr) and the packet type (pktype).
	 *
	 * Returns NULL if the header could not be attached (the passed
	 * buf is thrown away in this case).
	 *
	 * Otherwise a pointer to a new BUF with the packet and attached
	 * header is returned and the old buf pointer is no longer valid.
	 */
	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if (nbuf == 0)
	{
		nif->out_errors++;
		return ENOMEM;
	}
	nif->out_packets++;

	/*
	 * Here you should either send the packet to the hardware or
	 * enqueue the packet and send the next packet as soon as
	 * the hardware is finished.
	 *
	 * If you are done sending the packet free it with buf_deref().
	 *
	 * Before sending it pass it to the packet filter.
	 */
	if (nif->bpf)
		bpf_input (nif, nbuf);

	/*
	 * Send packet
	 */
	{
		short len = nbuf->dend - nbuf->dstart;
		DEBUG (("araeth: send %d (%x) bytes", len));
		len = MAX (len, 60);

		send_block (nbuf->dstart, len);
	}

	return 0;
}

/*
 * MintNet notifies you of some noteable IOCLT's. Usually you don't
 * need to act on them because MintNet already has done so and only
 * tells you that an ioctl happened.
 *
 * One useful thing might be SIOCGLNKFLAGS and SIOCSLNKFLAGS for setting
 * and getting flags specific to your driver. For an example how to use
 * them look at slip.c
 */
static long
ara_ioctl (struct netif *nif, short cmd, long arg)
{
#if AUTO_IP_CONFIGURE
	enum {NONE, ADDR, NETMASK} gif = NONE;
#endif
#if MY_DEBUG
	char a[255];
	ksprintf(a, "Ethernet ioctl %d\n", cmd - ('S'<<8));
	nf_stderr(a);
#endif
	DEBUG (("araeth: ioctl cmd = %d (%x) bytes", cmd));



	switch (cmd)
	{
#if AUTO_IP_CONFIGURE
		case SIOCGIFADDR:
			if (gif == NONE) gif = ADDR;
			/* fall through */
		case SIOCGIFNETMASK:
			if (gif == NONE) gif = NETMASK;

			if (gif == NETMASK || gif == ADDR) {
				char buffer[128];
				struct ifreq *ifr = (struct ifreq *) arg;
				struct sockaddr_in *s = (struct sockaddr_in *)&(
					gif ? ifr->ifru.netmask : ifr->ifru.addr
				);
				nfCall((ETH(gif == NETMASK ? XIF_GET_NETMASK : XIF_GET_IPATARI),
					0 /* ethX */, buffer, sizeof(buffer)));
				inet_aton(buffer, &s->sin_addr);
			}
			return 0;
#endif /* AUTO_IP_CONFIGURE */

		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;

		case SIOCSIFMTU:
			/*
			 * Limit MTU to 1500 bytes. MintNet has alraedy set nif->mtu
			 * to the new value, we only limit it here.
			 */
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;

		case SIOCSIFOPT:
			/*
			 * Interface configuration, handled by ara_config()
			 */
			{
				struct ifreq *ifr = (struct ifreq *) arg;
				return ara_config (nif, ifr->ifru.data);
			}
	}

	return ENOSYS;
}

/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that ara_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
ara_config (struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

	if (!STRNCMP ("hwaddr"))
	{
		uchar *cp;
		/*
		 * Set hardware address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwlocal.addr, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwlocal.addr;
		DEBUG (("dummy: hwaddr is %x:%x:%x:%x:%x:%x",
				cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
	}
	else if (!STRNCMP ("braddr"))
	{
		uchar *cp;
		/*
		 * Set broadcast address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.addr, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwbrcst.addr;
		DEBUG (("dummy: braddr is %x:%x:%x:%x:%x:%x",
				cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
	}
	else if (!STRNCMP ("debug"))
	{
		/*
		 * turn debuggin on/off
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DEBUG (("dummy: debug level is %ld", ifo->ifou.v_long));
	}
	else if (!STRNCMP ("log"))
	{
		/*
		 * set log file
		 */
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DEBUG (("dummy: log file is %s", ifo->ifou.v_string));
	}

	return ENOSYS;
}

/*
 * Initialization. This is called when the driver is loaded. If you
 * link the driver with main.o and init.o then this must be called
 * driver_init() because main() calles a function with this name.
 *
 * You should probe for your hardware here, setup the interface
 * structure and register your interface.
 *
 * This function should return 0 on success and != 0 if initialization
 * fails.
 */
long
driver_init (void)
{
	static char message[100];
	static char my_file_name[128];


	/* get the HostFs NatFeat ID */
	nfEtherFsId = nfGetID(("ETHERNET"));
	if ( nfEtherFsId == 0 ) {
		c_conws(XIF_NAME " not installed - NatFeat not found\n\r");
		return 1;
	}

	/* compare the version */
	if ( get_nfapi_version() != ARAETHER_NFAPI_VERSION ) {
		ksprintf (message, XIF_NAME " not installed - version mismatch: %ld != %d\n\r", get_nfapi_version(), ARAETHER_NFAPI_VERSION);
		c_conws(message);
		return 1;
	}

	/*
	 * Set interface name
	 */
	strcpy (if_ara.name, "eth");
	/*
	 * Set interface unit. if_getfreeunit("name") returns a yet
	 * unused unit number for the interface type "name".
	 */
	if_ara.unit = if_getfreeunit ("eth");
	/*
	 * Alays set to zero
	 */
	if_ara.metric = 0;
	/*
	 * Initial interface flags, should be IFF_BROADCAST for
	 * Ethernet.
	 */
	if_ara.flags = IFF_BROADCAST;
	/*
	 * Maximum transmission unit, should be >= 46 and <= 1500 for
	 * Ethernet
	 */
	if_ara.mtu = 1500;
	/*
	 * Time in ms between calls to (*if_ara.timeout) ();
	 */
	if_ara.timer = 0;

	/*
	 * Interface hardware type
	 */
	if_ara.hwtype = HWTYPE_ETH;
	/*
	 * Hardware address length, 6 bytes for Ethernet
	 */
	if_ara.hwlocal.len = ETH_ALEN;
	if_ara.hwbrcst.len = ETH_ALEN;

	/*
	 * Set interface hardware and broadcast addresses. For real ethernet
	 * drivers you must get them from the hardware of course!
	 */
	/* ask host for the hardware address */
	get_hw_addr(if_ara.hwlocal.addr, ETH_ALEN);
	memcpy (if_ara.hwbrcst.addr, "\377\377\377\377\377\377", ETH_ALEN);

	/*
	 * Set length of send and receive queue. IF_MAXQ is a good value.
	 */
	if_ara.rcv.maxqlen = IF_MAXQ;
	if_ara.snd.maxqlen = IF_MAXQ;
	/*
	 * Setup pointers to service functions
	 */
	if_ara.open = ara_open;
	if_ara.close = ara_close;
	if_ara.output = ara_output;
	if_ara.ioctl = ara_ioctl;
	/*
	 * Optional timer function that is called every 200ms.
	 */
	if_ara.timeout = 0;

	/*
	 * Here you could attach some more data your driver may need
	 */
	if_ara.data = 0;

	/*
	 * Number of packets the hardware can receive in fast succession,
	 * 0 means unlimited.
	 */
	if_ara.maxpackets = 0;

	/*
	 * Install the interface interrupt.
	 */
	aranym_install_int();

	/*
	 * Register the interface.
	 */
	if_register (&if_ara);

	/*
	 * NETINFO->fname is a pointer to the drivers file name
	 * (without leading path), eg. "dummy.xif".
	 * NOTE: the file name will be overwritten when you leave the
	 * init function. So if you need it later make a copy!
	 */
	if (NETINFO->fname)
	{
		strncpy (my_file_name, NETINFO->fname, sizeof (my_file_name));
		my_file_name[sizeof (my_file_name) - 1] = '\0';
# if 0
		ksprintf (message, "My file name is '%s'\n\r", my_file_name);
		c_conws (message);
# endif
	}
	/*
	 * And say we are alive...
	 */
	ksprintf (message, XIF_NAME " (eth%d)\n\r", if_ara.unit);
	c_conws (message);
	return 0;
}


/*
 * Read a packet out of the adapter and pass it to the upper layers
 */
INLINE void
recv_packet (struct netif *nif)
{
	ushort pktlen;
	BUF *b;

	/* read packet length (excluding 32 bit crc) */
	pktlen = read_packet_len ();

	DEBUG (("araeth: recv_packet: %i", pktlen));

	//if (pktlen < 32)
	if (!pktlen)
	{
		DEBUG (("araeth: recv_packet: pktlen == 0"));
		nif->in_errors++;
		return;
	}

	b = buf_alloc (pktlen+100, 50, BUF_ATOMIC);
	if (!b)
	{
		DEBUG (("araeth: recv_packet: out of mem (buf_alloc failed)"));
		nif->in_errors++;
		return;
	}
	b->dend += pktlen;

	read_block (b->dstart, pktlen);

	/* Pass packet to upper layers */
	if (nif->bpf)
		bpf_input (nif, b);

	/* and enqueue packet */
	if (!if_input (nif, b, 0, eth_remove_hdr (b)))
		nif->in_packets++;
	else
		nif->in_errors++;
}


/*
 * interrupt routine
 */
void _cdecl
aranym_interrupt (void)
{
	static int in_use = 0;
	if (in_use)
		return;
	nfInterrupt( in_use = 1 );
	recv_packet (&if_ara);
	nfInterrupt( in_use = 0 );
}

#if AUTO_IP_CONFIGURE
/*
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
/*in_addr_t*/
unsigned long inet_aton(const char *cp, struct in_addr *addr)
{
	register unsigned long val;
	int base;
	register int n;
	register char c;
	unsigned long parts[4];
	register unsigned long *pp = parts;

	c = *cp;
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!isdigit(c))
			goto ret_0;
		base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X')
				base = 16, c = *++cp;
			else
				base = 8;
		}
		val = 0;
		for (;;) {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) |
					(c + 10 - (islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3)
				goto ret_0;
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && (!isascii(c) || !isspace(c)))
		goto ret_0;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
		goto ret_0;		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (parts[0] > 0xff || val > 0xffffff)
			goto ret_0;
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (parts[0] > 0xff || parts[1] > 0xff || val > 0xffff)
			goto ret_0;
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xff
		    || val > 0xff)
			goto ret_0;
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);

	return (1);

ret_0:
	return (0);
}
#endif /* AUTO_IP_CONFIGURE */
