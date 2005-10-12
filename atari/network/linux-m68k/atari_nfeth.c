/*
 * atari_nfeth.c - ARAnyM ethernet card driver for GNU/Linux
 *
 * Copyright (c) 2005 Milan Jurik, Petr Stehlik of ARAnyM dev team
 *
 * Based on ARAnyM driver for FreeMiNT written by Standa Opichal
 * 
 * This software may be used and distributed according to the terms of
 * the GNU General Public License (GPL), incorporated herein by reference.
 */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <asm/atariints.h>

#include "../../arch/m68k/atari/natfeat.h"

#define DRV_NAME        "atari_nfeth"
#define DRV_VERSION     "0.3"
#define DRV_RELDATE     "10/12/2005"

/* These identify the driver base version and may not be removed. */
static char version[] __devinitdata =
KERN_INFO DRV_NAME ".c:v" DRV_VERSION " " DRV_RELDATE " S.Opichal, M.Jurik, P.Stehlik\n"
KERN_INFO "  http://aranym.atari.org/\n";

MODULE_AUTHOR("Milan Jurik");
MODULE_DESCRIPTION("Atari NFeth driver");
MODULE_LICENSE("GPL");
/*
MODULE_PARM(atari_nfeth_debug, "i");
MODULE_PARM_DESC(atari_nfeth_debug, "atari_nfeth_debug level (1-2)");
*/

#undef DEBUG

struct atari_nfeth_private {
	int ethX;
	struct net_device_stats	stats;
	spinlock_t lock;
};

static inline int getEthX(struct net_device *dev)
{
	return ((struct atari_nfeth_private *)netdev_priv(dev))->ethX;
}

int atari_nfeth_open(struct net_device *dev);
int atari_nfeth_stop(struct net_device *dev);
irqreturn_t atari_nfeth_interrupt(int irq, void *dev_id, struct pt_regs *fp);
int atari_nfeth_xmit(struct sk_buff *skb, struct net_device *dev);

int atari_nfeth_open(struct net_device *dev)
{
	nf_ethernet_xif_start(getEthX(dev));

	/* Set IRQ */
	dev->irq = nf_ethernet_get_irq();
	if (request_irq(dev->irq, atari_nfeth_interrupt, IRQ_TYPE_PRIO, dev->name, dev)) {
		printk( DRV_NAME ": request for irq %d failed\n", dev->irq);
		return( 0 );
	}

	/* Clean statistics */
	memset(&(((struct atari_nfeth_private *)netdev_priv(dev))->stats), 0, sizeof(((struct atari_nfeth_private *)(dev->priv))->stats));

	spin_lock_init(&(((struct atari_nfeth_private *)netdev_priv(dev))->lock));

#ifdef DEBUG
	printk( DRV_NAME ": open");
#endif

	/* Ready for data */
	netif_start_queue(dev);

	return 0;
}

int atari_nfeth_stop(struct net_device *dev)
{
	/* No more data */
	netif_stop_queue(dev);

	/* Release IRQ */
	free_irq(dev->irq, dev);

	nf_ethernet_xif_stop(getEthX(dev));

	return 0;
}

/*
 * Read a packet out of the adapter and pass it to the upper layers
 */
static irqreturn_t inline recv_packet (struct net_device *dev)
{
	int handled = 0;
	unsigned short pktlen;
	struct sk_buff *skb;
	struct atari_nfeth_private *anp = (struct atari_nfeth_private *)netdev_priv(dev);

	if (dev == NULL) {
		printk(DRV_NAME " recv_packet(): interrupt for unknown device.\n");
		return IRQ_NONE;
	}

	/* read packet length (excluding 32 bit crc) */
	pktlen = nf_ethernet_read_packet_len(getEthX(dev));

#ifdef DEBUG
	printk(DRV_NAME ": recv_packet: %i", pktlen);
#endif

	//if (pktlen < 32)
	if (!pktlen)
	{
#ifdef DEBUG
		printk(DRV_NAME ": recv_packet: pktlen == 0");
#endif
		anp->stats.rx_errors++;
		return IRQ_RETVAL(handled);
	}

	skb = dev_alloc_skb(pktlen + 2);
	if (skb == NULL)
	{
#ifdef DEBUG
		printk(DRV_NAME ": recv_packet: out of mem (buf_alloc failed)");
#endif
		anp->stats.rx_dropped++;
		return IRQ_RETVAL(handled);
	}

	skb->dev = dev;
	skb_reserve( skb, 2 );		/* 16 Byte align  */
	skb_put( skb, pktlen );	/* make room */
	nf_ethernet_read_block(getEthX(dev), skb->data, pktlen);

	skb->protocol = eth_type_trans(skb, dev);
	netif_rx(skb);
	dev->last_rx = jiffies;
	anp->stats.rx_packets++;
	anp->stats.rx_bytes += pktlen;

	/* and enqueue packet */
	handled = 1;
	return IRQ_RETVAL(handled);
}

