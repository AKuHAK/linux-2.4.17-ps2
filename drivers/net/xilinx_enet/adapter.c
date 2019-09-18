/*
 * adapter.c
 *
 * Xilinx Ethernet Adapter component to interface XEmac component to Linux
 *
 * Author: MontaVista Software, Inc.
 *         source@mvista.com
 *
 * Copyright 2002 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This driver is a bit unusual in that it is composed of two logical
 * parts where one part is the OS independent code and the other part is
 * the OS dependent code.  Xilinx provides their drivers split in this
 * fashion.  This file represents the Linux OS dependent part known as
 * the Linux adapter.  The other files in this directory are the OS
 * independent files as provided by Xilinx with no changes made to them.
 * The names exported by those files begin with XEmac_.  All functions
 * in this file that are called by Linux have names that begin with
 * xenet_.  The functions in this file that have Handler in their name
 * are registered as callbacks with the underlying Xilinx OS independent
 * layer.  Any other functions are static helper functions.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/mii.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <xbasic_types.h>
#include "xemac.h"
#include "xemac_i.h"

MODULE_AUTHOR("MontaVista Software, Inc. <source@mvista.com>");
MODULE_DESCRIPTION("Xilinx Ethernet Media Access Controller driver");
MODULE_LICENSE("GPL");

#define TX_TIMEOUT (60*HZ)	/* Transmission timeout is 60 seconds. */

/*
 * Our private per device data.  When a net_device is allocated we will
 * ask for enough extra space for this.
 */
struct net_local {
	struct net_device_stats stats;	/* Statistics for this device */
	struct net_device *next_dev;	/* The next device in dev_list */
	u32 index;		/* Which interface is this */
	u32 save_BaseAddress;	/* Saved physical base address */
	XInterruptHandler Isr;	/* Pointer to the XEmac ISR routine */
	struct sk_buff *saved_skb;	/* skb being transmitted */
	spinlock_t skb_lock;	/* For atomic access to saved_skb */
	/*
	 * The underlying OS independent code needs space as well.  A
	 * pointer to the following XEmac structure will be passed to
	 * any XEmac_ function that requires it.  However, we treat the
	 * data as an opaque object in this file (meaning that we never
	 * reference any of the fields inside of the structure).
	 */
	XEmac Emac;
};

/* List of devices we're handling and a lock to give us atomic access. */
static struct net_device *dev_list = NULL;
static spinlock_t dev_lock = SPIN_LOCK_UNLOCKED;

/* SAATODO: This function will be moved into the Xilinx code. */
/*****************************************************************************/
/**
*
* Lookup the device configuration based on the emac instance.  The table
* EmacConfigTable contains the configuration info for each device in the system.
*
* @param Instance is the index of the emac being looked up.
*
* @return
*
* A pointer to the configuration table entry corresponding to the given
* device ID, or NULL if no match is found.
*
* @note
*
* None.
*
******************************************************************************/
XEmac_Config *XEmac_GetConfig(int Instance)
{
	if (Instance < 0 || Instance >= XPAR_XEMAC_NUM_INSTANCES)
	{
		return NULL;
	}

	return &XEmac_ConfigTable[Instance];
}

/*
 * This routine is registered with the OS as the function to call when
 * the EMAC interrupts.  It in turn, calls the Xilinx OS independent
 * interrupt function.  There are different interrupt functions for FIFO
 * and scatter-gather so we just set a pointer (Isr) into our private
 * data so we don't have to figure it out here.  The Xilinx OS
 * independent interrupt function will in turn call any callbacks that
 * we have registered for various conditions.
 */
static void
xenet_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device *dev = dev_id;
	struct net_local *lp = (struct net_local *) dev->priv;

	/* Call it. */
	(*(lp->Isr)) (&lp->Emac);
}

