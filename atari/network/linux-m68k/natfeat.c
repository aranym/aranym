/*
 * natfeat.c - ARAnyM hardware support via Native Features (natfeats)
 *
 * Copyright (c) 2005 Petr Stehlik of ARAnyM dev team
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License (GPL), incorporated herein by reference.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/string.h> // strncpy()
#include <linux/kernel.h> // snprintf()
#include <asm/io.h> // virt_to_phys()

#include "natfeat.h"

#ifdef CONFIG_ATARI_NFETH
#include "natfeat_ethernet_nfapi.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

static unsigned long nf_get_id_instr = 0x73004e75UL;
static unsigned long nf_call_instr = 0x73014e75UL;

static struct nf_ops _nf_ops = { &nf_get_id_instr, &nf_call_instr }; 
static const struct nf_ops *nf_ops; 

#define NF_GETID(a)		nf_ops->get_id(virt_to_phys(a))

int detect_native_features(void)
{
    int		ret;
    long	save_sp, save_vbr;
    static long tmp_vectors[5];
    static char *nf_version = "NF_VERSION";
	
    __asm__ __volatile__
	(	"movec	%/vbr,%2\n\t"	/* save vbr value            */
		"movel	#Liierr,%3@(0x10)\n\t" /* set Illegal Insn vec */
 		"movec	%3,%/vbr\n\t"	/* set up temporary vectors  */
		"movel	%/sp,%1\n\t"	/* save sp                   */
		"moveq	#0,%0\n\t"		/* assume no NatFeats        */
		"clrl	%/d0\n\t"		/* clear ID value register   */
		"movel	%4,%/sp@\n\t"
		"subql	#4,%/sp\n\t"
		"dc	0x7300\n\t"			/* call NatFeat GetID        */
		"tstl	%/d0\n\t"		/* check ID value register   */
		"sne	%0\n\t"			/* if non-zero NatFeats work */
		"Liierr:\n\t"
		"movel	%1,%/sp\n\t"	/* restore sp                */
		"movec	%2,%/vbr"		/* restore vbr               */
		: "=&d" (ret), "=&r" (save_sp), "=&r" (save_vbr)
		: "a" (tmp_vectors), "a" (virt_to_phys(nf_version))
		: "d0"					/* reg d0 used by NatFeats   */
        );

    return( ret );
}

const struct nf_ops * nf_init(void)
{
	if (detect_native_features()) {
		nf_ops = &_nf_ops;
		nf_debug("NatFeats found\n");
		return nf_ops;
	}
	
	return NULL;
}

/****************/
/* NF Basic Set */
int nf_name1(char *buf, int bufsize)
{
	if (nf_ops) {
		static long nfid_name = 0;
		if (nfid_name == 0) {
			nfid_name = NF_GETID("NF_NAME");
		}

		if (nfid_name) {
			nf_ops->call(nfid_name+1, virt_to_phys(buf), bufsize); // TODO: lock buf to prevent swap out
			return TRUE;
		}
	}
        
	return FALSE;
}

int nf_debug(const char *msg)
{
	if (nf_ops) {
		static long nfid_stderr = 0;
		if (nfid_stderr == 0) {
			nfid_stderr = NF_GETID("NF_STDERR");
		}
		
		if (nfid_stderr) {
			nf_ops->call(nfid_stderr, virt_to_phys(msg)); // TODO: lock msg to prevent swap out
			return TRUE;
		}
	}
	
	return FALSE;
}

int nf_shutdown(void)
{
	if (nf_ops) {
		long shutdown_id = NF_GETID("NF_SHUTDOWN");
		
		if (shutdown_id) {
			nf_ops->call(shutdown_id);
			return TRUE; /* never returns actually */
		}
	}

	return FALSE;
}

#ifdef CONFIG_ATARI_NFETH

/****************************/
/* NatFeat Ethernet support */
static long nfEtherID = 0;

static int is_nf_eth(void)
{
	if (nf_ops) {
		if (nfEtherID == 0) {
			nfEtherID = NF_GETID("ETHERNET");
		}
	}
	return (nfEtherID != 0);
}

int nf_ethernet_check_version(char *errmsg, int errmsglen)
{
	if (is_nf_eth()) {
		static unsigned long host_ver = 0;
		if (host_ver == 0) {
			host_ver = nf_ops->call(ETH(GET_VERSION));
		}
		if (host_ver == ARAETHER_NFAPI_VERSION) {
			return TRUE;
		}
		else {
			// API version mismatch
			if (errmsg != NULL) {
				snprintf(errmsg, errmsglen,
					"NatFeat Ethernet API: expected %d, got %ld",
					ARAETHER_NFAPI_VERSION, host_ver);
			}
			return FALSE;
		}
	}
	if (errmsg != NULL) {
		strncpy(errmsg, "NatFeat Ethernet support not found", errmsglen);
	}
	return FALSE;
}

int nf_ethernet_get_irq(void)
{
	return is_nf_eth() ? nf_ops->call(ETH(XIF_INTLEVEL)) : 0;
}

int nf_ethernet_get_hw_addr(int ethX, char *buffer, int bufsize)
{
	if (is_nf_eth()) {
		return nf_ops->call(ETH(XIF_GET_MAC), (unsigned long)ethX, virt_to_phys(buffer), (unsigned long)bufsize); // TODO: lock buffer to prevent swap out
	}
	return FALSE;
}

int nf_ethernet_interrupt(int bit)
{
	if (is_nf_eth()) {
		return nf_ops->call(ETH(XIF_IRQ), (unsigned long)bit);
	}
	return 0;
}

int nf_ethernet_read_packet_len(int ethX)
{
	if (is_nf_eth()) {
		return nf_ops->call(ETH(XIF_READLENGTH), (unsigned long)ethX);
	}
	return 0;
}

void nf_ethernet_read_block(int ethX, char *buffer, int len)
{
	if (is_nf_eth()) {
		nf_ops->call(ETH(XIF_READBLOCK), (unsigned long)ethX, virt_to_phys(buffer), (unsigned long)len); // TODO: lock buffer to prevent swap out
	}
}

void nf_ethernet_write_block(int ethX, char *buffer, int len)
{
	if (is_nf_eth()) {
		nf_ops->call(ETH(XIF_WRITEBLOCK), (unsigned long)ethX, virt_to_phys(buffer), (unsigned long)len); // TODO: lock buffer to prevent swap out
	}
}

void nf_ethernet_xif_start(int ethX)
{
	if (is_nf_eth()) {
		nf_ops->call(ETH(XIF_START), (unsigned long)ethX);
	}
}

void nf_ethernet_xif_stop(int ethX)
{
	if (is_nf_eth()) {
		nf_ops->call(ETH(XIF_STOP), (unsigned long)ethX);
	}
}
#endif // CONFIG_ATARI_NFETH

/*
vim:ts=4:sw=4:
*/
