/*******************************************************************************

  This software program is available to you under a choice of one of two 
  licenses. You may choose to be licensed under either the GNU General Public 
  License (GPL) Version 2, June 1991, available at 
  http://www.fsf.org/copyleft/gpl.html, or the Intel BSD + Patent License, the 
  text of which follows:
  
  Recipient has requested a license and Intel Corporation ("Intel") is willing
  to grant a license for the software entitled Linux Base Driver for the 
  Intel(R) PRO/1000 Family of Adapters (e1000) (the "Software") being provided
  by Intel Corporation. The following definitions apply to this license:
  
  "Licensed Patents" means patent claims licensable by Intel Corporation which 
  are necessarily infringed by the use of sale of the Software alone or when 
  combined with the operating system referred to below.
  
  "Recipient" means the party to whom Intel delivers this Software.
  
  "Licensee" means Recipient and those third parties that receive a license to 
  any operating system available under the GNU Public License version 2.0 or 
  later.
  
  Copyright (c) 1999 - 2002 Intel Corporation.
  All rights reserved.
  
  The license is provided to Recipient and Recipient's Licensees under the 
  following terms.
  
  Redistribution and use in source and binary forms of the Software, with or 
  without modification, are permitted provided that the following conditions 
  are met:
  
  Redistributions of source code of the Software may retain the above 
  copyright notice, this list of conditions and the following disclaimer.
  
  Redistributions in binary form of the Software may reproduce the above 
  copyright notice, this list of conditions and the following disclaimer in 
  the documentation and/or materials provided with the distribution.
  
  Neither the name of Intel Corporation nor the names of its contributors 
  shall be used to endorse or promote products derived from this Software 
  without specific prior written permission.
  
  Intel hereby grants Recipient and Licensees a non-exclusive, worldwide, 
  royalty-free patent license under Licensed Patents to make, use, sell, offer 
  to sell, import and otherwise transfer the Software, if any, in source code 
  and object code form. This license shall include changes to the Software 
  that are error corrections or other minor changes to the Software that do 
  not add functionality or features when the Software is incorporated in any 
  version of an operating system that has been distributed under the GNU 
  General Public License 2.0 or later. This patent license shall apply to the 
  combination of the Software and any operating system licensed under the GNU 
  Public License version 2.0 or later if, at the time Intel provides the 
  Software to Recipient, such addition of the Software to the then publicly 
  available versions of such operating systems available under the GNU Public 
  License version 2.0 or later (whether in gold, beta or alpha form) causes 
  such combination to be covered by the Licensed Patents. The patent license 
  shall not apply to any other combinations which include the Software. NO 
  hardware per se is licensed hereunder.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MECHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR IT CONTRIBUTORS BE LIABLE FOR ANY 
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
  (INCLUDING, BUT NOT LIMITED, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  ANY LOSS OF USE; DATA, OR PROFITS; OR BUSINESS INTERUPTION) HOWEVER CAUSED 
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR 
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/


/* Linux PRO/1000 Ethernet Driver main header file */

#ifndef _E1000_H_
#define _E1000_H_

#ifndef __E1000_MAIN__
#define __NO_VERSION__
#endif

#include <linux/stddef.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/capability.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/pkt_sched.h>

/* ethtool support */
#ifdef SIOCETHTOOL
#include <linux/ethtool.h>
#include <asm/uaccess.h>
#define  E1000_ETHTOOL_COPPER_INTERFACE_SUPPORTS (SUPPORTED_10baseT_Half | \
                    SUPPORTED_10baseT_Full | SUPPORTED_100baseT_Half | \
                    SUPPORTED_100baseT_Full | SUPPORTED_1000baseT_Full | \
                    SUPPORTED_Autoneg | SUPPORTED_MII)
#define  E1000_ETHTOOL_COPPER_INTERFACE_ADVERTISE (ADVERTISED_10baseT_Half | \
                    ADVERTISED_10baseT_Full | ADVERTISED_100baseT_Half | \
                    ADVERTISED_100baseT_Full | ADVERTISED_1000baseT_Full | \
                    ADVERTISED_Autoneg | ADVERTISED_MII)