static int
xenet_open(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;

	/* Just to be safe, stop the device first */
	(void) XEmac_Stop(&lp->Emac);

	/* Set the MAC address each time opened. */
	if (XEmac_SetMacAddress(&lp->Emac, dev->dev_addr) != XST_SUCCESS) {
		printk(KERN_ERR "%s: Could not set MAC address.\n", dev->name);
		return -EIO;
	}

	/*
	 * If the device is not configured for polled mode, connect to the
	 * interrupt controller and enable interrupts.  Currently, there
	 * isn't any code to set polled mode, so this check is probably
	 * superfluous.
	 */
	if ((XEmac_GetOptions(&lp->Emac) & XEM_POLLED_OPTION) == 0) {
		int retval;
		/* Grab the IRQ */
		retval =
		    request_irq(dev->irq, &xenet_interrupt, 0, dev->name, dev);
		if (retval) {
			printk(KERN_ERR
			       "%s: Could not allocate interrupt %d.\n",
			       dev->name, dev->irq);
			return retval;
		}
	}

	if (XEmac_Start(&lp->Emac) != XST_SUCCESS) {
		printk(KERN_ERR "%s: Could not start device.\n", dev->name);
		free_irq(dev->irq, dev);
		return -EBUSY;
	}

	/* We're ready to go. */
	MOD_INC_USE_COUNT;
	netif_start_queue(dev);
	return 0;
}
static int
xenet_close(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;

	netif_stop_queue(dev);

	/*
	 * If not in polled mode, free the interrupt.  Currently, there
	 * isn't any code to set polled mode, so this check is probably
	 * superfluous.
	 */
	if ((XEmac_GetOptions(&lp->Emac) & XEM_POLLED_OPTION) == 0)
		free_irq(dev->irq, dev);

	if (XEmac_Stop(&lp->Emac) != XST_SUCCESS) {
		printk(KERN_ERR "%s: Could not stop device.\n", dev->name);
		return -EBUSY;
	}

	MOD_DEC_USE_COUNT;
	return 0;
}
static struct net_device_stats *
xenet_get_stats(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;
	return &lp->stats;
}

/*
 * Helper function to reset the underlying hardware.  This is called
 * when we get into such deep trouble that we don't know how to handle
 * otherwise.
 */
static void
reset(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;
	struct sk_buff *tskb;
	u32 Options;
	u8 IfgPart1;
	u8 IfgPart2;
	u8 SendThreshold;
	u32 SendWaitBound;
	u8 RecvThreshold;
	u32 RecvWaitBound;

	/* Shouldn't really be necessary, but shouldn't hurt. */
	netif_stop_queue(dev);

	/*
	   * XEmac_Reset puts the device back to the default state.  We need
	   * to save all the settings we don't already know, reset, restore
	   * the settings, and then restart the emac.
	 */
	XEmac_GetInterframeGap(&lp->Emac, &IfgPart1, &IfgPart2);
	Options = XEmac_GetOptions(&lp->Emac);
	if (XEmac_mIsSgDma(&lp->Emac)) {
		(void) XEmac_GetPktThreshold(&lp->Emac, XEM_SEND,
					     &SendThreshold);
		(void) XEmac_GetPktWaitBound(&lp->Emac, XEM_SEND,
					     &SendWaitBound);
		(void) XEmac_GetPktThreshold(&lp->Emac, XEM_RECV,
					     &RecvThreshold);
		(void) XEmac_GetPktWaitBound(&lp->Emac, XEM_RECV,
					     &RecvWaitBound);
	}

	XEmac_Reset(&lp->Emac);

	(void) XEmac_SetMacAddress(&lp->Emac, dev->dev_addr);
	(void) XEmac_SetInterframeGap(&lp->Emac, IfgPart1, IfgPart2);
	(void) XEmac_SetOptions(&lp->Emac, Options);
	if (XEmac_mIsSgDma(&lp->Emac)) {
		(void) XEmac_SetPktThreshold(&lp->Emac, XEM_SEND,
					     SendThreshold);
		(void) XEmac_SetPktWaitBound(&lp->Emac, XEM_SEND,
					     SendWaitBound);
		(void) XEmac_SetPktThreshold(&lp->Emac, XEM_RECV,
					     RecvThreshold);
		(void) XEmac_SetPktWaitBound(&lp->Emac, XEM_RECV,
					     RecvWaitBound);
	}

	(void) XEmac_Start(&lp->Emac);

	/* Make sure that the send handler and we don't both free the skb. */
	spin_lock_irq(&lp->skb_lock);
	tskb = lp->saved_skb;
	lp->saved_skb = NULL;
	spin_unlock_irq(&lp->skb_lock);
	if (tskb)
		dev_kfree_skb(tskb);

	/* We're all ready to go.  Start the queue in case it was stopped. */
	netif_wake_queue(dev);
}

