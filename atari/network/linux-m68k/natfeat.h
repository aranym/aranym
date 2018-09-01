/*
 * ARAnyM hardware support via Native Features (natfeats)
 *
 * Copyright (c) 2005 Petr Stehlik of ARAnyM dev team
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License (GPL), incorporated herein by reference.
 */

#ifndef _natfeat_h
#define _natfeat_h

struct nf_ops
{
	long (*get_id)(const char *);
	long (*call)(long id, ...);
	long res[3];
};

const struct nf_ops *nf_init(void);

int nf_name(char *buf, int bufsize);
int nf_debug(const char *msg);
int nf_shutdown(void);

int nf_ethernet_check_version(char *errmsg, int errmsglen);
int nf_ethernet_get_irq(void);
int nf_ethernet_get_hw_addr(int ethX, char *buffer, int bufsize);
int nf_ethernet_interrupt(int bit);
int nf_ethernet_read_packet_len(int ethX);
void nf_ethernet_read_block(int ethX, char *buffer, int len);
void nf_ethernet_write_block(int ethX, char *buffer, int len);
void nf_ethernet_xif_start(int ethX);
void nf_ethernet_xif_stop(int ethX);
# endif /* _natfeat_h */