#define E1000_ETHTOOL_FIBER_INTERFACE_SUPPORTS (SUPPORTED_Autoneg | \
                    SUPPORTED_FIBRE)
#define E1000_ETHTOOL_FIBER_INTERFACE_ADVERTISE (ADVERTISED_Autoneg | \
                    ADVERTISED_FIBRE)
#endif /* SIOCETHTOOL */

#include "e1000_kcompat.h"

struct e1000_adapter;

#include "e1000_mac.h"
#include "e1000_phy.h"

#ifdef IANS
#include "base_comm.h"
#include "ans_driver.h"
#include "ans.h"
#endif

#ifdef IDIAG
#include "idiag_pro.h"
#include "idiag_e1000.h"
#endif

#define BAR_0 0

/* 8254x can use Dual Address Cycles for 64-bit addressing */

/* Advertise that we can DMA from any address location */
#define E1000_DMA_MASK (~0x0UL)
#define E1000_DBG(args...)
//#define E1000_DBG(args...) printk("e1000: " args)
#define E1000_ERR(args...) printk(KERN_ERR "e1000: " args)
#ifdef CONFIG_PPC
#define E1000_MAX_INTR 1
#else
#define E1000_MAX_INTR 10
#endif
#define MAX_NUM_MULTICAST_ADDRESSES 128

/* command line options defaults */
#define DEFAULT_TXD                  256
#define MAX_TXD                      256
#define MIN_TXD                       80
#define MAX_82544_TXD               4096
#define DEFAULT_RXD                  256
#define MAX_RXD                      256
#define MIN_RXD                       80
#define MAX_82544_RXD               4096
#define DEFAULT_TIDV                  64
#define MAX_TIDV                  0xFFFF
#define MIN_TIDV                       0
#define DEFAULT_RIDV                  64
#define MAX_RIDV                  0xFFFF
#define MIN_RIDV                       0
#define DEFAULT_MDIX                   0
#define MAX_MDIX                       3
#define MIN_MDIX                       0

#define OPTION_UNSET    -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED  1
#define XSUMRX_DEFAULT       OPTION_ENABLED
#define WAITFORLINK_DEFAULT  OPTION_ENABLED
#define AUTONEG_ADV_DEFAULT  0x2F
#define AUTONEG_ADV_MASK     0x2F
#define FLOW_CONTROL_DEFAULT FLOW_CONTROL_FULL

#define E1000_REPORT_TX_EARLY  2

/* Supported RX Buffer Sizes */
#define E1000_RXBUFFER_2048  2048
#define E1000_RXBUFFER_4096  4096
#define E1000_RXBUFFER_8192  8192
#define E1000_RXBUFFER_16384 16384

#define E1000_JUMBO_PBA      0x00000028
#define E1000_DEFAULT_PBA    0x00000030

/* Round size up to the next multiple of unit */
#define E1000_ROUNDUP(size, unit) ((((size) + (unit) - 1) / (unit)) * (unit))

/* This is better, but only works for unit sizes that are powers of 2 */
#define E1000_ROUNDUP2(size, unit) (((size) + (unit) - 1) & ~((unit) - 1))

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer */
struct e1000_buffer {
    struct sk_buff *skb;
    uint64_t dma;
    unsigned long length;
};

/* Adapter->flags definitions */
#define E1000_BOARD_OPEN 0
#define E1000_RX_REFILL 1
#define E1000_DIAG_OPEN 2
#define E1000_LINK_STATUS_CHANGED 3

typedef enum _XSUM_CONTEXT_T {
    OFFLOAD_NONE,
    OFFLOAD_TCP_IP,
    OFFLOAD_UDP_IP
} XSUM_CONTEXT_T;