/* Helper function to determine if a given XEmac error warrants a reset. */
extern inline int
status_requires_reset(XStatus s)
{
	return (s == XST_DMA_ERROR || s == XST_FIFO_ERROR
		|| s == XST_RESET_ERROR);
}

static int
xenet_FifoSend(struct sk_buff *orig_skb, struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;
	struct sk_buff *new_skb;
	unsigned int len, align;

	/*
	 * The FIFO takes a single request at a time.  Stop the queue to
	 * accomplish this.  We'll wake the queue in FifoSendHandler once
	 * the skb has been sent or in xenet_tx_timeout if something goes
	 * horribly wrong.
	 */
	netif_stop_queue(dev);

	len = orig_skb->len;
	/*
	 * The packet FIFO requires the buffers to be 32 bit aligned.
	 * The sk_buff data is not 32 bit aligned, so we have to do this
	 * copy.  As you probably well know, this is not optimal.
	 */
	if (!(new_skb = dev_alloc_skb(len + 4))) {
		/* We couldn't get another skb. */
		dev_kfree_skb(orig_skb);
		lp->stats.tx_dropped++;
		printk(KERN_ERR "%s: Could not allocate transmit buffer.\n",
		       dev->name);
		netif_wake_queue(dev);
		return -EBUSY;
	}

	/*
	 * A new skb should have the data word aligned, but this code is
	 * here just in case that isn't true...  Calculate how many
	 * bytes we should reserve to get the data to start on a word
	 * boundary.  */
	align = 4 - ((unsigned long) new_skb->data & 3);
	if (align != 4)
		skb_reserve(new_skb, align);

	/* Copy the data from the original skb to the new one. */
	skb_put(new_skb, len);
	memcpy(new_skb->data, orig_skb->data, len);

	/* Get rid of the original skb. */
	dev_kfree_skb(orig_skb);

	lp->saved_skb = new_skb;
	if (XEmac_FifoSend(&lp->Emac, (u8 *) new_skb->data, len) != XST_SUCCESS) {
		/*
		 * I don't think that we will be fighting FifoSendHandler or
		 * xenet_tx_timeout, but it's cheap to guarantee it won't be a
		 * problem.
		 */
		spin_lock_irq(&lp->skb_lock);
		new_skb = lp->saved_skb;
		lp->saved_skb = NULL;
		spin_unlock_irq(&lp->skb_lock);

		dev_kfree_skb(new_skb);
		lp->stats.tx_errors++;
		printk(KERN_ERR "%s: Could not transmit buffer.\n", dev->name);
		netif_wake_queue(dev);
		return -EIO;
	}
	return 0;
}

/* The callback function for completed frames sent in FIFO mode. */
static void
FifoSendHandler(void *CallbackRef)
{
	struct net_device *dev = (struct net_device *) CallbackRef;
	struct net_local *lp = (struct net_local *) dev->priv;
	struct sk_buff *tskb;

	lp->stats.tx_bytes += lp->saved_skb->len;
	lp->stats.tx_packets++;

	/* Make sure that the timeout handler and we don't both free the skb. */
	spin_lock_irq(&lp->skb_lock);
	tskb = lp->saved_skb;
	lp->saved_skb = NULL;
	spin_unlock_irq(&lp->skb_lock);
	if (tskb)
		dev_kfree_skb(tskb);

	/* Start the queue back up to allow next request. */
	netif_wake_queue(dev);
}
static void
xenet_tx_timeout(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;
	printk("%s: Exceeded transmit timeout of %lu ms.  Resetting emac.\n",
	       dev->name, TX_TIMEOUT * 1000UL / HZ);

	lp->stats.tx_errors++;

	/*
	 * Make sure that no interrupts come in that could cause reentrancy
	 * problems in reset.
	 */
	disable_irq(dev->irq);
	reset(dev);
	enable_irq(dev->irq);
}

