/*
 * ethernet_macosx.cpp - Mac OS X Ethernet support (via Berkley Packet Filter device)
 *
 * Copyright (c) 2007 ARAnyM dev team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "cpu_emulation.h"

#if defined(OS_darwin)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/socket.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CoreFoundation/CoreFoundation.h>

#include "main.h"
#include "ethernet_macosx.h"

#define DEBUG 0
#include "debug.h"

#include "fd_trans.h"

#define ETH_HELPER "bpf_helper"

// BPF filter program to ensure only ethernet packets are processed which are for configured MAC
static struct bpf_insn bpf_filter_mac_insn[] = {
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 0),                  //      ld  P[0:4]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xAAAAAAAA, 0, 3),	//		jeq destMAC, CONT, NOK
	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 4),                  // CONT:ld  P[4:2]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xAAAA, 0, 1),		//		jeq destMAC, OK, NOK
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                 // OK:  ret -1
	BPF_STMT(BPF_RET+BPF_K, 0),                         // NOK: ret 0
};

static struct bpf_program bpf_filter_mac = {sizeof(bpf_filter_mac_insn)/sizeof(struct bpf_insn), &bpf_filter_mac_insn[0]};

// BPF filter program to process multicast ethernet and configured MAC address packets
static struct bpf_insn bpf_filter_mcast_insn[] = {
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 0),                  //      ld  P[0:4]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xAAAAAAAA, 0, 2),	//		jeq destMAC, CONT, NOK1
	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 4),                  // CONT:ld  P[4:2]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xAAAA, 2, 0),		//		jeq destMAC, OK, NOK1
	BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 0),                  // NOK1:ld  P[0:1]
	BPF_JUMP(BPF_JMP|BPF_JSET|BPF_K, 0x01, 0, 1),       //      if (A & 1) OK else NOK2
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                 // OK:  ret -1
	BPF_STMT(BPF_RET+BPF_K, 0),                         // NOK2:ret 0
};

static struct bpf_program bpf_filter_mcast = {sizeof(bpf_filter_mcast_insn)/sizeof(struct bpf_insn), &bpf_filter_mcast_insn[0]};

// BPF filter program to process ethernet packets if the configured IP address matches
static struct bpf_insn bpf_filter_ip_insn[] = {
	// Check ethernet protocol (IP and ARP supported)
	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),					//			ld	P[12:2]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x0800, 3, 0),		//			jeq	IP_Type, CONT_IP, CONT
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x0806, 0, 5),		// CONT:	jeq	IP_Type, CONT_ARP, NOK
	// Load IP address from offset 38
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 38),                 // CONT_ARP:ld  P[38:4]
	BPF_JUMP(BPF_JMP+BPF_JA, 1, 1, 1),					//			jmp	COMPARE
	// Load IP address from offset 30
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 30),                 // CONT_IP:	ld  P[30:4]
	// Compare value with expected dest IP address
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xAAAAAAAA, 0, 1),	// COMPARE:	jeq destIP, OK, NOK
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                 // OK:		ret -1
	BPF_STMT(BPF_RET+BPF_K, 0),                         // NOK:		ret 0
};

static struct bpf_program bpf_filter_ip = {sizeof(bpf_filter_ip_insn)/sizeof(struct bpf_insn), &bpf_filter_ip_insn[0]};


// Ethernet frame
struct ethernet_frame
{
	unsigned char dst_addr[ 6 ];
	unsigned char src_addr[ 6 ];
	unsigned char type[2];
	unsigned char payload;
};

// ARP packet structure
struct arp_packet
{
	unsigned char hardware_type[2];
	unsigned char proto_type[2];
	unsigned char hardware_addr_length;
	unsigned char proto_addr_length;
	unsigned char opcode[2];
	unsigned char src_mac[6];
	unsigned char src_ip[4];
	unsigned char dst_mac[6];
	unsigned char dst_ip[4];
};

// IP packet structure
struct ip_packet
{
	unsigned char version_hdr_len;
	unsigned char services;
	unsigned char tot_len[2];
	unsigned char ident[2];
	unsigned char flags_frag_offset[2];
	unsigned char ttl;
	unsigned char proto_type;
	unsigned char hdr_chksum[2];
	unsigned char src_ip[4];
	unsigned char dst_ip[4];
	unsigned char data;
};

static void dump_frame(const char *prefix, struct ethernet_frame *frame)
{
	unsigned short int frame_type = (frame->type[0]<<8)|frame->type[1];
	bug("  %s: %02x:%02x:%02x:%02x:%02x:%02x > %02x:%02x:%02x:%02x:%02x:%02x  (Type %04x)", prefix,
		frame->src_addr[0], frame->src_addr[1], frame->src_addr[2], frame->src_addr[3], frame->src_addr[4], frame->src_addr[5],
		frame->dst_addr[0], frame->dst_addr[1], frame->dst_addr[2], frame->dst_addr[3], frame->dst_addr[4], frame->dst_addr[5],
		frame_type);
	if (frame_type == 0x0806) {
		struct arp_packet* arp = (struct arp_packet*) &frame->payload;
		bug("  %s: ARP packet %d.%d.%d.%d > %d.%d.%d.%d  (Type %04x)", prefix,
			arp->src_ip[0], arp->src_ip[1], arp->src_ip[2], arp->src_ip[3],
			arp->dst_ip[0], arp->dst_ip[1], arp->dst_ip[2], arp->dst_ip[3],
			(arp->proto_type[0]<<8)|arp->proto_type[1]);
	}
	else if (frame_type == 0x0800) {
		struct ip_packet* ip = (struct ip_packet*) &frame->payload;
		int tot_len = (ip->tot_len[0]<<8) + ip->tot_len[1];
		bug("  %s: IP packet %d.%d.%d.%d > %d.%d.%d.%d  (Type %04x Total %02x%02x = %d, Payload=%d)", prefix,
			ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3],
			ip->dst_ip[0], ip->dst_ip[1], ip->dst_ip[2], ip->dst_ip[3],
			ip->proto_type, ip->tot_len[0], ip->tot_len[1], tot_len, tot_len - (int)sizeof(ip_packet)+1);

//		for (unsigned char *ptr=&ip->data; ptr<(unsigned char*)&ip->data+tot_len; ptr++)
//			bug(">%*s\n",  tot_len-(sizeof(ip_packet)-1), &ip->data);
	}
}

static void dump_bpf_buf(const char *prefix, bpf_hdr* bpf_buf)
{
	if (bpf_buf->bh_datalen > 0) {
		bug("  %s: %d bytes in buf:", prefix, bpf_buf->bh_datalen);

		dump_frame(prefix, (struct ethernet_frame*)((char*)bpf_buf + bpf_buf->bh_hdrlen));
	}
}

void BPFEthernetHandler::reset_read_pos()
{
	read_len = 0;
	bpf_packet = NULL;
}


bool BPFEthernetHandler::open()
{
	// int nonblock = 1;
	char *type = bx_options.ethernet[ethX].type;
	char *dev_name = bx_options.ethernet[ethX].tunnel;

	close();

	if (strcmp(type, "none") == 0 || strlen(type) == 0)
	{
		return false;
	}

	debug = (strstr(type, "debug") != NULL);
	if (debug || DEBUG)
	{
		D(bug("BPF(%d): debug mode=%d", ethX, debug));
	}

	if (debug)
	{
		D(bug("BPF(%d): open() type=%s", ethX, type));
	}
	if (strstr(type, "bridge") == NULL )
	{
		panicbug("BPF(%d): unsupported type '%s'", ethX, type);
		return false;
	}

	int sockfd;
	if ((sockfd = fd_setup_server()) < 0)
	{
		fprintf(stderr, "receiver: failed to create socket.\n");
		return false;
	}

	/******************************************************
	 Fork child process to get an open BPF file descriptor
	 ******************************************************/
	char exe_path[2048];
	CFURLRef url = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
	CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
	CFURLGetFileSystemRepresentation(url2, true, (UInt8 *)exe_path, sizeof(exe_path));
	CFRelease(url2);
	CFRelease(url);
	addFilename(exe_path, ETH_HELPER, sizeof(exe_path));
	if (debug)
	{
		D(bug("BPF(%d): starting helper from <%s>", ethX, exe_path));
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		panicbug("BPF(%d): ERROR: fork() failed. Ethernet disabled!", ethX);
		::close(sockfd);
		return false;
	}
	else if (pid == 0)
	{
		if (debug)
		{
			D(bug("BPF(%d): "ETH_HELPER" child running", ethX));
		}
		int result = execl(exe_path, exe_path, NULL);
		_exit(result);
	}

	if (debug)
	{
		D(bug("BPF(%d): waiting for "ETH_HELPER" (PID %d) to send file descriptor", ethX, pid));
	}
	fd = fd_receive(sockfd, pid);
	::close(sockfd);
	if (fd < 0)
	{
		panicbug("BPF(%d): failed receiving file descriptor from "ETH_HELPER".", ethX);
		return false;
	}
	if (debug)
	{
		D(bug("BPF(%d): got file descriptor %d", ethX, fd));
	}


	/******************************************************
	 Configure BPF device
	 ******************************************************/
	// associate with specified interface
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	safe_strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);
	if (debug)
	{
		D(bug("BPF(%d): connecting with device %s", ethX, dev_name));
	}
	if(ioctl(fd, BIOCSETIF, &ifr) > 0)
	{
		panicbug("BPF(%d): Failed associating to %s: %s", ethX, dev_name, strerror(errno));
		close();
		return false;
	}

	// activate immediate mode
	int immediate = 1;
	if (debug)
	{
		D(bug("BPF(%d): enabling immediate mode", ethX));
	}
	if(ioctl(fd, BIOCIMMEDIATE, &immediate) == -1)
	{
		panicbug("BPF(%d): Unable to set immediate mode: %s", ethX, strerror(errno));
		close();
		return false;
	}

	// request buffer length
	if(ioctl(fd, BIOCGBLEN, &buf_len) == -1)
	{
		panicbug("BPF(%d): Unable to get buffer length: %s", ethX, strerror(errno));
		close();
		return false;
	}
	if (debug)
	{
		D(bug("BPF(%d): buf_len=%d", ethX, buf_len));
	}
	bpf_buf = (struct bpf_hdr*) malloc(buf_len);

	// activate promiscuous mode
	if (debug)
	{
		D(bug("BPF(%d): enabling promiscuous mode", ethX));
	}
	int promiscuous = 1;
	if(ioctl(fd, BIOCPROMISC, &promiscuous) == -1)
	{
		panicbug("BPF(%d): Unable to set promiscuous mode: %s", ethX, strerror(errno));
#if 0
		close();
		return false;
#endif
	}

	// activate "header complete" mode
	if (debug)
	{
		D(bug("BPF(%d): enabling header complete mode", ethX));
	}
	int complete = 1;
	if(ioctl(fd, BIOCGHDRCMPLT, &complete) == -1)
	{
		panicbug("BPF(%d): Unable to set header complete mode: %s", ethX, strerror(errno));
		close();
		return false;
	}

	// disable "see sent" mode
	if (debug)
	{
		D(bug("BPF(%d): disabling see sent mode", ethX));
	}
	int seesent = 1;
	if(ioctl(fd, BIOCSSEESENT, &seesent) == -1)
	{
		panicbug("BPF(%d): Unable to disable see sent mode: %s", ethX, strerror(errno));
		close();
		return false;
	}

	if (strstr(type, "nofilter") == NULL)
	{
		// convert and validate MAC address from configuration
		// default MAC Address is just made up
		uint8 mac_addr[6] = {'\0','A','E','T','H', '0'+ethX };

		// convert user-defined MAC Address from string to 6 bytes array
		char *mac_text = bx_options.ethernet[ethX].mac_addr;
		bool format_OK = false;
		if (strlen(mac_text) == 2*6+5 && (mac_text[2] == ':' || mac_text[2] == '-')) {
			mac_text[2] = mac_text[5] = mac_text[8] = mac_text[11] = mac_text[14] = ':';
			int md[6] = {0, 0, 0, 0, 0, 0};
			int matched = sscanf(mac_text, "%02x:%02x:%02x:%02x:%02x:%02x",
								 &md[0], &md[1], &md[2], &md[3], &md[4], &md[5]);
			if (matched == 6) {
				for(int i=0; i<6; i++)
					mac_addr[i] = md[i];
				format_OK = true;
			}
		}
		if (!format_OK) {
			panicbug("BPF(%d): Invalid MAC address: %s", ethX, mac_text);
			close();
			return false;
		}

		// convert and validate specified IP address
		uint8 ip_addr[4];
		if (!inet_pton(AF_INET, bx_options.ethernet[ethX].ip_atari, ip_addr)) {
			panicbug("BPF(%d): Invalid IP address specified: %s", ethX, bx_options.ethernet[ethX].ip_atari);
			close();
			return false;
		}

		// modify filter program to use specified IP address
		if (debug)
		{
			D(bug("BPF(%d): setting filter program for MAC address %s", ethX, mac_text));
		}

		// Select filter program according to chosen options
		struct bpf_program *filter;
		if (strstr(type, "mcast")) {
			filter = &bpf_filter_mcast;

			// patch filter instructions to detect valid MAC address
			filter->bf_insns[1].k = (mac_addr[0]<<24) | (mac_addr[1]<<16) | (mac_addr[2]<<8) | mac_addr[3];
			filter->bf_insns[3].k = (mac_addr[4]<<8) | mac_addr[5];
		}
		else if (strstr(type, "ip")) {
			filter = &bpf_filter_ip;

			// patch filter instructions to detect valid IP address
			filter->bf_insns[6].k = (ip_addr[0]<<24) | (ip_addr[1]<<16) | (ip_addr[2]<<8) | ip_addr[3];

		}
		else {
			filter = &bpf_filter_mac;
			// patch filter instructions to detect valid MAC address
			filter->bf_insns[1].k = (mac_addr[0]<<24) | (mac_addr[1]<<16) | (mac_addr[2]<<8) | mac_addr[3];
			filter->bf_insns[3].k = (mac_addr[4]<<8) | mac_addr[5];
		}

		// Enable filter program
		if(ioctl(fd, BIOCSETF, filter) == -1)
		{
			panicbug("BPF(%d): Unable to load filter program: %s", ethX, strerror(errno));
			close();
			return false;
		}
		if (debug)
		{
			D(bug("BPF(%d): filter program load", ethX));
		}
	}
	else {
		if (debug)
		{
			D(bug("BPF(%d): filter program skipped", ethX));
		}
	}


	// Reset bpf buffer read position (for handling multi packet)
	reset_read_pos();

	return true;
}