struct e1000_desc_ring {
    void *desc;                 /* pointer to the descriptor ring memory      */
    dma_addr_t dma;             /* physical address of the descriptor ring    */
    unsigned int size;          /* length of descriptor ring in bytes         */
    unsigned int count;         /* number of descriptors in the ring          */
    atomic_t unused;            /* number of descriptors with no buffer       */
    unsigned int next_to_use;   /* next descriptor to associate a buffer with */
    unsigned int next_to_clean; /* next descriptor to check for DD status bit */
    struct e1000_buffer *buffer_info; /* array of buffer information structs  */
};

#define E1000_RX_DESC(ring, i) \
    (&(((struct e1000_rx_desc *)(ring.desc))[i]))

#define E1000_TX_DESC(ring, i) \
    (&(((struct e1000_tx_desc *)(ring.desc))[i]))

#define E1000_CONTEXT_DESC(ring, i) \
    (&(((struct e1000_context_desc *)(ring.desc))[i]))

/* board specific private data structure */

struct e1000_adapter {
    struct e1000_adapter *next;
    struct e1000_adapter *prev;

    struct e1000_shared_adapter shared;

#ifdef IANS
    void *iANSReserved;
    piANSsupport_t iANSdata;
    uint32_t ans_link;
    uint32_t ans_speed;
    uint32_t ans_duplex;
    uint32_t ans_suspend;
    IANS_BD_TAGGING_MODE tag_mode;
#endif

    spinlock_t stats_lock;
    spinlock_t rx_fill_lock;

    unsigned long flags;
    uint32_t bd_number;
    struct timer_list timer_id;

    /* Ethernet Node Address */
    uint8_t perm_net_addr[ETH_LENGTH_OF_ADDRESS];

    /* Status Flags */
    boolean_t link_active;
    uint16_t link_speed;
    uint16_t link_duplex;
    uint32_t rx_buffer_len;

    /* PCI Device Info */
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t rev_id;
    uint16_t subven_id;
    uint16_t subsys_id;

    uint32_t part_num;

    uint32_t int_mask;

    /* driver specific */
    struct tasklet_struct rx_fill_tasklet;

    struct e1000_desc_ring tx_ring;
    uint32_t tx_int_delay;
    uint32_t TxdCmd;
    atomic_t tx_timeout;

    struct e1000_desc_ring rx_ring;
    uint32_t rx_int_delay;

    uint64_t XsumRXGood;
    uint64_t XsumRXError;

    /* Linux driver specific */
    struct net_device *netdev;
    struct pci_dev *pdev;
    struct net_device_stats net_stats;
    char *id_string;
    boolean_t RxChecksum;
    XSUM_CONTEXT_T ActiveChecksumContext;

    struct e1000_phy_info phy_info;
    struct e1000_shared_stats stats;

    /* PHY Statistics */
    struct e1000_phy_stats phy_stats;
};

/* Prototypes */

/* e1000_main.c */
extern int e1000_init_module(void);
extern int e1000_probe_all(void);
extern void e1000_exit_module(void);
extern int e1000_probe(struct pci_dev *pdev,
                       const struct pci_device_id *ent);
extern void e1000_remove(struct pci_dev *pdev);
extern void e1000_delete(struct e1000_adapter *Adapter);
extern int e1000_open(struct net_device *netdev);
extern int e1000_close(struct net_device *netdev);
extern void e1000_set_multi(struct net_device *netdev);
extern int e1000_xmit_frame(struct sk_buff *skb,
                            struct net_device *netdev);
extern struct net_device_stats *e1000_get_stats(struct net_device *netdev);
extern int e1000_change_mtu(struct net_device *netdev,
                            int new_mtu);
extern int e1000_set_mac(struct net_device *netdev,
                         void *p);
extern void e1000_intr(int irq,
                       void *data,
                       struct pt_regs *regs);
extern int e1000_ioctl(struct net_device *netdev,
                       struct ifreq *ifr,
                       int cmd);
extern void e1000_watchdog(unsigned long data);
extern void e1000_diag_ioctl(struct net_device *netdev,
                             struct ifreq *ifr);

#ifdef CONFIG_PROC_FS
#include "e1000_proc.h"
#endif
#ifdef IDIAG
#include "e1000_idiag.h"
#endif
#endif /* _E1000_H_ */