/* The callback function for frames received when in FIFO mode. */
static void
FifoRecvHandler(void *CallbackRef)
{
	struct net_device *dev = (struct net_device *) CallbackRef;
	struct net_local *lp = (struct net_local *) dev->priv;
	struct sk_buff *skb;
	unsigned int align;
	u32 len;
	XStatus Result;

	/*
	 * The OS independent Xilinx EMAC code does not provide a
	 * function to get the length of an incoming packet and a
	 * separate call to actually get the packet data.  It does this
	 * because they didn't add any code to keep the hardware's
	 * receive length and data FIFOs in sync.  Instead, they require
	 * that you send a maximal length buffer so that they can read
	 * the length and data FIFOs in a single chunk of code so that
	 * they can't get out of sync.  So, we need to allocate an skb
	 * that can hold a maximal sized packet.  The OS independent
	 * code needs to see the data 32-bit aligned, so we tack on an
	 * extra four just in case we need to do an skb_reserve to get
	 * it that way.
	 */
	len = XEM_MAX_FRAME_SIZE;
	if (!(skb = dev_alloc_skb(len + 4))) {
		/* Couldn't get memory. */
		lp->stats.rx_dropped++;
		printk(KERN_ERR "%s: Could not allocate receive buffer.\n",
		       dev->name);
		return;
	}

	/*
	 * A new skb should have the data word aligned, but this code is
	 * here just in case that isn't true...  Calculate how many
	 * bytes we should reserve to get the data to start on a word
	 * boundary.  */
	align = 4 - ((unsigned long) skb->data & 3);
	if (align != 4)
		skb_reserve(skb, align);

	Result = XEmac_FifoRecv(&lp->Emac, (u8 *) skb->data, &len);
	if (Result != XST_SUCCESS) {
		int need_reset = status_requires_reset(Result);

		lp->stats.rx_errors++;
		dev_kfree_skb(skb);

		printk(KERN_ERR "%s: Could not receive buffer, error=%d%s.\n",
		       dev->name, Result,
		       need_reset ? ", resetting device." : "");
		if (need_reset) {
			/*
			 * Make sure that the transmit timer doesn't pop
			 * while we're in reset to eliminate reentrancy
			 * problems.  We don't have to worry about
			 * missing it, because it would do exactly what
			 * we're going to do: reset the xemac.  */
			dev->tx_timeout = NULL;
			reset(dev);
			dev->tx_timeout = xenet_tx_timeout;
		}
		return;
	}

	skb_put(skb, len);	/* Tell the skb how much data we got. */
	skb->dev = dev;		/* Fill out required meta-data. */
	skb->protocol = eth_type_trans(skb, dev);

	lp->stats.rx_packets++;
	lp->stats.rx_bytes += len;

	netif_rx(skb);		/* Send the packet upstream. */
}

/* The callback function for errors. */
static void
ErrorHandler(void *CallbackRef, XStatus Code)
{
	struct net_device *dev = (struct net_device *) CallbackRef;
	int need_reset;
	need_reset = status_requires_reset(Code);

	printk(KERN_ERR "%s: device error %d%s\n",
	       dev->name, Code, need_reset ? ", resetting device." : "");
	if (need_reset) {
		/*
		 * Make sure that the transmit timer doesn't pop while we're
		 * in reset to eliminate reentrancy problems.  We don't have
		 * to worry about missing it, because it would do exactly
		 * what we're going to do: reset the xemac.
		 */
		dev->tx_timeout = NULL;
		reset(dev);
		dev->tx_timeout = xenet_tx_timeout;
	}
}