irqreturn_t atari_nfeth_interrupt(int irq, void *dev_id, struct pt_regs *fp)
{
	struct net_device *dev = dev_id;
	struct atari_nfeth_private *anp = (struct atari_nfeth_private *)netdev_priv(dev);
	int this_dev_irq_bit;
	int irq_for_eth_bitmask;
	if (dev == NULL) {
#ifdef DEBUG
		printk(DRV_NAME " atari_nfeth_interrupt(): interrupt for unknown device.\n");
#endif
		return IRQ_NONE;
	}
	spin_lock(&anp->lock);
	irq_for_eth_bitmask = nf_ethernet_interrupt(0);
	this_dev_irq_bit = 2 ^ (anp->ethX);
	if (this_dev_irq_bit & irq_for_eth_bitmask) {
		recv_packet(dev);
		nf_ethernet_interrupt(this_dev_irq_bit);
	}
	spin_unlock(&anp->lock);
}

int atari_nfeth_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct atari_nfeth_private *anp = netdev_priv(dev);

	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, data, len);
		data = shortpkt;
		len = ETH_ZLEN;
	}

	dev->trans_start = jiffies;
	
#ifdef DEBUG
	printk( DRV_NAME ": send %d bytes", len);
#endif
	nf_ethernet_write_block(getEthX(dev), data, len);

	anp->stats.tx_packets++;
	anp->stats.tx_bytes += len;

	dev_kfree_skb(skb);
	return 0;
}

static void atari_nfeth_tx_timeout(struct net_device *dev)
{
	struct atari_nfeth_private *anp = netdev_priv(dev);
	anp->stats.tx_errors++;
	netif_wake_queue(dev);
}

static struct net_device_stats *atari_nfeth_get_stats(struct net_device *dev)
{
	struct atari_nfeth_private *anp = netdev_priv(dev);
	return &(anp->stats);
}

// probe1() - HW detection
// probe() - set module owner, found == 1, probe1()
// init() - probe()

static int __init atari_nfeth_probe1(struct net_device *dev, int ethX)
{
	static int did_version = 0;
	static int did_notinstall = 0;
	char errmsg[60];

	if ( ! nf_ethernet_check_version(errmsg, sizeof(errmsg)-1) ) {
		if (did_notinstall++ == 0)
			printk (DRV_NAME " not installed - %s\n", errmsg);
                return -ENODEV;
        }

	/* Get MAC address */
	if (! nf_ethernet_get_hw_addr(ethX, (unsigned char *)&(dev->dev_addr), ETH_ALEN)) {
#ifdef DEBUG
		printk(DRV_NAME " eth%d not installed - not defined\n", ethX);
#endif
		return -ENODEV;
	}

	ether_setup(dev);

	dev->open = &atari_nfeth_open;
	dev->stop = &atari_nfeth_stop;
	dev->hard_start_xmit = &atari_nfeth_xmit;
	dev->tx_timeout = &atari_nfeth_tx_timeout;
	dev->get_stats = &atari_nfeth_get_stats;
	dev->flags |= NETIF_F_NO_CSUM;

	if ((dev->priv = kmalloc(sizeof(struct atari_nfeth_private), GFP_KERNEL)) == NULL)
		return -ENOMEM;
	((struct atari_nfeth_private *)(dev->priv))->ethX = ethX; /* index of NF NIC */

	if (did_version++ == 0)
		printk(version);

	return 0;
}

int __init atari_nfeth_probe(struct net_device *dev)
{
	static int found = 0;

	SET_MODULE_OWNER(dev);

	if (!atari_nfeth_probe1(dev, found++)) {
		return 0;
	}

	return -ENODEV;
}

#ifdef MODULE
static struct net_device atari_nfeth_dev;

int atari_nfeth_init(void)
{
	int err;

	if (err = atari_nfeth_probe(&atari_nfeth_dev)) {
		return err;
	}

	// ? atari_nfeth_dev.init = atari_nfeth_probe;
	if ((err = register_netdev(&atari_nfeth_dev))) {
		if (err == -EIO)  {
			printk(DRV_NAME ": NatFeat Ethernet not found. Module not loaded.\n");
		}
		return err;
	}

        return 0;
}

void atari_nfeth_cleanup(void)
{
        unregister_netdev(&atari_nfeth_dev);
        free_netdev(atari_nfeth_dev); // ?
}

module_init(atari_nfeth_init);
module_exit(atari_nfeth_cleanup);

#endif /* MODULE */

/*
vim:ts=4:sw=4:
*/