void BPFEthernetHandler::close()
{
	if (debug)
	{
		D(bug("BPF(%d): close", ethX));
	}

	reset_read_pos();

	if (fd > 0)
	{
		::close(fd);
		fd = -1;
	}

	free(bpf_buf);
	bpf_buf = NULL;

	debug = false;
}

int BPFEthernetHandler::recv(uint8 *buf, int len)
{
	if (debug)
	{
		D(bug("BPF(%d): recv(len=%d)", ethX, len));
	}

	// No more cached packet in memory?
	if (bpf_packet == NULL)
	{
		// Read a packet from the ethernet device, timeout after 2s
		// Initialize the file descriptor set.
		fd_set set;
		struct timeval timeout;
		FD_ZERO (&set);
		FD_SET (fd, &set);

		// Initialize the timeout data structure.
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		// select returns 0 if timeout, 1 if input available, -1 if error.
		if (select(FD_SETSIZE, &set, NULL, NULL, &timeout) == 1)
		{
			// Read from BPF device
			read_len = read(fd, bpf_buf, buf_len);
			if (debug) {
				D(bug("BPF(%d): %d bytes read => %d data bytes", ethX, read_len, bpf_buf->bh_datalen));
			}
			if (read_len > 0)
			{
				bpf_packet = bpf_buf;

				if (BPF_WORDALIGN(bpf_buf->bh_hdrlen + bpf_buf->bh_caplen) < read_len) {
					if (debug)
					{
						D(bug("BPF(%d): More than one packet received by BPF\n"
							"   read_len = %d\n"
							"   bh_hdrlen=%d\n"
							"   bh_datalen=%d\n"
							"   bh_caplen=%d\n"
							,ethX, read_len, bpf_buf->bh_hdrlen, bpf_buf->bh_datalen, bpf_buf->bh_caplen));
					}
				}
			}
		}
	}
	else {
		if (debug) {
			D(bug("BPF(%d): using cached %d data bytes from previous read", ethX, read_len));
		}
	}

	char *ptr = (char *)bpf_packet;
	if (bpf_packet && (ptr < ((char *)bpf_buf + read_len)))
	{
		char *frame_start = ptr + bpf_packet->bh_hdrlen;
		int frame_len = bpf_packet->bh_caplen;

		ptr += BPF_WORDALIGN(bpf_packet->bh_hdrlen + bpf_packet->bh_caplen);

		// Copy valid frame data
		if (frame_len <= len) {
			if (debug) {
				D(bug("BPF(%d): frame length %d bytes", ethX, frame_len));
				dump_bpf_buf("recv", bpf_packet);
			}
			memcpy(buf, frame_start, frame_len);
		}
		else {

			bug("BPF(%d): Host side received %d bytes of data but only %d bytes expected by guest.\n"
				"There are probably multiple packets in the received %d bytes.\n"
				"Packet discarded!", ethX, frame_len, len, read_len);
			bug("BPF(%d): read_len = %d\n"
				"   bh_hdrlen=%d\n"
				"   bh_datalen=%d\n"
				"   bh_caplen=%d\n"
				,ethX, read_len, bpf_packet->bh_hdrlen, bpf_packet->bh_datalen, bpf_packet->bh_caplen);
			frame_len = 0;
		}

		if (ptr < ((char *)bpf_buf + read_len))
			bpf_packet = (struct bpf_hdr*)ptr;
		else
			bpf_packet = NULL;


		return frame_len;
	}
	return 0;
}

int BPFEthernetHandler::send(const uint8 *buf, int len)
{
	if (debug)
	{
		D(bug("BPF(%d): send(len=%d)", ethX, len));
	}
	int res = -1;
	if (len > 0)
	{
		if (debug) {
			D(bug("BPF(%d): send(len=%d)", ethX, len));
			dump_frame("send", (struct ethernet_frame*) buf);
		}
		res = write(fd, buf, len);
		if (res < 0)
		{
			if (debug)
			{
				D(bug("BPF(%d): WARNING: Couldn't transmit packet", ethX));
			}
		}
	}
	return res;
}

BPFEthernetHandler::BPFEthernetHandler(int eth_idx) :
	Handler(eth_idx),
	debug(false),
	fd(-1),
	buf_len(0),
	bpf_buf(NULL),
	read_len(0),
	bpf_packet(NULL)
{
}

BPFEthernetHandler::~BPFEthernetHandler()
{
	close();
}

#endif /* OS_darwin? */