static void
xenet_set_multicast_list(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) dev->priv;
	u32 Options;

	/*
	 * XEmac_Start, XEmac_Stop and XEmac_SetOptions are supposed to
	 * be protected by a semaphore.  This Linux adapter doesn't have
	 * it as bad as the VxWorks adapter because the sequence of
	 * requests to us is much more sequential.  However, we do have
	 * one area in which this is a problem.
	 *
	 * xenet_set_multicast_list() is called while the link is up and
	 * interrupts are enabled, so at any point in time we could get
	 * an error that causes our reset() to be called.  reset() calls
	 * the aforementioned functions, and we need to call them from
	 * here as well.
	 *
	 * The solution is to make sure that we don't get interrupts or
	 * timers popping while we are in this function.
	 */
	disable_irq(dev->irq);
	local_bh_disable();

	/*
	 * The dev's set_multicast_list function is only called when
	 * the device is up.  So, without checking, we know we need to
	 * Stop and Start the XEmac because it has already been started.
	 */
	(void) XEmac_Stop(&lp->Emac);

	Options = XEmac_GetOptions(&lp->Emac);

	/* Clear out the bits we may set. */
	Options &= ~(XEM_PROMISC_OPTION | XEM_MULTICAST_OPTION);

	if (dev->flags & IFF_PROMISC)
		Options |= XEM_PROMISC_OPTION;
#if 0
	else {
		/*
		 * SAATODO: Xilinx is going to add multicast support to their
		 * VxWorks adapter and OS independent layer.  After that is
		 * done, this skeleton code should be fleshed out.  Note that
		 * IFF_MULTICAST is being masked out from dev->flags in probe,
		 * so that will need to be removed to actually do multidrop.
		 */
		if ((dev->flags & IFF_ALLMULTI)
		    || dev->mc_count > MAX_MULTICAST ? ? ?) {
			xemac_get_all_multicast ? ? ? ();
			Options |= XEM_MULTICAST_OPTION;
		} else if (dev->mc_count != 0) {
			struct dev_mc_list mc;

			xemac_clear_multicast_list ? ? ? ();
			for (mc = dev->mc_list; mc; mc = mc->next)
				xemac_add_multicast ? ? ? (mc);
			Options |= XEM_MULTICAST_OPTION;
		}
	}
#endif

	(void) XEmac_SetOptions(&lp->Emac, Options);

	(void) XEmac_Start(&lp->Emac);

	/* All done, get those interrupts and timers going again. */
	local_bh_enable();
	enable_irq(dev->irq);
}

static void
remove_head_dev(void)
{
	struct net_local *lp;
	struct net_device *dev;
	XEmac_Config *cfg;

	/* Pull the head off of dev_list. */
	spin_lock(&dev_lock);
	dev = dev_list;
	lp = (struct net_local *) dev->priv;
	dev_list = lp->next_dev;
	spin_unlock(&dev_lock);

	/* Put the base address back to the physical address. */
	cfg = XEmac_GetConfig(lp->index);
	iounmap((void *) cfg->BaseAddress);
	cfg->BaseAddress = lp->save_BaseAddress;

	/* Free up the memory. */
	if (lp->saved_skb)
		dev_kfree_skb(lp->saved_skb);
	kfree(lp);

	unregister_netdev(dev);
	kfree(dev);
}

static int __init
probe(int index)
{
	static const unsigned long remap_size
	    = XPAR_EMAC_0_HIGHADDR - XPAR_EMAC_0_BASEADDR + 1;
	struct net_device *dev;
	struct net_local *lp;
	XEmac_Config *cfg;
	unsigned int irq;

	switch (index) {
#if defined(XPAR_INTC_0_EMAC_0_VEC_ID)
	case 0:
		irq = 31 - XPAR_INTC_0_EMAC_0_VEC_ID;
		break;
#if defined(XPAR_INTC_0_EMAC_1_VEC_ID)
	case 1:
		irq = 31 - XPAR_INTC_0_EMAC_1_VEC_ID;
		break;
#if defined(XPAR_INTC_0_EMAC_2_VEC_ID)
	case 2:
		irq = 31 - XPAR_INTC_0_EMAC_2_VEC_ID;
		break;
#if defined(XPAR_INTC_0_EMAC_3_VEC_ID)
#error Edit this file to add more devices.
#endif				/* 3 */
#endif				/* 2 */
#endif				/* 1 */
#endif				/* 0 */
	default:
		return -ENODEV;
	}

	/* Find the config for our device. */
	cfg = XEmac_GetConfig(index);
	if (!cfg)
		return -ENODEV;

	dev = init_etherdev(0, sizeof (struct net_local));
	if (!dev) {
		printk(KERN_ERR "Could not allocate Xilinx enet device %d.\n",
		       index);
		return -ENOMEM;
	}
	SET_MODULE_OWNER(dev);

	ether_setup(dev);
	dev->irq = irq;

	/* Initialize our private data. */
	lp = (struct net_local *) dev->priv;
	memset(lp, 0, sizeof (struct net_local));
	lp->index = index;
	spin_lock_init(&lp->skb_lock);

	/* Make it the head of dev_list. */
	spin_lock(&dev_lock);
	lp->next_dev = dev_list;
	dev_list = dev;
	spin_unlock(&dev_lock);

	/* Change the addresses to be virtual; save the old ones to restore. */
	lp->save_BaseAddress = cfg->BaseAddress;
	cfg->BaseAddress = (u32) ioremap(lp->save_BaseAddress, remap_size);

	if (XEmac_Initialize(&lp->Emac, cfg->DeviceId) != XST_SUCCESS) {
		printk(KERN_ERR "%s: Could not initialize device.\n",
		       dev->name);
		remove_head_dev();
		return -ENODEV;
	}

	memcpy(dev->dev_addr, ((bd_t *) __res)->bi_enetaddr, 6);
	if (XEmac_SetMacAddress(&lp->Emac, dev->dev_addr) != XST_SUCCESS) {
		/* should not fail right after an initialize */
		printk(KERN_ERR "%s: Could not set MAC address.\n", dev->name);
		remove_head_dev();
		return -EIO;
	}

	/*
	 * SAATODO: Right now there is no code to sync the EMAC's and PHY's
	 * half/full duplex settings or reflect the connection state.
	 * Xilinx is going to enhance their driver with that and at that
	 * point, this driver should be modified in a similar fashion.
	 */

	if (XEmac_mIsSgDma(&lp->Emac)) {
		/*
		 * SAATODO: Currently scatter-gather DMA does not work.  At
		 * some point Xilinx is going to get that working in their
		 * code and then this driver should be enhanced in a similar
		 * fashion.
		 */
		printk(KERN_ERR "%s: Scatter gather not supported yet.\n",
		       dev->name);
		remove_head_dev();
		return -EIO;
		/* lp->Isr = XEmac_IntrHandlerDma; */
	} else {
		XEmac_SetFifoRecvHandler(&lp->Emac, dev, FifoRecvHandler);
		XEmac_SetFifoSendHandler(&lp->Emac, dev, FifoSendHandler);
		dev->hard_start_xmit = xenet_FifoSend;
		lp->Isr = XEmac_IntrHandlerFifo;
	}
	XEmac_SetErrorHandler(&lp->Emac, dev, ErrorHandler);

	dev->open = xenet_open;
	dev->stop = xenet_close;
	dev->get_stats = xenet_get_stats;
	dev->flags &= ~IFF_MULTICAST;
	dev->set_multicast_list = xenet_set_multicast_list;
	dev->tx_timeout = xenet_tx_timeout;
	dev->watchdog_timeo = TX_TIMEOUT;

	printk(KERN_INFO
	       "%s: Xilinx EMAC #%d at 0x%08X mapped to 0x%08X, irq=%d\n",
	       dev->name, index,
	       lp->save_BaseAddress, cfg->BaseAddress, dev->irq);
	return 0;
}
static int __init
xenet_init(void)
{
	int index = 0;

	while (probe(index++) == 0) ;
	/* If we found at least one, report success. */
	return (index > 1) ? 0 : -ENODEV;
}

static void __exit
xenet_cleanup(void)
{
	while (dev_list)
		remove_head_dev();
}

EXPORT_NO_SYMBOLS;

module_init(xenet_init);
module_exit(xenet_cleanup);
