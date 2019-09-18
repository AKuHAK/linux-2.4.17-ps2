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


#define __E1000_MAIN__
#ifdef IANS
#define _IANS_MAIN_MODULE_C_
#endif
#include "e1000.h"

/* Driver name string */
char e1000_driver_name[] = "e1000";

/* Driver ID string, displayed when loading */
char e1000_driver_string[] = "Intel(R) PRO/1000 Network Driver";

/* Driver version */
char e1000_driver_version[] = "4.1.7";

/* Copyright string, displayed when loading */
char e1000_copyright[] = "Copyright (c) 1999-2002 Intel Corporation.";

/* Linked list of board private structures for all NICs found */
struct e1000_adapter *e1000_adapter_list = NULL;

/* e1000_strings - PCI Device ID Table
 *
 * for selecting devices to load on
 * private driver_data field (last one) stores an index
 * into e1000_strings
 * Wildcard entries (PCI_ANY_ID) should come last
 * Last entry must be all 0s
 *
 * { Vendor ID, Device ID, SubVendor ID, SubDevice ID,
 *   Class, Class Mask, String Index }
 */
static struct pci_device_id e1000_pci_table[] = {
    /* Intel(R) PRO/1000 Network Connection */
    {0x8086, 0x1000, 0x8086, 0x1000, 0, 0, 0},
    {0x8086, 0x1001, 0x8086, 0x1003, 0, 0, 0},
    {0x8086, 0x1004, 0x8086, 0x1004, 0, 0, 0},
    {0x8086, 0x1008, 0x8086, 0x1107, 0, 0, 0},
    {0x8086, 0x1009, 0x8086, 0x1109, 0, 0, 0},
    {0x8086, 0x100C, 0x8086, 0x1112, 0, 0, 0},
    {0x8086, 0x100E, 0x8086, 0x001E, 0, 0, 0},
    /* Compaq Gigabit Ethernet Server Adapter */
    {0x8086, 0x1000, 0x0E11, PCI_ANY_ID, 0, 0, 1},
    {0x8086, 0x1001, 0x0E11, PCI_ANY_ID, 0, 0, 1},
    {0x8086, 0x1004, 0x0E11, PCI_ANY_ID, 0, 0, 1},
    /* IBM Mobile, Desktop & Server Adapters */
    {0x8086, 0x1000, 0x1014, PCI_ANY_ID, 0, 0, 2},
    {0x8086, 0x1001, 0x1014, PCI_ANY_ID, 0, 0, 2},
    {0x8086, 0x1004, 0x1014, PCI_ANY_ID, 0, 0, 2},
    /* Generic */
    {0x8086, 0x1000, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x1001, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x1004, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x1008, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x1009, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x100C, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x100D, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x100E, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    /* required last entry */
    {0,}
};

MODULE_DEVICE_TABLE(pci, e1000_pci_table);

/* e1000_pci_table - Table of branding strings for all supported NICs. */

static char *e1000_strings[] = {
    "Intel(R) PRO/1000 Network Connection",
    "Compaq Gigabit Ethernet Server Adapter",
    "IBM Mobile, Desktop & Server Adapters"
};

/* PCI driver information (Linux 2.4 driver API) */
static struct pci_driver e1000_driver = {
    name:     e1000_driver_name,
    id_table: e1000_pci_table,
    probe:    e1000_probe,
    remove:   e1000_remove,
    /* Power Managment Hooks */
    suspend:  NULL,
    resume:   NULL
};

/* Module Parameters are always initialized to -1, so that the driver
 * can tell the difference between no user specified value or the
 * user asking for the default value.
 * The true default values are loaded in when e1000_check_options is called.
 */

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */

#define E1000_MAX_NIC 8

/* This is a GCC extension to ANSI C.
 * See the item "Labeled Elements in Initializers" in the section
 * "Extensions to the C Language Family" of the GCC documentation.
 */

#define E1000_OPTION_INIT { [0 ... E1000_MAX_NIC] = OPTION_UNSET }

/* Transmit Descriptor Count
 *
 * Valid Range: 80-256 for 82542 and 82543 gigabit ethernet controllers
 * Valid Range: 80-4096 for 82544
 *
 * Default Value: 256
 */

static int TxDescriptors[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* Receive Descriptor Count
 *
 * Valid Range: 80-256 for 82542 and 82543 gigabit ethernet controllers
 * Valid Range: 80-4096 for 82544
 *
 * Default Value: 256
 */

static int RxDescriptors[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* User Specified Speed Override
 *
 * Valid Range: 0, 10, 100, 1000
 *  - 0    - auto-negotiate at all supported speeds
 *  - 10   - only link at 10 Mbps
 *  - 100  - only link at 100 Mbps
 *  - 1000 - only link at 1000 Mbps
 *
 * Default Value: 0
 */

static int Speed[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* User Specified Duplex Override
 *
 * Valid Range: 0-2
 *  - 0 - auto-negotiate for duplex
 *  - 1 - only link at half duplex
 *  - 2 - only link at full duplex
 *
 * Default Value: 0
 */

static int Duplex[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* Auto-negotiation Advertisement Override
 *
 * Valid Range: 0x00-0x0F, 0x20-0x2F
 *
 * The AutoNeg value is a bit mask describing which speed and duplex
 * combinations should be advertised during auto-negotiation.
 * The supported speed and duplex modes are listed below
 *
 * Bit           7     6     5      4      3     2     1      0
 * Speed (Mbps)  N/A   N/A   1000   N/A    100   100   10     10
 * Duplex                    Full          Full  Half  Full   Half
 *
 * Default Value: 0x2F
 */

static int AutoNeg[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* User Specified Flow Control Override
 *
 * Valid Range: 0-3
 *  - 0 - No Flow Control
 *  - 1 - Rx only, respond to PAUSE frames but do not generate them
 *  - 2 - Tx only, generate PAUSE frames but ignore them on receive
 *  - 3 - Full Flow Control Support
 *
 * Default Value: Read flow control settings from the EEPROM
 */

static int FlowControl[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* XsumRX - Receive Checksum Offload Enable/Disable
 *
 * Valid Range: 0, 1
 *  - 0 - disables all checksum offload
 *  - 1 - enables receive IP/TCP/UDP checksum offload
 *        on 82543 based NICs
 *
 * Default Value: 1
 */

static int XsumRX[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* Transmit Interrupt Delay in units of 1.024 microseconds
 *
 * Valid Range: 0-65535
 *
 * Default Value: 64
 */

static int TxIntDelay[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* Receive Interrupt Delay in units of 1.024 microseconds
 *
 * Valid Range: 0-65535
 *
 * Default Value: 64
 */

static int RxIntDelay[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* MDI-X Support Enable/Disable - Applies only to Copper PHY
 *
 * Valid Range: 0, 3
 *  - 0 - Auto in all modes
 *  - 1 - MDI
 *  - 2 - MDI-X
 *  - 3 - Auto in 1000 Base-T mode (MDI in 10 Base-T and 100 Base-T)
 *
 * Default Value: 0 (Auto)
 */

static int MdiX[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

/* Automatic Correction of Reversed Cable Polarity Enable/Disable
 * This setting applies only to Copper PHY
 *
 * Valid Range: 0, 1
 *  - 0 - Disabled
 *  - 1 - Enabled
 *
 * Default Value: 1 (Enabled)
 */

static int DisablePolarityCorrection[E1000_MAX_NIC + 1] = E1000_OPTION_INIT;

#ifdef MODULE

MODULE_AUTHOR("Intel Corporation, <linux.nics@intel.com>");
MODULE_DESCRIPTION("Intel(R) PRO/1000 Network Driver");

#if defined(MODULE_LICENSE)
MODULE_LICENSE("BSD with patent grant");
#endif

MODULE_PARM(TxDescriptors, "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(RxDescriptors, "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(Speed,         "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(Duplex,        "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(AutoNeg,       "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(XsumRX,        "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(FlowControl,   "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(TxIntDelay,    "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(RxIntDelay,    "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(MdiX,          "1-" __MODULE_STRING(E1000_MAX_NIC) "i");
MODULE_PARM(DisablePolarityCorrection, "1-" __MODULE_STRING(E1000_MAX_NIC) "i");

MODULE_PARM_DESC(TxDescriptors, "Number of transmit descriptors");
MODULE_PARM_DESC(RxDescriptors, "Number of receive descriptors");
MODULE_PARM_DESC(Speed,         "Speed setting");
MODULE_PARM_DESC(Duplex,        "Duplex setting");
MODULE_PARM_DESC(AutoNeg,       "Advertised auto-negotiation setting");
MODULE_PARM_DESC(XsumRX,        "Disable or enable Receive Checksum offload");
MODULE_PARM_DESC(FlowControl,   "Flow Control setting");
MODULE_PARM_DESC(TxIntDelay,    "Transmit Interrupt Delay");
MODULE_PARM_DESC(RxIntDelay,    "Receive Interrupt Delay");
MODULE_PARM_DESC(MdiX,          "Set MDI/MDI-X Mode");
MODULE_PARM_DESC(DisablePolarityCorrection,
                 "Disable or enable Automatic Correction for Reversed Cable Polarity");

#ifdef EXPORT_SYMTAB
EXPORT_SYMBOL(e1000_init_module);
EXPORT_SYMBOL(e1000_exit_module);
EXPORT_SYMBOL(e1000_probe);
EXPORT_SYMBOL(e1000_remove);
EXPORT_SYMBOL(e1000_open);
EXPORT_SYMBOL(e1000_close);
EXPORT_SYMBOL(e1000_xmit_frame);
EXPORT_SYMBOL(e1000_intr);
EXPORT_SYMBOL(e1000_set_multi);
EXPORT_SYMBOL(e1000_change_mtu);
EXPORT_SYMBOL(e1000_set_mac);
EXPORT_SYMBOL(e1000_get_stats);
EXPORT_SYMBOL(e1000_watchdog);
EXPORT_SYMBOL(e1000_ioctl);
EXPORT_SYMBOL(e1000_adapter_list);
#endif

#endif

/* Local Function Prototypes */

static void e1000_check_options(struct e1000_adapter *adapter);
static void e1000_check_fiber_options(struct e1000_adapter *adapter);
static void e1000_check_copper_options(struct e1000_adapter *adapter);
static int e1000_sw_init(struct e1000_adapter *adapter);
static int e1000_hw_init(struct e1000_adapter *adapter);
static void e1000_read_address(struct e1000_adapter *adapter,
                               uint8_t *addr);
static int e1000_setup_tx_resources(struct e1000_adapter *adapter);
static int e1000_setup_rx_resources(struct e1000_adapter *adapter);
static void e1000_setup_rctl(struct e1000_adapter *adapter);
static void e1000_configure_rx(struct e1000_adapter *adapter);
static void e1000_configure_tx(struct e1000_adapter *adapter);
static void e1000_free_tx_resources(struct e1000_adapter *adapter);
static void e1000_free_rx_resources(struct e1000_adapter *adapter);
static void e1000_update_stats(struct e1000_adapter *adapter);
static inline void e1000_irq_disable(struct e1000_adapter *adapter);
static inline void e1000_irq_enable(struct e1000_adapter *adapter);
static void e1000_clean_tx_irq(struct e1000_adapter *adapter);
static void e1000_clean_rx_irq(struct e1000_adapter *adapter);
static inline void e1000_rx_checksum(struct e1000_adapter *adapter,
                                     struct e1000_rx_desc *rx_desc,
                                     struct sk_buff *skb);
static void e1000_alloc_rx_buffers(unsigned long data);
static void e1000_clean_tx_ring(struct e1000_adapter *adapter);
static void e1000_clean_rx_ring(struct e1000_adapter *adapter);
void e1000_hibernate_adapter(struct net_device *netdev);
void e1000_wakeup_adapter(struct net_device *netdev);
static void e1000_enable_WOL(struct e1000_adapter *adapter);

#ifdef SIOCETHTOOL
static int e1000_ethtool_ioctl(struct net_device *netdev,
                               struct ifreq *ifr);
#endif
#ifdef IDIAG
static int e1000_check_lbtest_frame(struct sk_buff *skb,
                                    unsigned int frame_size);
#endif

/**
 * e1000_init_module - Driver Registration Routine
 *
 * e1000_init_module is the first routine called when the driver is
 * loaded. All it does is register with the PCI subsystem.
 **/

int
e1000_init_module()
{
    E1000_DBG("e1000_init_module\n");

    /* Print the driver ID string and copyright notice */

    printk("%s - version %s\n%s\n", e1000_driver_string, e1000_driver_version,
           e1000_copyright);

    /* register the driver with the PCI subsystem */

    return pci_module_init(&e1000_driver);
}

/* this macro creates a special symbol in the object file that
 * identifies the driver initialization routine
 */
module_init(e1000_init_module);

/**
 * e1000_exit_module - Driver Exit Cleanup Routine
 *
 * e1000_exit_module is called just before the driver is removed
 * from memory.
 **/

void
e1000_exit_module()
{
#ifdef CONFIG_PROC_FS
    struct proc_dir_entry *de;
#endif

    E1000_DBG("e1000_exit_module\n");

    pci_unregister_driver(&e1000_driver);

#ifdef CONFIG_PROC_FS
    /* if there is no e1000_proc_dir (proc creation failure on load)
     * then we're done
     */
    if(e1000_proc_dir == NULL)
        return;

    /* If ADAPTERS_PROC_DIR (/proc/net/PRO_LAN_Adapters) is empty
     * it can be removed now (might still be in use by e100)
     */
    for(de = e1000_proc_dir->subdir; de; de = de->next) {

        /* ignore . and .. */

        if(*(de->name) == '.')
            continue;
        break;
    }
    if(de)
        return;
    remove_proc_entry(ADAPTERS_PROC_DIR, proc_net);
#endif

    return;
}

/* this macro creates a special symbol in the object file that
 * identifies the driver cleanup routine
 */
module_exit(e1000_exit_module);

/**
 * e1000_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in e1000_pci_table
 *
 * Returns 0 on success, negative on failure
 *
 * e1000_probe initializes an adapter identified by a pci_dev
 * structure.  The OS initialization is handled here, and
 * e1000_sw_init and e1000_hw_init are called to handle the driver
 * specific software structures and hardware initialization
 * respectively.
 **/

int
e1000_probe(struct pci_dev *pdev,
            const struct pci_device_id *ent)
{
    struct net_device *netdev = NULL;
    struct e1000_adapter *adapter;
    static int cards_found = 0;

#ifdef CONFIG_PROC_FS
    int len;
#endif

    E1000_DBG("e1000_probe\n");

    /* Make sure the PCI device has the proper resources available */

    if(pci_enable_device(pdev) != 0) {
        E1000_ERR("pci_enable_device failed\n");
        return -ENODEV;
    }

    /* Make sure we are enabled as a bus mastering device */

    pci_set_master(pdev);

    /* Check to see if our PCI addressing needs are supported */
    if(pci_set_dma_mask(pdev, E1000_DMA_MASK) < 0) {
        E1000_ERR("PCI DMA not supported by the system\n");
        return -ENODEV;
    }

    /* Allocate private data structure (struct e1000_adapter)
     */
    netdev = alloc_etherdev(sizeof(struct e1000_adapter));

    if(netdev == NULL) {
        E1000_ERR("Unable to allocate net_device struct\n");
        return -ENOMEM;
    }

    /* Calling alloc_etherdev with sizeof(struct e1000_adapter) allocates
     * a single buffer of size net_device + struct e1000_adapter +
     * alignment. If this is not done then the struct e1000_adapter needs
     * to be allocated and freed separately.
     */
    adapter = (struct e1000_adapter *) netdev->priv;
    memset(adapter, 0, sizeof(struct e1000_adapter));
    adapter->netdev = netdev;
    adapter->pdev = pdev;

    /* link the struct e1000_adapter into the list */

    if(e1000_adapter_list != NULL)
        e1000_adapter_list->prev = adapter;
    adapter->next = e1000_adapter_list;
    e1000_adapter_list = adapter;
    adapter->shared.back = (void *) adapter;

    /* reserve the MMIO region as ours */

    if(!request_mem_region
       (pci_resource_start(pdev, BAR_0), pci_resource_len(pdev, BAR_0),
        e1000_driver_name)) {
        E1000_ERR("request_mem_region failed\n");
        e1000_remove(pdev);
        return -ENODEV;
    }

    /* map the MMIO region into the kernel virtual address space */

    adapter->shared.hw_addr =
        ioremap(pci_resource_start(pdev, BAR_0), pci_resource_len(pdev, BAR_0));

    if(adapter->shared.hw_addr == NULL) {
        E1000_ERR("ioremap failed\n");
        release_mem_region(pci_resource_start(pdev, BAR_0),
                           pci_resource_len(pdev, BAR_0));
        e1000_remove(pdev);
        return -ENOMEM;
    }

    /* don't actually register the interrupt handler until e1000_open */

    netdev->irq = pdev->irq;

    /* Set the MMIO base address for the NIC */

#ifdef IANS
    netdev->base_addr = pci_resource_start(pdev, BAR_0);
#endif
    netdev->mem_start = pci_resource_start(pdev, BAR_0);
    netdev->mem_end = netdev->mem_start + pci_resource_len(pdev, BAR_0);

    /* set up function pointers to driver entry points */

    netdev->open = &e1000_open;
    netdev->stop = &e1000_close;
    netdev->hard_start_xmit = &e1000_xmit_frame;
    netdev->get_stats = &e1000_get_stats;
    netdev->set_multicast_list = &e1000_set_multi;
    netdev->set_mac_address = &e1000_set_mac;
    netdev->change_mtu = &e1000_change_mtu;
    netdev->do_ioctl = &e1000_ioctl;

    /* set up the struct e1000_adapter */

    adapter->bd_number = cards_found;
    adapter->id_string = e1000_strings[ent->driver_data];
    printk("\n%s\n", adapter->id_string);

    /* Order is important here.  e1000_sw_init also identifies the
     * hardware, so that e1000_check_options can treat command line parameters
     * differently depending on the hardware.
     */
    e1000_sw_init(adapter);
    e1000_check_options(adapter);

#ifdef MAX_SKB_FRAGS
    if(adapter->shared.mac_type >= e1000_82543) {
        netdev->features = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_HIGHDMA;
    } else {
        netdev->features = NETIF_F_SG | NETIF_F_HIGHDMA;
    }
#endif

#ifdef IANS
    adapter->iANSdata = kmalloc(sizeof(iANSsupport_t), GFP_KERNEL);
    if(adapter->iANSdata == NULL) {
        e1000_remove(pdev);
        return -ENOMEM;
    }
    memset(adapter->iANSdata, 0, sizeof(iANSsupport_t));
    bd_ans_drv_InitANS(adapter, adapter->iANSdata);
#endif

    /* finally, we get around to setting up the hardware */

    if(e1000_hw_init(adapter) < 0) {
        e1000_remove(pdev);
        return -ENODEV;
    }
    cards_found++;

    /* reset stats */

    e1000_clear_hw_cntrs(&adapter->shared);
    e1000_phy_get_info(&adapter->shared, &adapter->phy_info);

    /* Then register the net device once everything initializes
     */
    register_netdev(netdev);

#ifdef CONFIG_PROC_FS
    /* set up the proc fs entry */

    len = strlen(ADAPTERS_PROC_DIR);

    for(e1000_proc_dir = proc_net->subdir; e1000_proc_dir;
        e1000_proc_dir = e1000_proc_dir->next) {
        if((e1000_proc_dir->namelen == len) &&
           (memcmp(e1000_proc_dir->name, ADAPTERS_PROC_DIR, len) == 0))
            break;
    }

    if(e1000_proc_dir == NULL)
        e1000_proc_dir =
            create_proc_entry(ADAPTERS_PROC_DIR, S_IFDIR, proc_net);

    if(e1000_proc_dir != NULL)
        if(e1000_create_proc_dev(adapter) < 0) {
            e1000_remove_proc_dev(adapter->netdev);
        }
#endif

    /* print the link status */

    if(adapter->link_active == 1)
        printk("%s:  Mem:0x%p  IRQ:%d  Speed:%d Mbps  Duplex:%s\n",
               netdev->name, (void *) netdev->mem_start, netdev->irq,
               adapter->link_speed,
               adapter->link_duplex == FULL_DUPLEX ? "Full" : "Half");
    else
        printk("%s:  Mem:0x%p  IRQ:%d  Speed:N/A  Duplex:N/A\n", netdev->name,
               (void *) netdev->mem_start, netdev->irq);

    return 0;
}

/**
 * e1000_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * e1000_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 *
 * This routine is also called to clean up from a failure in
 * e1000_probe.  The Adapter struct and netdev will always exist,
 * all other pointers must be checked for NULL before freeing.
 **/

void
e1000_remove(struct pci_dev *pdev)
{
    struct net_device *netdev;
    struct e1000_adapter *adapter;

    /* find the Adapter struct that matches this PCI device */

    for(adapter = e1000_adapter_list; adapter != NULL; adapter = adapter->next) {
        if(adapter->pdev == pdev)
            break;
    }
    if(adapter == NULL)
        return;

    netdev = adapter->netdev;

    /* this must be called before freeing anything,
     * otherwise there is a case where the open entry point can be
     * running at the same time as remove. Calling unregister_netdev on an
     * open interface results in a call to dev_close, which locks
     * properly against the other netdev entry points, so this takes
     * care of the hotplug issue of removing an active interface as well.
     */
    unregister_netdev(netdev);

    e1000_phy_hw_reset(&adapter->shared);

#ifdef CONFIG_PROC_FS
    /* remove the proc nodes */

    if(e1000_proc_dir != NULL)
        e1000_remove_proc_dev(adapter->netdev);
#endif

    /* remove from the adapter list */

    if(e1000_adapter_list == adapter)
        e1000_adapter_list = adapter->next;
    if(adapter->next != NULL)
        adapter->next->prev = adapter->prev;
    if(adapter->prev != NULL)
        adapter->prev->next = adapter->next;

    /* free system resources */

#ifdef IANS
    if(adapter->iANSdata != NULL)
        kfree(adapter->iANSdata);
#endif

    if(adapter->shared.hw_addr != NULL) {
        iounmap((void *) adapter->shared.hw_addr);
        release_mem_region(pci_resource_start(pdev, BAR_0),
                           pci_resource_len(pdev, BAR_0));
    }

    /* free the net_device _and_ struct e1000_adapter memory */

    kfree(netdev);

    return;
}

/**
 * e1000_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 *
 * This routine checks all command line paramters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the Adapter structure.
 **/

static void
e1000_check_options(struct e1000_adapter *adapter)
{
    int board = adapter->bd_number;

    if(board >= E1000_MAX_NIC) {
        printk("Warning: no configuration for board #%i\n", board);
        printk("Using defaults for all values\n");
        board = E1000_MAX_NIC;
    }

    E1000_DBG("e1000_check_options\n");

    /* Transmit Descriptor Count */

    if(TxDescriptors[board] == OPTION_UNSET) {
        adapter->tx_ring.count = DEFAULT_TXD;
        TxDescriptors[board] = DEFAULT_TXD;
    } else
        if(((TxDescriptors[board] > MAX_TXD) ||
            (TxDescriptors[board] < MIN_TXD)) &&
           (adapter->shared.mac_type <= e1000_82543)) {
        printk("Invalid TxDescriptors specified (%i), using default %i\n",
               TxDescriptors[board], DEFAULT_TXD);
        adapter->tx_ring.count = DEFAULT_TXD;
    } else
        if(((TxDescriptors[board] > MAX_82544_TXD) ||
            (TxDescriptors[board] < MIN_TXD)) &&
           (adapter->shared.mac_type > e1000_82543)) {
        printk("Invalid TxDescriptors specified (%i), using default %i\n",
               TxDescriptors[board], DEFAULT_TXD);
        adapter->tx_ring.count = DEFAULT_TXD;
    } else {
        printk("Using specified value of %i TxDescriptors\n",
               TxDescriptors[board]);
        adapter->tx_ring.count = TxDescriptors[board];
    }

    /* tx_ring.count must be a multiple of 8 */

    adapter->tx_ring.count = E1000_ROUNDUP2(adapter->tx_ring.count,
                                            REQ_TX_DESCRIPTOR_MULTIPLE);

    /* Receive Descriptor Count */

    if(RxDescriptors[board] == OPTION_UNSET) {
        adapter->rx_ring.count = DEFAULT_RXD;
        RxDescriptors[board] = DEFAULT_RXD;
    } else
        if(((RxDescriptors[board] > MAX_RXD) ||
            (RxDescriptors[board] < MIN_RXD)) &&
           (adapter->shared.mac_type <= e1000_82543)) {
        printk("Invalid RxDescriptors specified (%i), using default %i\n",
               RxDescriptors[board], DEFAULT_RXD);
        adapter->rx_ring.count = DEFAULT_RXD;
    } else
        if(((RxDescriptors[board] > MAX_82544_RXD) ||
            (RxDescriptors[board] < MIN_RXD)) &&
           (adapter->shared.mac_type > e1000_82543)) {
        printk("Invalid RxDescriptors specified (%i), using default %i\n",
               RxDescriptors[board], DEFAULT_RXD);
        adapter->rx_ring.count = DEFAULT_RXD;
    } else {
        printk("Using specified value of %i RxDescriptors\n",
               RxDescriptors[board]);
        adapter->rx_ring.count = RxDescriptors[board];
    }

    /* rx_ring.count must be a multiple of 8 */

    adapter->rx_ring.count =
        E1000_ROUNDUP2(adapter->rx_ring.count, REQ_RX_DESCRIPTOR_MULTIPLE);

    /* Receive Checksum Offload Enable */

    if(XsumRX[board] == OPTION_UNSET) {
        adapter->RxChecksum = XSUMRX_DEFAULT;
        XsumRX[board] = XSUMRX_DEFAULT;
    } else if((XsumRX[board] != OPTION_ENABLED) &&
              (XsumRX[board] != OPTION_DISABLED)) {
        printk("Invalid XsumRX specified (%i), using default of %i\n",
               XsumRX[board], XSUMRX_DEFAULT);
        adapter->RxChecksum = XSUMRX_DEFAULT;
    } else {
        printk("Receive Checksum Offload %s\n",
               XsumRX[board] == OPTION_ENABLED ? "Enabled" : "Disabled");
        adapter->RxChecksum = XsumRX[board];
    }

    /* Flow Control */

    if(FlowControl[board] == OPTION_UNSET) {
        adapter->shared.fc = e1000_fc_default;
        FlowControl[board] = e1000_fc_default;
    } else if((FlowControl[board] > e1000_fc_full) ||
              (FlowControl[board] < e1000_fc_none)) {
        printk("Invalid FlowControl specified (%i), "
               "reading default settings from the EEPROM\n",
               FlowControl[board]);
        adapter->shared.fc = e1000_fc_default;
    } else {
        adapter->shared.fc = FlowControl[board];
        switch (adapter->shared.fc) {
        case e1000_fc_none:
            printk("Flow Control Disabled\n");
            break;
        case e1000_fc_rx_pause:
            printk("Flow Control Receive Only\n");
            break;
        case e1000_fc_tx_pause:
            printk("Flow Control Transmit Only\n");
            break;
        case e1000_fc_full:
            printk("Flow Control Enabled\n");
        case e1000_fc_default:
            printk("Flow Control Hardware Default\n");
        }
    }

    /* Transmit Interrupt Delay */

    if(TxIntDelay[board] == OPTION_UNSET) {
        adapter->tx_int_delay = DEFAULT_TIDV;
        TxIntDelay[board] = DEFAULT_TIDV;
    } else if((TxIntDelay[board] > MAX_TIDV) || (TxIntDelay[board] < MIN_TIDV)) {
        printk("Invalid TxIntDelay specified (%i), using default %i\n",
               TxIntDelay[board], DEFAULT_TIDV);
        adapter->tx_int_delay = DEFAULT_TIDV;
    } else {
        printk("Using specified TxIntDelay of %i\n", TxIntDelay[board]);
        adapter->tx_int_delay = TxIntDelay[board];
    }

    /* Receive Interrupt Delay */

    if(RxIntDelay[board] == OPTION_UNSET) {
        adapter->rx_int_delay = DEFAULT_RIDV;
        RxIntDelay[board] = DEFAULT_RIDV;
    } else if((RxIntDelay[board] > MAX_RIDV) || (RxIntDelay[board] < MIN_RIDV)) {
        printk("Invalid RxIntDelay specified (%i), using default %i\n",
               RxIntDelay[board], DEFAULT_RIDV);
        adapter->rx_int_delay = DEFAULT_RIDV;
    } else {
        printk("Using specified RxIntDelay of %i\n", RxIntDelay[board]);
        adapter->rx_int_delay = RxIntDelay[board];
    }

    if(adapter->shared.media_type == e1000_media_type_copper) {
        /* MDI/MDI-X Support */

        if(MdiX[board] == OPTION_UNSET) {
            adapter->shared.mdix = DEFAULT_MDIX;
            MdiX[board] = DEFAULT_MDIX;
        } else if((MdiX[board] > MAX_MDIX) || (MdiX[board] < MIN_MDIX)) {
            printk("Invalid MDI/MDI-X specified (%i), using default %i\n",
                   MdiX[board], DEFAULT_MDIX);
            adapter->shared.mdix = DEFAULT_MDIX;
        } else {
            printk("Using specified MDI/MDI-X of %i\n", MdiX[board]);
            adapter->shared.mdix = MdiX[board];
        }

        /* Automatic Correction for Reverse Cable Polarity */

        if(DisablePolarityCorrection[board] == OPTION_UNSET) {
            adapter->shared.disable_polarity_correction = OPTION_DISABLED;
            DisablePolarityCorrection[board] = OPTION_DISABLED;
        } else if((DisablePolarityCorrection[board] != OPTION_ENABLED) &&
                  (DisablePolarityCorrection[board] != OPTION_DISABLED)) {
            printk("Invalid polarity correction specified (%i),"
                   "    using default %i\n", DisablePolarityCorrection[board],
                   OPTION_DISABLED);
            adapter->shared.disable_polarity_correction = OPTION_DISABLED;
        } else {
            printk("Using specified polarity correction of %i\n",
                   DisablePolarityCorrection[board]);
            adapter->shared.disable_polarity_correction =
                DisablePolarityCorrection[board];
        }
    }

    /* Speed, Duplex, and AutoNeg */

    switch (adapter->shared.media_type) {

    case e1000_media_type_fiber:
        e1000_check_fiber_options(adapter);
        break;

    case e1000_media_type_copper:
        e1000_check_copper_options(adapter);
        break;

    default:
        printk("Unknown Media Type\n");
        break;
    }

    return;
}

/**
 * e1000_check_fiber_options - Range Checking for Link Options, Fiber Version
 * @adapter: board private structure
 *
 * Handles speed and duplex options on fiber based adapters
 **/

static void
e1000_check_fiber_options(struct e1000_adapter *adapter)
{
    int board =
        adapter->bd_number > E1000_MAX_NIC ? E1000_MAX_NIC : adapter->bd_number;

    E1000_DBG("CheckSpeedDuplexFiber\n");

    /* Speed, Duplex, and AutoNeg are not valid on fiber NICs */

    if((Speed[board] != OPTION_UNSET)) {
        Speed[board] = 0;
        printk("Warning: Speed not valid for fiber adapters\n");
        printk("Speed Parameter Ignored\n");
    }
    if((Duplex[board] != OPTION_UNSET)) {
        Duplex[board] = 0;
        printk("Warning: Duplex not valid for fiber adapters\n");
        printk("Duplex Parameter Ignored\n");
    }
    if((AutoNeg[board] != OPTION_UNSET)) {
        AutoNeg[board] = AUTONEG_ADV_DEFAULT;
        printk("Warning: AutoNeg not valid for fiber adapters\n");
        printk("AutoNeg Parameter Ignored\n");
    }

    return;
}

/**
 * e1000_check_copper_options - Range Checking for Link Options, Copper Version
 * @adapter: board private structure
 *
 * Handles speed and duplex options on copper based adapters
 **/

static void
e1000_check_copper_options(struct e1000_adapter *adapter)
{
    int board =
        adapter->bd_number > E1000_MAX_NIC ? E1000_MAX_NIC : adapter->bd_number;
    int speed, duplex;
    boolean_t all_default = TRUE;

    E1000_DBG("CheckSpeedDuplexCopper\n");

    /* User Specified Auto-negotiation Settings */

    if(AutoNeg[board] == OPTION_UNSET) {

        adapter->shared.autoneg_advertised = AUTONEG_ADV_DEFAULT;
        AutoNeg[board] = AUTONEG_ADV_DEFAULT;

    } else if((Speed[board] != 0 && Speed[board] != OPTION_UNSET) ||
              (Duplex[board] != 0 && Duplex[board] != OPTION_UNSET)) {

        printk("Warning: AutoNeg specified along with Speed or Duplex\n");
        printk("AutoNeg Parameter Ignored\n");

        adapter->shared.autoneg_advertised = AUTONEG_ADV_DEFAULT;

    } else {

        if(AutoNeg[board] & ~AUTONEG_ADV_MASK) {

            printk("Invalid AutoNeg Specified (0x%X), Parameter Ignored\n",
                   AutoNeg[board]);

            adapter->shared.autoneg_advertised = AUTONEG_ADV_DEFAULT;

        } else {

            adapter->shared.autoneg_advertised = AutoNeg[board];
        }

        printk("AutoNeg Advertising ");
        if(adapter->shared.autoneg_advertised & ADVERTISE_1000_FULL) {
            printk("1000/FD");
            if(adapter->shared.autoneg_advertised & (ADVERTISE_1000_FULL - 1))
                printk(", ");
        }
        if(adapter->shared.autoneg_advertised & ADVERTISE_1000_HALF) {
            printk("1000/HD");
            if(adapter->shared.autoneg_advertised & (ADVERTISE_1000_HALF - 1))
                printk(", ");
        }
        if(adapter->shared.autoneg_advertised & ADVERTISE_100_FULL) {
            printk("100/FD");
            if(adapter->shared.autoneg_advertised & (ADVERTISE_100_FULL - 1))
                printk(", ");
        }
        if(adapter->shared.autoneg_advertised & ADVERTISE_100_HALF) {
            printk("100/HD");
            if(adapter->shared.autoneg_advertised & (ADVERTISE_100_HALF - 1))
                printk(", ");
        }
        if(adapter->shared.autoneg_advertised & ADVERTISE_10_FULL) {
            printk("10/FD");
            if(adapter->shared.autoneg_advertised & (ADVERTISE_10_FULL - 1))
                printk(", ");
        }
        if(adapter->shared.autoneg_advertised & ADVERTISE_10_HALF)
            printk("10/HD");
        printk("\n");
    }

    /* Forced Speed and Duplex */

    switch (Speed[board]) {
    default:
        printk("Invalid Speed Specified (%i), Parameter Ignored\n",
               Speed[board]);
        all_default = FALSE;
    case OPTION_UNSET:
        speed = 0;
        Speed[board] = 0;
        break;
    case 0:
    case 10:
    case 100:
    case 1000:
        speed = Speed[board];
        all_default = FALSE;
        break;
    }

    switch (Duplex[board]) {
    default:
        printk("Invalid Duplex Specified (%i), Parameter Ignored\n",
               Duplex[board]);
        all_default = FALSE;
    case OPTION_UNSET:
        duplex = 0;
        Duplex[board] = 0;
        break;
    case 0:
    case 1:
    case 2:
        duplex = Duplex[board];
        all_default = FALSE;
        break;
    }

    switch (speed + duplex) {
    case 0:
        if(all_default == FALSE)
            printk("Speed and Duplex Auto-negotiation Enabled\n");
        adapter->shared.autoneg = 1;
        break;
    case 1:
        printk("Warning: Half Duplex specified without Speed\n");
        printk("Using Auto-negotiation at Half Duplex only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised =
            ADVERTISE_10_HALF | ADVERTISE_100_HALF;
        break;
    case 2:
        printk("Warning: Full Duplex specified without Speed\n");
        printk("Using Auto-negotiation at Full Duplex only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised =
            ADVERTISE_10_FULL | ADVERTISE_100_FULL | ADVERTISE_1000_FULL;
        break;
    case 10:
        printk("Warning: 10 Mbps Speed specified without Duplex\n");
        printk("Using Auto-negotiation at 10 Mbps only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised =
            ADVERTISE_10_HALF | ADVERTISE_10_FULL;
        break;
    case 11:
        printk("Forcing to 10 Mbps Half Duplex\n");
        adapter->shared.autoneg = 0;
        adapter->shared.forced_speed_duplex = e1000_10_half;
        adapter->shared.autoneg_advertised = 0;
        break;
    case 12:
        printk("Forcing to 10 Mbps Full Duplex\n");
        adapter->shared.autoneg = 0;
        adapter->shared.forced_speed_duplex = e1000_10_full;
        adapter->shared.autoneg_advertised = 0;
        break;
    case 100:
        printk("Warning: 100 Mbps Speed specified without Duplex\n");
        printk("Using Auto-negotiation at 100 Mbps only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised =
            ADVERTISE_100_HALF | ADVERTISE_100_FULL;
        break;
    case 101:
        printk("Forcing to 100 Mbps Half Duplex\n");
        adapter->shared.autoneg = 0;
        adapter->shared.forced_speed_duplex = e1000_100_half;
        adapter->shared.autoneg_advertised = 0;
        break;
    case 102:
        printk("Forcing to 100 Mbps Full Duplex\n");
        adapter->shared.autoneg = 0;
        adapter->shared.forced_speed_duplex = e1000_100_full;
        adapter->shared.autoneg_advertised = 0;
        break;
    case 1000:
        printk("Warning: 1000 Mbps Speed specified without Duplex\n");
        printk("Using Auto-negotiation at 1000 Mbps Full Duplex only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised = ADVERTISE_1000_FULL;
        break;
    case 1001:
        printk("Warning: Half Duplex is not supported at 1000 Mbps\n");
        printk("Using Auto-negotiation at 1000 Mbps Full Duplex only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised = ADVERTISE_1000_FULL;
        break;
    case 1002:
        printk("Using Auto-negotiation at 1000 Mbps Full Duplex only\n");
        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised = ADVERTISE_1000_FULL;
        break;
    default:
        panic("something is wrong in e1000_check_copper_options");
    }

    /* Speed, AutoNeg and MDI/MDI-X */
    if (!e1000_validate_mdi_setting(&(adapter->shared))) {
        printk ("Speed, AutoNeg and MDI-X specifications are incompatible."
                " Setting MDI-X to a compatible value.\n");
    }

    return;
}

/**
 * e1000_sw_init - Initialize general software structures (struct e1000_adapter)
 * @adapter: board private structure to initialize
 *
 * Returns 0 on success, negative on failure
 *
 * e1000_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/

static int
e1000_sw_init(struct e1000_adapter *adapter)
{
    struct net_device *netdev = adapter->netdev;
    struct pci_dev *pdev = adapter->pdev;
    uint32_t status;

    E1000_DBG("e1000_sw_init\n");

    /* PCI config space info */

    pci_read_config_word(pdev, PCI_VENDOR_ID, &adapter->vendor_id);
    pci_read_config_word(pdev, PCI_DEVICE_ID, &adapter->device_id);
    pci_read_config_byte(pdev, PCI_REVISION_ID, &adapter->rev_id);
    pci_read_config_word(pdev, PCI_SUBSYSTEM_VENDOR_ID, &adapter->subven_id);
    pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &adapter->subsys_id);
    pci_read_config_word(pdev, PCI_COMMAND, &adapter->shared.pci_cmd_word);
    adapter->shared.vendor_id = adapter->vendor_id;
    adapter->shared.device_id = adapter->device_id;
    adapter->shared.revision_id = adapter->rev_id;
    adapter->shared.subsystem_vendor_id = adapter->subven_id;
    adapter->shared.subsystem_id = adapter->subsys_id;

    /* Initial Receive Buffer Length */

    if((netdev->mtu + ENET_HEADER_SIZE + CRC_LENGTH) < E1000_RXBUFFER_2048)
        adapter->rx_buffer_len = E1000_RXBUFFER_2048;
    else if((netdev->mtu + ENET_HEADER_SIZE + CRC_LENGTH) < E1000_RXBUFFER_4096)
        adapter->rx_buffer_len = E1000_RXBUFFER_4096;
    else if((netdev->mtu + ENET_HEADER_SIZE + CRC_LENGTH) < E1000_RXBUFFER_8192)
        adapter->rx_buffer_len = E1000_RXBUFFER_8192;
    else
        adapter->rx_buffer_len = E1000_RXBUFFER_16384;

    adapter->shared.max_frame_size =
        netdev->mtu + ENET_HEADER_SIZE + CRC_LENGTH;
    adapter->shared.min_frame_size = MINIMUM_ETHERNET_PACKET_SIZE + CRC_LENGTH;

    /* MAC and Phy settings */

    switch (adapter->device_id) {
    case E1000_DEV_ID_82542:
        switch (adapter->rev_id) {
        case E1000_82542_2_0_REV_ID:
            adapter->shared.mac_type = e1000_82542_rev2_0;
            break;
        case E1000_82542_2_1_REV_ID:
            adapter->shared.mac_type = e1000_82542_rev2_1;
            break;
        default:
            adapter->shared.mac_type = e1000_82542_rev2_0;
            E1000_ERR("Could not identify 82542 revision\n");
        }
        break;
    case E1000_DEV_ID_82543GC_FIBER:
    case E1000_DEV_ID_82543GC_COPPER:
        adapter->shared.mac_type = e1000_82543;
        break;
    case E1000_DEV_ID_82544EI_COPPER:
    case E1000_DEV_ID_82544EI_FIBER:
    case E1000_DEV_ID_82544GC_COPPER:
    case E1000_DEV_ID_82544GC_LOM:
        adapter->shared.mac_type = e1000_82544;
        break;
    case E1000_DEV_ID_82540EM:
        adapter->shared.mac_type = e1000_82540;
        break;
    default:
        E1000_ERR("Could not identify hardware\n");
        return -ENODEV;
    }

    adapter->shared.fc_high_water = FC_DEFAULT_HI_THRESH;
    adapter->shared.fc_low_water = FC_DEFAULT_LO_THRESH;
    adapter->shared.fc_pause_time = FC_DEFAULT_TX_TIMER;
    adapter->shared.fc_send_xon = 1;

    /* Identify the Hardware - this is done by the gigabit shared code
     * in e1000_init_hw, but it would help to identify the NIC
     * before bringing the hardware online for use in e1000_check_options.
     */
    if(adapter->shared.mac_type >= e1000_82543) {
        status = E1000_READ_REG(&adapter->shared, STATUS);
        if(status & E1000_STATUS_TBIMODE) {
            adapter->shared.media_type = e1000_media_type_fiber;
        } else {
            adapter->shared.media_type = e1000_media_type_copper;
        }
    } else {
        adapter->shared.media_type = e1000_media_type_fiber;
    }

    if((E1000_REPORT_TX_EARLY == 0) || (E1000_REPORT_TX_EARLY == 1)) {
        adapter->shared.report_tx_early = E1000_REPORT_TX_EARLY;
    } else {
        if(adapter->shared.mac_type < e1000_82543) {

            adapter->shared.report_tx_early = 0;
        } else {
            adapter->shared.report_tx_early = 1;
        }
    }

    adapter->shared.wait_autoneg_complete = WAITFORLINK_DEFAULT;

    adapter->shared.tbi_compatibility_en = 1;

    atomic_set(&adapter->tx_timeout, 0);

    spin_lock_init(&adapter->stats_lock);
    spin_lock_init(&adapter->rx_fill_lock);

    return 0;
}

/**
 * e1000_hw_init - prepare the hardware
 * @adapter: board private struct containing configuration
 *
 * Returns 0 on success, negative on failure
 *
 * Initialize the hardware to a configuration as specified by the
 * Adapter structure.  The controler is reset, the EEPROM is
 * verified, the MAC address is set, then the shared initilization
 * routines are called.
 **/

static int
e1000_hw_init(struct e1000_adapter *adapter)
{
    struct net_device *netdev = adapter->netdev;

    E1000_DBG("e1000_hw_init\n");

    /* Repartition Pba for greater than 9k mtu
     * To take effect Ctrl_Rst is required.
     */
    if(adapter->rx_buffer_len > E1000_RXBUFFER_8192)
        E1000_WRITE_REG(&adapter->shared, PBA, E1000_JUMBO_PBA);
    else
        E1000_WRITE_REG(&adapter->shared, PBA, E1000_DEFAULT_PBA);

    /* Issue a global reset */

    adapter->shared.adapter_stopped = 0;
    e1000_adapter_stop(&adapter->shared);
    adapter->shared.adapter_stopped = 0;

    /* make sure the EEPROM is good */

    if(!e1000_validate_eeprom_checksum(&adapter->shared)) {
        E1000_ERR("The EEPROM Checksum Is Not Valid\n");
        return -1;
    }

    /* copy the MAC address out of the EEPROM */

    e1000_read_address(adapter, adapter->perm_net_addr);
    memcpy(netdev->dev_addr, adapter->perm_net_addr, netdev->addr_len);
    memcpy(adapter->shared.mac_addr, netdev->dev_addr, netdev->addr_len);

    e1000_read_part_num(&adapter->shared, &(adapter->part_num));

    if(!e1000_init_hw(&adapter->shared)) {
        E1000_ERR("Hardware Initialization Failed\n");
        return -1;
    }

    e1000_enable_WOL(adapter);

    adapter->shared.get_link_status = 1;
    e1000_check_for_link(&adapter->shared);

    if(E1000_READ_REG(&adapter->shared, STATUS) & E1000_STATUS_LU)
        adapter->link_active = TRUE;
    else
        adapter->link_active = FALSE;

    if(adapter->link_active == TRUE) {
        e1000_get_speed_and_duplex(&adapter->shared, &adapter->link_speed,
                                   &adapter->link_duplex);
    } else {
        adapter->link_speed = 0;
        adapter->link_duplex = 0;
    }

    e1000_get_bus_info(&adapter->shared);

    return 0;
}

/**
 * e1000_read_address - Reads the MAC address from the EEPROM
 * @adapter: board private structure
 * @addr: pointer to an array of bytes
 **/

static void
e1000_read_address(struct e1000_adapter *adapter,
                   uint8_t *addr)
{
    uint16_t eeprom_word;
    int i;

    E1000_DBG("e1000_read_address\n");

    for(i = 0; i < NODE_ADDRESS_SIZE; i += 2) {
        eeprom_word =
            e1000_read_eeprom(&adapter->shared,
                              EEPROM_NODE_ADDRESS_BYTE_0 + (i / 2));
        addr[i] = (uint8_t) (eeprom_word & 0x00FF);
        addr[i + 1] = (uint8_t) (eeprom_word >> 8);
    }
    return;
}

/**
 * e1000_open - Called when a network interface is made active
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative value on failure
 *
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the watchdog timer is started,
 * and the stack is notified that the interface is ready.
 **/

int
e1000_open(struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;

    E1000_DBG("e1000_open\n");

    /* prevent multiple opens when dealing with iANS */

    if(test_and_set_bit(E1000_BOARD_OPEN, &adapter->flags)) {
        return -EBUSY;
    }

    adapter->shared.fc = adapter->shared.original_fc;

    /* e1000_close issues a global reset (e1000_adapter_stop)
     * so e1000_hw_init must be called again or the hardware
     * will resume in it's default state
     */
    if(e1000_hw_init(adapter) < 0) {
        clear_bit(E1000_BOARD_OPEN, &adapter->flags);
        return -EBUSY;
    }
#ifdef IANS
    /* restore VLAN settings */
    if((IANS_BD_TAGGING_MODE) (ANS_PRIVATE_DATA_FIELD(adapter)->tag_mode) !=
       IANS_BD_TAGGING_NONE)
        bd_ans_hw_EnableVLAN(adapter);
#endif

    adapter->shared.adapter_stopped = 0;

    /* allocate transmit descriptors */

    if(e1000_setup_tx_resources(adapter) != 0) {
        e1000_adapter_stop(&adapter->shared);
        clear_bit(E1000_BOARD_OPEN, &adapter->flags);
        return -ENOMEM;
    }
    e1000_configure_tx(adapter);

    /* allocate receive descriptors and buffers */

    if(e1000_setup_rx_resources(adapter) != 0) {
        e1000_adapter_stop(&adapter->shared);
        e1000_free_tx_resources(adapter);
        clear_bit(E1000_BOARD_OPEN, &adapter->flags);
        return -ENOMEM;
    }
    e1000_setup_rctl(adapter);
    e1000_configure_rx(adapter);

    /* hook the interrupt */

    if(request_irq(netdev->irq, &e1000_intr,
                   SA_SHIRQ, e1000_driver_name, netdev) != 0) {
        clear_bit(E1000_BOARD_OPEN, &adapter->flags);
        e1000_adapter_stop(&adapter->shared);
        e1000_free_tx_resources(adapter);
        e1000_free_rx_resources(adapter);
        clear_bit(E1000_BOARD_OPEN, &adapter->flags);
        return -EBUSY;
    }

    /* fill Rx ring with sk_buffs */

    tasklet_init(&adapter->rx_fill_tasklet, e1000_alloc_rx_buffers,
                 (unsigned long) adapter);

    tasklet_schedule(&adapter->rx_fill_tasklet);

    /* Set the watchdog timer for 2 seconds */

    init_timer(&adapter->timer_id);
    adapter->timer_id.function = &e1000_watchdog;
    adapter->timer_id.data = (unsigned long) netdev;
    mod_timer(&adapter->timer_id, (jiffies + 2 * HZ));

    /* stats accumulated while down are dropped
     * this does not clear the running total
     */

    e1000_clear_hw_cntrs(&adapter->shared);

    adapter->int_mask = IMS_ENABLE_MASK;
    e1000_irq_enable(adapter);
    netif_start_queue(netdev);

#ifdef MODULE

    /* Incrementing the module use count prevents a driver from being
     * unloaded while an active network interface is using it.
     */
    MOD_INC_USE_COUNT;

#endif

    return 0;
}

/**
 * e1000_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/

int
e1000_close(struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;

    E1000_DBG("e1000_close\n");

    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags))
        return 0;

    /* Issue a global reset */

    e1000_adapter_stop((&adapter->shared));

    /* Enable receiver unit after Global reset
     * for WOL, so that receiver can still recive
     * wake up packet and will not drop it.
     */
    if(adapter->shared.mac_type > e1000_82543)
        E1000_WRITE_REG(&adapter->shared, RCTL, E1000_RCTL_EN);

    /* free OS resources */

    netif_stop_queue(netdev);
    free_irq(netdev->irq, netdev);
    del_timer_sync(&adapter->timer_id);

    /* Make sure the tasklet won't be left after ifconfig down */

    /*
     * Assumption: tasklet is ALREADY enabled, ie, t->count == 0.
     * Otherwise, tasklet is still left in the tasklet list, and,
     * tasklet_kill will not be able to return (hang).
     */
    tasklet_kill(&adapter->rx_fill_tasklet);

    /* free software resources */

    e1000_free_tx_resources(adapter);
    e1000_free_rx_resources(adapter);

#ifdef MODULE

    /* decrement the module usage count
     * so that the driver can be unloaded
     */
    MOD_DEC_USE_COUNT;

#endif

    clear_bit(E1000_BOARD_OPEN, &adapter->flags);
    return 0;
}

/**
 * e1000_setup_tx_resources - allocate Tx resources (Descriptors)
 * @adapter: board private structure
 *
 * Return 0 on success, negative on failure
 *
 * e1000_setup_tx_resources allocates all software transmit resources
 * and enabled the Tx unit of the MAC.
 **/

static int
e1000_setup_tx_resources(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;
    int size;

    E1000_DBG("e1000_setup_tx_resources\n");

    size = sizeof(struct e1000_buffer) * adapter->tx_ring.count;
    adapter->tx_ring.buffer_info = kmalloc(size, GFP_KERNEL);
    if(adapter->tx_ring.buffer_info == NULL) {
        return -ENOMEM;
    }
    memset(adapter->tx_ring.buffer_info, 0, size);

    /* round up to nearest 4K */

    adapter->tx_ring.size = E1000_ROUNDUP2(adapter->tx_ring.count *
                                           sizeof(struct e1000_tx_desc),
                                           4096);

    adapter->tx_ring.desc = pci_alloc_consistent(pdev, adapter->tx_ring.size,
                                                 &adapter->tx_ring.dma);
    if(adapter->tx_ring.desc == NULL) {
        kfree(adapter->tx_ring.buffer_info);
        return -ENOMEM;
    }
    memset(adapter->tx_ring.desc, 0, adapter->tx_ring.size);

    atomic_set(&adapter->tx_ring.unused, adapter->tx_ring.count);
    adapter->tx_ring.next_to_use = 0;
    adapter->tx_ring.next_to_clean = 0;

    return 0;
}

/**
 * e1000_configure_tx - Configure 8254x Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/

static void
e1000_configure_tx(struct e1000_adapter *adapter)
{
    uint32_t tctl, tipg;

    /* Setup the Base and Length of the Rx Descriptor Ring */
    /* tx_ring.dma can be either a 32 or 64 bit value */

#if (BITS_PER_LONG == 32)
    E1000_WRITE_REG(&adapter->shared, TDBAL, adapter->tx_ring.dma);
    E1000_WRITE_REG(&adapter->shared, TDBAH, 0);
#elif ( BITS_PER_LONG == 64)
    E1000_WRITE_REG(&adapter->shared, TDBAL,
                    (uint32_t) (adapter->tx_ring.dma & 0x00000000FFFFFFFF));
    E1000_WRITE_REG(&adapter->shared, TDBAH,
                    (uint32_t) (adapter->tx_ring.dma >> 32));
#else
#error "Unsupported System - does not use 32 or 64 bit pointers!"
#endif

    E1000_WRITE_REG(&adapter->shared, TDLEN,
                    adapter->tx_ring.count * sizeof(struct e1000_tx_desc));

    /* Setup the HW Tx Head and Tail descriptor pointers */

    E1000_WRITE_REG(&adapter->shared, TDH, 0);
    E1000_WRITE_REG(&adapter->shared, TDT, 0);

    /* Set the default values for the Tx Inter Packet Gap timer */

    switch (adapter->shared.mac_type) {
    case e1000_82543:
    case e1000_82544:
    case e1000_82540:
        if(adapter->shared.media_type == e1000_media_type_fiber)
            tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
        else
            tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
        tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
        tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
        break;
    case e1000_82542_rev2_0:
    case e1000_82542_rev2_1:
    default:
        tipg = DEFAULT_82542_TIPG_IPGT;
        tipg |= DEFAULT_82542_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
        tipg |= DEFAULT_82542_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
        break;
    }
    E1000_WRITE_REG(&adapter->shared, TIPG, tipg);

    /* Set the Tx Interrupt Delay register */

    E1000_WRITE_REG(&adapter->shared, TIDV, adapter->tx_int_delay);

    /* Program the Transmit Control Register */

    tctl =
        E1000_TCTL_PSP | E1000_TCTL_EN | (E1000_COLLISION_THRESHOLD <<
                                          E1000_CT_SHIFT);
    if(adapter->link_duplex == FULL_DUPLEX) {
        tctl |= E1000_FDX_COLLISION_DISTANCE << E1000_COLD_SHIFT;
    } else {
        tctl |= E1000_HDX_COLLISION_DISTANCE << E1000_COLD_SHIFT;
    }
    E1000_WRITE_REG(&adapter->shared, TCTL, tctl);

#ifdef CONFIG_PPC
    if(adapter->shared.mac_type >= e1000_82543) {
        E1000_WRITE_REG(&adapter->shared, TXDCTL, 0x00020000);
    }
#endif

    /* Setup Transmit Descriptor Settings for this adapter */
    adapter->TxdCmd = E1000_TXD_CMD_IFCS;

    if(adapter->tx_int_delay > 0)
        adapter->TxdCmd |= E1000_TXD_CMD_IDE;
    if(adapter->shared.report_tx_early == 1)
        adapter->TxdCmd |= E1000_TXD_CMD_RS;
    else
        adapter->TxdCmd |= E1000_TXD_CMD_RPS;

    adapter->ActiveChecksumContext = OFFLOAD_NONE;

    return;
}

/**
 * e1000_setup_rx_resources - allocate Rx resources (Descriptors, receive SKBs)
 * @adapter: board private structure
 * 
 * Returns 0 on success, negative on failure
 *
 * e1000_setup_rx_resources allocates all software receive resources
 * and network buffers, and enables the Rx unit of the MAC.
 **/

static int
e1000_setup_rx_resources(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;
    int size;

    E1000_DBG("e1000_setup_rx_resources\n");

    size = sizeof(struct e1000_buffer) * adapter->rx_ring.count;
    adapter->rx_ring.buffer_info = kmalloc(size, GFP_KERNEL);
    if(adapter->rx_ring.buffer_info == NULL) {
        return -ENOMEM;
    }
    memset(adapter->rx_ring.buffer_info, 0, size);

    /* Round up to nearest 4K */

    adapter->rx_ring.size = E1000_ROUNDUP2(adapter->rx_ring.count *
                                           sizeof(struct e1000_rx_desc),
                                           4096);

    adapter->rx_ring.desc = pci_alloc_consistent(pdev, adapter->rx_ring.size, 
                                                 &adapter->rx_ring.dma);

    if(adapter->rx_ring.desc == NULL) {
        kfree(adapter->rx_ring.buffer_info);
        return -ENOMEM;
    }
    memset(adapter->rx_ring.desc, 0, adapter->rx_ring.size);

    adapter->rx_ring.next_to_clean = 0;
    atomic_set(&adapter->rx_ring.unused, adapter->rx_ring.count);

    adapter->rx_ring.next_to_use = 0;

    return 0;
}

/**
 * e1000_setup_rctl - configure the receive control register
 * @adapter: Board private structure
 **/

static void
e1000_setup_rctl(struct e1000_adapter *adapter)
{
    uint32_t rctl;

    /* Setup the Receive Control Register */
    rctl =
        E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_LBM_NO |
        E1000_RCTL_RDMTS_HALF | (adapter->shared.
                                 mc_filter_type << E1000_RCTL_MO_SHIFT);

    if(adapter->shared.tbi_compatibility_on == 1)
        rctl |= E1000_RCTL_SBP;

    switch (adapter->rx_buffer_len) {
    case E1000_RXBUFFER_2048:
    default:
        rctl |= E1000_RCTL_SZ_2048;
        break;
    case E1000_RXBUFFER_4096:
        rctl |= E1000_RCTL_SZ_4096 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
        break;
    case E1000_RXBUFFER_8192:
        rctl |= E1000_RCTL_SZ_8192 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
        break;
    case E1000_RXBUFFER_16384:
        rctl |= E1000_RCTL_SZ_16384 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
        break;
    }

    E1000_WRITE_REG(&adapter->shared, RCTL, rctl);
}

/**
 * e1000_configure_rx - Configure 8254x Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/

static void
e1000_configure_rx(struct e1000_adapter *adapter)
{
    uint32_t rctl;
    uint32_t rxcsum;

    /* make sure receives are disabled while setting up the descriptor ring */
    rctl = E1000_READ_REG(&adapter->shared, RCTL);
    E1000_WRITE_REG(&adapter->shared, RCTL, rctl & ~E1000_RCTL_EN);

    /* set the Receive Delay Timer Register */
    E1000_WRITE_REG(&adapter->shared, RDTR,
                    adapter->rx_int_delay | E1000_RDT_FPDB);

    /* Setup the Base and Length of the Rx Descriptor Ring */
    /* rx_ring.dma can be either a 32 or 64 bit value */

#if (BITS_PER_LONG == 32)
    E1000_WRITE_REG(&adapter->shared, RDBAL, adapter->rx_ring.dma);
    E1000_WRITE_REG(&adapter->shared, RDBAH, 0);
#elif ( BITS_PER_LONG == 64)
    E1000_WRITE_REG(&adapter->shared, RDBAL,
                    (uint32_t) (adapter->rx_ring.dma & 0x00000000FFFFFFFF));
    E1000_WRITE_REG(&adapter->shared, RDBAH,
                    (uint32_t) (adapter->rx_ring.dma >> 32));
#else
#error "Unsupported System - does not use 32 or 64 bit pointers!"
#endif

    E1000_WRITE_REG(&adapter->shared, RDLEN,
                    adapter->rx_ring.count * sizeof(struct e1000_rx_desc));

    /* Setup the HW Rx Head and Tail Descriptor Pointers */
    E1000_WRITE_REG(&adapter->shared, RDH, 0);
    E1000_WRITE_REG(&adapter->shared, RDT, 0);

    /* Enable 82543 Receive Checksum Offload for TCP and UDP */
    if((adapter->shared.mac_type >= e1000_82543) &&
       (adapter->RxChecksum == TRUE)) {
        rxcsum = E1000_READ_REG(&adapter->shared, RXCSUM);
        rxcsum |= E1000_RXCSUM_TUOFL;
        E1000_WRITE_REG(&adapter->shared, RXCSUM, rxcsum);
    }
#ifdef CONFIG_PPC
    if(adapter->shared.mac_type >= e1000_82543) {
        E1000_WRITE_REG(&adapter->shared, RXDCTL, 0x00020000);
    }
#endif

    /* Enable Receives */
    E1000_WRITE_REG(&adapter->shared, RCTL, rctl);

    return;
}

/**
 * e1000_free_tx_resources - Free Tx Resources
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/

static void
e1000_free_tx_resources(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;

    E1000_DBG("e1000_free_tx_resources\n");

    e1000_clean_tx_ring(adapter);

    kfree(adapter->tx_ring.buffer_info);
    adapter->tx_ring.buffer_info = NULL;

    pci_free_consistent(pdev, adapter->tx_ring.size, adapter->tx_ring.desc,
                        adapter->tx_ring.dma);

    adapter->tx_ring.desc = NULL;

    return;
}

/**
 * e1000_clean_tx_ring - Free Tx Buffers
 * @adapter: board private structure
 **/

static void
e1000_clean_tx_ring(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;
    unsigned long size;
    int i;

    /* Free all the Tx ring sk_buffs */

    for(i = 0; i < adapter->tx_ring.count; i++) {
        if(adapter->tx_ring.buffer_info[i].skb != NULL) {

            pci_unmap_page(pdev, adapter->tx_ring.buffer_info[i].dma,
                           adapter->tx_ring.buffer_info[i].length,
                           PCI_DMA_TODEVICE);

            dev_kfree_skb(adapter->tx_ring.buffer_info[i].skb);

            adapter->tx_ring.buffer_info[i].skb = NULL;
        }
    }

    size = sizeof(struct e1000_buffer) * adapter->tx_ring.count;
    memset(adapter->tx_ring.buffer_info, 0, size);

    /* Zero out the descriptor ring */

    memset(adapter->tx_ring.desc, 0, adapter->tx_ring.size);

    atomic_set(&adapter->tx_ring.unused, adapter->tx_ring.count);
    adapter->tx_ring.next_to_use = 0;
    adapter->tx_ring.next_to_clean = 0;

    return;
}

/**
 * e1000_free_rx_resources - Free Rx Resources
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/

static void
e1000_free_rx_resources(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;

    E1000_DBG("e1000_free_rx_resources\n");

    tasklet_disable(&adapter->rx_fill_tasklet);

    e1000_clean_rx_ring(adapter);

    kfree(adapter->rx_ring.buffer_info);
    adapter->rx_ring.buffer_info = NULL;

    pci_free_consistent(pdev, adapter->rx_ring.size, adapter->rx_ring.desc,
                        adapter->rx_ring.dma);

    adapter->rx_ring.desc = NULL;

    return;
}

/**
 * e1000_clean_rx_ring - Free Rx Buffers
 * @adapter: board private structure
 **/

static void
e1000_clean_rx_ring(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;
    unsigned long size;
    int i;

    /* Free all the Rx ring sk_buffs */

    for(i = 0; i < adapter->rx_ring.count; i++) {
        if(adapter->rx_ring.buffer_info[i].skb != NULL) {

            pci_unmap_single(pdev, adapter->rx_ring.buffer_info[i].dma,
                             adapter->rx_ring.buffer_info[i].length,
                             PCI_DMA_FROMDEVICE);

            dev_kfree_skb(adapter->rx_ring.buffer_info[i].skb);

            adapter->rx_ring.buffer_info[i].skb = NULL;
        }
    }

    size = sizeof(struct e1000_buffer) * adapter->rx_ring.count;
    memset(adapter->rx_ring.buffer_info, 0, size);

    /* Zero out the descriptor ring */

    memset(adapter->rx_ring.desc, 0, adapter->rx_ring.size);

    atomic_set(&adapter->rx_ring.unused, adapter->rx_ring.count);
    adapter->rx_ring.next_to_clean = 0;
    adapter->rx_ring.next_to_use = 0;

    return;
}

/**
 * e1000_set_multi - Multicast and Promiscuous mode set
 * @netdev: network interface device structure
 *
 * The set_multi entry point is called whenever the multicast address
 * list or the network interface flags are updated.  This routine is
 * resposible for configuring the hardware for proper multicast,
 * promiscuous mode, and all-multi behavior.
 **/

void
e1000_set_multi(struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;
    struct pci_dev *pdev = adapter->pdev;
    uint32_t rctl;
    uint8_t mta[MAX_NUM_MULTICAST_ADDRESSES * ETH_LENGTH_OF_ADDRESS];
    uint16_t pci_command_word;
    struct dev_mc_list *mc_ptr;
    int i;

    E1000_DBG("e1000_set_multi\n");

    rctl = E1000_READ_REG(&adapter->shared, RCTL);

    if(adapter->shared.mac_type == e1000_82542_rev2_0) {
        if(adapter->shared.pci_cmd_word & PCI_COMMAND_INVALIDATE) {
            pci_command_word =
                adapter->shared.pci_cmd_word & ~PCI_COMMAND_INVALIDATE;
            pci_write_config_word(pdev, PCI_COMMAND, pci_command_word);
        }
        rctl |= E1000_RCTL_RST;
        E1000_WRITE_REG(&adapter->shared, RCTL, rctl);
        mdelay(5);
        if(test_bit(E1000_BOARD_OPEN, &adapter->flags)) {
            tasklet_disable(&adapter->rx_fill_tasklet);
            e1000_clean_rx_ring(adapter);
        }
    }

    /* Check for Promiscuous and All Multicast modes */

    if(netdev->flags & IFF_PROMISC) {
        rctl |= (E1000_RCTL_UPE | E1000_RCTL_MPE);
    } else if(netdev->flags & IFF_ALLMULTI) {
        rctl |= E1000_RCTL_MPE;
        rctl &= ~E1000_RCTL_UPE;
    } else {
        rctl &= ~(E1000_RCTL_UPE | E1000_RCTL_MPE);
    }

    if(netdev->mc_count > MAX_NUM_MULTICAST_ADDRESSES) {
        rctl |= E1000_RCTL_MPE;
        E1000_WRITE_REG(&adapter->shared, RCTL, rctl);
    } else {
        E1000_WRITE_REG(&adapter->shared, RCTL, rctl);
        for(i = 0, mc_ptr = netdev->mc_list; mc_ptr; i++, mc_ptr = mc_ptr->next)
            memcpy(&mta[i * ETH_LENGTH_OF_ADDRESS], mc_ptr->dmi_addr,
                   ETH_LENGTH_OF_ADDRESS);
        e1000_mc_addr_list_update(&adapter->shared, mta, netdev->mc_count, 0);
    }

    if(adapter->shared.mac_type == e1000_82542_rev2_0) {
        rctl = E1000_READ_REG(&adapter->shared, RCTL);
        rctl &= ~E1000_RCTL_RST;
        E1000_WRITE_REG(&adapter->shared, RCTL, rctl);
        mdelay(5);
        if(adapter->shared.pci_cmd_word & PCI_COMMAND_INVALIDATE) {
            pci_write_config_word(pdev, PCI_COMMAND,
                                  adapter->shared.pci_cmd_word);
        }
        if(test_bit(E1000_BOARD_OPEN, &adapter->flags)) {
            e1000_configure_rx(adapter);
            tasklet_enable(&adapter->rx_fill_tasklet);
        }
    }

    return;
}

#ifdef IANS

/* flush Tx queue without link */
static void
e1000_tx_flush(struct e1000_adapter *adapter)
{
    uint32_t ctrl, txcw, icr;

    adapter->int_mask = 0;
    e1000_irq_disable(adapter);
    synchronize_irq();

    if(adapter->shared.mac_type < e1000_82543) {
        /* Transmit Unit Reset */
        E1000_WRITE_REG(&adapter->shared, TCTL, E1000_TCTL_RST);
        E1000_WRITE_REG(&adapter->shared, TCTL, 0);
        e1000_clean_tx_ring(adapter);
        e1000_configure_tx(adapter);
    } else {
        /* turn off autoneg, set link up, and invert loss of signal */
        txcw = E1000_READ_REG(&adapter->shared, TXCW);
        ctrl = E1000_READ_REG(&adapter->shared, CTRL);
        E1000_WRITE_REG(&adapter->shared, TXCW, txcw & ~E1000_TXCW_ANE);
        E1000_WRITE_REG(&adapter->shared, CTRL,
                        (ctrl | E1000_CTRL_SLU | E1000_CTRL_ILOS));
        /* delay to flush queue, then clean up */
        mdelay(20);
        e1000_clean_tx_irq(adapter);
        E1000_WRITE_REG(&adapter->shared, CTRL, ctrl);
        E1000_WRITE_REG(&adapter->shared, TXCW, txcw);
        /* clear the link status change interrupts this caused */
        icr = E1000_READ_REG(&adapter->shared, ICR);
    }

    adapter->int_mask = IMS_ENABLE_MASK;
    e1000_irq_enable(adapter);
    return;
}
#endif

/**
 * e1000_watchdog - Timer Call-back
 * @data: pointer to netdev cast into an unsigned long
 **/

void
e1000_watchdog(unsigned long data)
{
    struct net_device *netdev = (struct net_device *) data;
    struct e1000_adapter *adapter = netdev->priv;

#ifdef IANS
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
    int flags;
#endif
#endif

    e1000_check_for_link(&adapter->shared);

    if (test_and_clear_bit(E1000_LINK_STATUS_CHANGED, &adapter->flags))
        e1000_phy_get_info(&adapter->shared, &adapter->phy_info);

    if(E1000_READ_REG(&adapter->shared, STATUS) & E1000_STATUS_LU) {
        if(adapter->link_active != TRUE) {

#ifdef IANS
            if((adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) &&
               (adapter->iANSdata->reporting_mode == IANS_STATUS_REPORTING_ON))
                if(ans_notify)
                    ans_notify(netdev, IANS_IND_XMIT_QUEUE_READY);
#endif
            netif_wake_queue(netdev);

            e1000_get_speed_and_duplex(&adapter->shared, &adapter->link_speed,
                                       &adapter->link_duplex);
            printk(KERN_ERR "e1000: %s NIC Link is Up %d Mbps %s\n",
                   netdev->name, adapter->link_speed,
                   adapter->link_duplex ==
                   FULL_DUPLEX ? "Full Duplex" : "Half Duplex");

            adapter->link_active = TRUE;
            set_bit(E1000_LINK_STATUS_CHANGED, &adapter->flags);
        }
    } else {
        if(adapter->link_active != FALSE) {
            adapter->link_speed = 0;
            adapter->link_duplex = 0;
            printk(KERN_ERR "e1000: %s NIC Link is Down\n", netdev->name);
            adapter->link_active = FALSE;
            atomic_set(&adapter->tx_timeout, 0);
        }
    }

    e1000_update_stats(adapter);

    if(atomic_read(&adapter->tx_timeout) > 1)
        atomic_dec(&adapter->tx_timeout);

    if((adapter->link_active == TRUE) && 
       (atomic_read(&adapter->tx_timeout) == 1)) {

        if(E1000_READ_REG(&adapter->shared, STATUS) & E1000_STATUS_TXOFF) {
            atomic_set(&adapter->tx_timeout, 3);
        } else {

            e1000_hibernate_adapter(netdev);

#ifdef IANS
        if((adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) &&
           (adapter->iANSdata->reporting_mode == IANS_STATUS_REPORTING_ON)) {
                adapter->link_active = FALSE;
                bd_ans_os_Watchdog(netdev, adapter);
                adapter->link_active = TRUE;
            }
#endif
            atomic_set(&adapter->tx_timeout, 0);
            e1000_wakeup_adapter(netdev);
        }
    }
#ifdef IANS
    if(adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) {

        if(adapter->iANSdata->reporting_mode == IANS_STATUS_REPORTING_ON)
            bd_ans_os_Watchdog(netdev, adapter);

        if(adapter->link_active == FALSE) {
            /* don't sit on SKBs while link is down */

            if(atomic_read(&adapter->tx_ring.unused) < adapter->tx_ring.count) {

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
                spin_lock_irqsave(&netdev->xmit_lock, flags);
                e1000_tx_flush(adapter);
                spin_unlock_irqrestore(&netdev->xmit_lock, flags);
#else
                e1000_tx_flush(adapter);
#endif
            }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
            spin_lock_irqsave(&netdev->queue_lock, flags);
            qdisc_reset(netdev->qdisc);
            spin_unlock_irqrestore(&netdev->queue_lock, flags);
#else
            qdisc_reset(netdev->qdisc);
#endif
        }
    }
#endif

    if(test_bit(E1000_RX_REFILL, &adapter->flags)) {
        tasklet_schedule(&adapter->rx_fill_tasklet);
    }

    /* Reset the timer */
    mod_timer(&adapter->timer_id, jiffies + 2 * HZ);

    return;
}

/**
 * e1000_tx_checksum_setup
 * @adapter:
 * @skb:
 * @txd_upper:
 * @txd_lower:
 **/

static inline void
e1000_tx_checksum_setup(struct e1000_adapter *adapter,
                        struct sk_buff *skb,
                        uint32_t *txd_upper,
                        uint32_t *txd_lower)
{

    struct e1000_context_desc *desc;
    int i;

    if(skb->protocol != __constant_htons(ETH_P_IP)) {
        *txd_upper = 0;
        *txd_lower = adapter->TxdCmd;
        return;
    }

    switch (skb->nh.iph->protocol) {
    case IPPROTO_TCP:
        /* Offload TCP checksum */
        *txd_upper = E1000_TXD_POPTS_TXSM << 8;
        *txd_lower = adapter->TxdCmd | E1000_TXD_CMD_DEXT | E1000_TXD_DTYP_D;
        if(adapter->ActiveChecksumContext == OFFLOAD_TCP_IP)
            return;
        else
            adapter->ActiveChecksumContext = OFFLOAD_TCP_IP;
        break;
    case IPPROTO_UDP:
        /* Offload UDP checksum */
        *txd_upper = E1000_TXD_POPTS_TXSM << 8;
        *txd_lower = adapter->TxdCmd | E1000_TXD_CMD_DEXT | E1000_TXD_DTYP_D;
        if(adapter->ActiveChecksumContext == OFFLOAD_UDP_IP)
            return;
        else
            adapter->ActiveChecksumContext = OFFLOAD_UDP_IP;
        break;
    default:
        /* no checksum to offload */
        *txd_upper = 0;
        *txd_lower = adapter->TxdCmd;
        return;
    }

    /* If we reach this point, the checksum offload context
     * needs to be reset
     */

    i = adapter->tx_ring.next_to_use;
    desc = E1000_CONTEXT_DESC(adapter->tx_ring, i);

    desc->lower_setup.ip_fields.ipcss = skb->nh.raw - skb->data;
    desc->lower_setup.ip_fields.ipcso =
        ((skb->nh.raw + offsetof(struct iphdr, check)) - skb->data);
    desc->lower_setup.ip_fields.ipcse = cpu_to_le16(skb->h.raw - skb->data - 1);

    desc->upper_setup.tcp_fields.tucss = (skb->h.raw - skb->data);
    desc->upper_setup.tcp_fields.tucso = ((skb->h.raw + skb->csum) - skb->data);
    desc->upper_setup.tcp_fields.tucse = 0;

    desc->tcp_seg_setup.data = 0;
    desc->cmd_and_length = cpu_to_le32(E1000_TXD_CMD_DEXT) | adapter->TxdCmd;

    i = (i + 1) % adapter->tx_ring.count;
    atomic_dec(&adapter->tx_ring.unused);
    adapter->tx_ring.next_to_use = i;
    E1000_WRITE_REG(&adapter->shared, TDT, adapter->tx_ring.next_to_use);
    return;
}

/**
 * e1000_xmit_frame - Transmit entry point
 * @skb: buffer with frame data to transmit
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative on error
 *
 * e1000_xmit_frame is called by the stack to initiate a transmit.
 * The out of resource condition is checked after each successful Tx
 * so that the stack can be notified, preventing the driver from
 * ever needing to drop a frame.  The atomic operations on
 * tx_ring.unused are used to syncronize with the transmit
 * interrupt processing code without the need for a spinlock.
 **/

int
e1000_xmit_frame(struct sk_buff *skb,
                 struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;
    struct pci_dev *pdev = adapter->pdev;
    struct e1000_tx_desc *tx_desc;
    int i, len, offset, txd_needed;
    uint32_t txd_upper, txd_lower;

#define TXD_USE_COUNT(x) (((x) >> 12) + ((x) & 0x0fff ? 1 : 0))

#ifdef MAX_SKB_FRAGS
    int f;
    skb_frag_t *frag;
#endif

    E1000_DBG("e1000_xmit_frame\n");

    if(adapter->link_active == FALSE) {
#ifdef IANS
        if((adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) &&
           (adapter->iANSdata->reporting_mode == IANS_STATUS_REPORTING_ON))
            if(ans_notify)
                ans_notify(netdev, IANS_IND_XMIT_QUEUE_FULL);
#endif
        netif_stop_queue(netdev);
        return 1;
    }

#ifdef MAX_SKB_FRAGS
    txd_needed = TXD_USE_COUNT(skb->len - skb->data_len);
    for(f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
        frag = &skb_shinfo(skb)->frags[f];
        txd_needed += TXD_USE_COUNT(frag->size);
    }
#else
    txd_needed = TXD_USE_COUNT(skb->len);
#endif

    /* make sure there are enough Tx descriptors available in the ring */
    if(atomic_read(&adapter->tx_ring.unused) <= (txd_needed + 1)) {
        adapter->net_stats.tx_dropped++;
#ifdef IANS
        if((adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) &&
           (adapter->iANSdata->reporting_mode == IANS_STATUS_REPORTING_ON))
            if(ans_notify)
                ans_notify(netdev, IANS_IND_XMIT_QUEUE_FULL);
#endif
        netif_stop_queue(netdev);

        return 1;
    }

    if(skb->ip_summed == CHECKSUM_HW) {
        e1000_tx_checksum_setup(adapter, skb, &txd_upper, &txd_lower);
    } else {
        txd_upper = 0;
        txd_lower = adapter->TxdCmd;
    }

    i = adapter->tx_ring.next_to_use;
    tx_desc = E1000_TX_DESC(adapter->tx_ring, i);

#ifdef IANS
    if(adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) {
        tx_desc->lower.data = cpu_to_le32(txd_lower);
        tx_desc->upper.data = cpu_to_le32(txd_upper);
        if(bd_ans_os_Transmit(adapter, tx_desc, &skb) == BD_ANS_FAILURE) {
            return 1;
        }
        txd_lower = le32_to_cpu(tx_desc->lower.data);
        txd_upper = le32_to_cpu(tx_desc->upper.data);
    }
#endif

#ifdef MAX_SKB_FRAGS
    len = skb->len - skb->data_len;
#else
    len = skb->len;
#endif
    offset = 0;

    while(len > 4096) {
        adapter->tx_ring.buffer_info[i].length = 4096;
        adapter->tx_ring.buffer_info[i].dma =
            pci_map_page(pdev, virt_to_page(skb->data + offset),
                         (unsigned long) (skb->data + offset) & ~PAGE_MASK,
                         4096, PCI_DMA_TODEVICE);

        tx_desc->buffer_addr = cpu_to_le64(adapter->tx_ring.buffer_info[i].dma);
        tx_desc->lower.data = cpu_to_le32(txd_lower | 4096);
        tx_desc->upper.data = cpu_to_le32(txd_upper);

        len -= 4096;
        offset += 4096;
        i = (i + 1) % adapter->tx_ring.count;
        atomic_dec(&adapter->tx_ring.unused);
        tx_desc = E1000_TX_DESC(adapter->tx_ring, i);
    }
    adapter->tx_ring.buffer_info[i].length = len;
    adapter->tx_ring.buffer_info[i].dma =
        pci_map_page(pdev, virt_to_page(skb->data + offset),
                     (unsigned long) (skb->data + offset) & ~PAGE_MASK, len,
                     PCI_DMA_TODEVICE);

    tx_desc->buffer_addr = cpu_to_le64(adapter->tx_ring.buffer_info[i].dma);
    tx_desc->lower.data = cpu_to_le32(txd_lower | len);
    tx_desc->upper.data = cpu_to_le32(txd_upper);

#ifdef MAX_SKB_FRAGS
    for(f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
        frag = &skb_shinfo(skb)->frags[f];
        i = (i + 1) % adapter->tx_ring.count;
        atomic_dec(&adapter->tx_ring.unused);
        tx_desc = E1000_TX_DESC(adapter->tx_ring, i);

        len = frag->size;
        offset = 0;

        while(len > 4096) {
            adapter->tx_ring.buffer_info[i].length = 4096;
            adapter->tx_ring.buffer_info[i].dma =
                pci_map_page(pdev, frag->page, frag->page_offset + offset,
                             4096, PCI_DMA_TODEVICE);

            tx_desc->buffer_addr =
                cpu_to_le64(adapter->tx_ring.buffer_info[i].dma);
            tx_desc->lower.data = cpu_to_le32(txd_lower | 4096);
            tx_desc->upper.data = cpu_to_le32(txd_upper);

            len -= 4096;
            offset += 4096;
            i = (i + 1) % adapter->tx_ring.count;
            atomic_dec(&adapter->tx_ring.unused);
            tx_desc = E1000_TX_DESC(adapter->tx_ring, i);
        }
        adapter->tx_ring.buffer_info[i].length = len;
        adapter->tx_ring.buffer_info[i].dma =
            pci_map_page(pdev, frag->page, frag->page_offset + offset, len,
                         PCI_DMA_TODEVICE);
        tx_desc->buffer_addr =
            cpu_to_le64(adapter->tx_ring.buffer_info[i].dma);

        tx_desc->lower.data = cpu_to_le32(txd_lower | len);
        tx_desc->upper.data = cpu_to_le32(txd_upper);
    }
#endif
    
    /* EOP and SKB pointer go with the last fragment */
    tx_desc->lower.data |= cpu_to_le32(E1000_TXD_CMD_EOP);
    adapter->tx_ring.buffer_info[i].skb = skb;

    i = (i + 1) % adapter->tx_ring.count;
    atomic_dec(&adapter->tx_ring.unused);

    /* Move the HW Tx Tail Pointer */
    adapter->tx_ring.next_to_use = i;

    E1000_WRITE_REG(&adapter->shared, TDT, adapter->tx_ring.next_to_use);

    if(atomic_read(&adapter->tx_timeout) == 0)
        atomic_set(&adapter->tx_timeout, 3);

    netdev->trans_start = jiffies;

    return 0;
}

/**
 * e1000_get_stats - Get System Network Statistics
 * @netdev: network interface device structure
 *
 * Returns the address of the device statistics structure.
 * The statistics are actually updated from the timer callback.
 **/

struct net_device_stats *
e1000_get_stats(struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;

    E1000_DBG("e1000_get_stats\n");

    return &adapter->net_stats;
}

/**
 * e1000_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/

int
e1000_change_mtu(struct net_device *netdev,
                 int new_mtu)
{
    struct e1000_adapter *adapter = netdev->priv;
    uint32_t old_mtu = adapter->rx_buffer_len;

    E1000_DBG("e1000_change_mtu\n");
    if((new_mtu < MINIMUM_ETHERNET_PACKET_SIZE - ENET_HEADER_SIZE) ||
       (new_mtu > MAX_JUMBO_FRAME_SIZE - ENET_HEADER_SIZE)) {
        E1000_ERR("Invalid MTU setting\n");
        return -EINVAL;
    }

    if(new_mtu <= MAXIMUM_ETHERNET_PACKET_SIZE - ENET_HEADER_SIZE) {
        /* 2k buffers */
        adapter->rx_buffer_len = E1000_RXBUFFER_2048;

    } else if(adapter->shared.mac_type < e1000_82543) {
        E1000_ERR("Jumbo Frames not supported on 82542\n");
        return -EINVAL;

    } else if(new_mtu <= E1000_RXBUFFER_4096 - ENET_HEADER_SIZE - CRC_LENGTH) {
        /* 4k buffers */
        adapter->rx_buffer_len = E1000_RXBUFFER_4096;

    } else if(new_mtu <= E1000_RXBUFFER_8192 - ENET_HEADER_SIZE - CRC_LENGTH) {
        /* 8k buffers */
        adapter->rx_buffer_len = E1000_RXBUFFER_8192;

    } else {
        /* 16k buffers */
        adapter->rx_buffer_len = E1000_RXBUFFER_16384;
    }

    if(old_mtu != adapter->rx_buffer_len &&
       test_bit(E1000_BOARD_OPEN, &adapter->flags)) {

        /* stop */
        tasklet_disable(&adapter->rx_fill_tasklet);
        netif_stop_queue(netdev);
        adapter->shared.adapter_stopped = 0;
        e1000_adapter_stop(&adapter->shared);

        /* clean out old buffers */
        e1000_clean_rx_ring(adapter);
        e1000_clean_tx_ring(adapter);

        /* reset hardware */
        adapter->shared.adapter_stopped = 0;
        e1000_hw_init(adapter);

        /* go */
        e1000_setup_rctl(adapter);
        e1000_configure_rx(adapter);
        e1000_configure_tx(adapter);
#ifdef IANS
        /* restore VLAN settings */
        if((IANS_BD_TAGGING_MODE) (ANS_PRIVATE_DATA_FIELD(adapter)->tag_mode)
           != IANS_BD_TAGGING_NONE)
            bd_ans_hw_EnableVLAN(adapter);
#endif
        tasklet_enable(&adapter->rx_fill_tasklet);
        tasklet_schedule(&adapter->rx_fill_tasklet);
        e1000_irq_enable(adapter);
        netif_start_queue(netdev);
    }

    netdev->mtu = new_mtu;
    adapter->shared.max_frame_size = new_mtu + ENET_HEADER_SIZE + CRC_LENGTH;

    return 0;
}

/**
 * e1000_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 * 
 * Returns 0 on success, negative on failure
 **/

int
e1000_set_mac(struct net_device *netdev,
              void *p)
{
    struct e1000_adapter *adapter = netdev->priv;
    struct pci_dev *pdev = adapter->pdev;
    struct sockaddr *addr = (struct sockaddr *) p;
    uint32_t pci_command;
    uint32_t rctl;

    E1000_DBG("e1000_set_mac\n");

    rctl = E1000_READ_REG(&adapter->shared, RCTL);

    if(adapter->shared.mac_type == e1000_82542_rev2_0) {
        if(adapter->shared.pci_cmd_word & PCI_COMMAND_INVALIDATE) {
            pci_command =
                adapter->shared.pci_cmd_word & ~PCI_COMMAND_INVALIDATE;
            pci_write_config_word(pdev, PCI_COMMAND, pci_command);
        }
        E1000_WRITE_REG(&adapter->shared, RCTL, rctl | E1000_RCTL_RST);
        mdelay(5);
        if(test_bit(E1000_BOARD_OPEN, &adapter->flags)) {
            tasklet_disable(&adapter->rx_fill_tasklet);
            e1000_clean_rx_ring(adapter);
        }
    }

    memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);
    memcpy(adapter->shared.mac_addr, addr->sa_data, netdev->addr_len);

    e1000_rar_set(&adapter->shared, adapter->shared.mac_addr, 0);

    if(adapter->shared.mac_type == e1000_82542_rev2_0) {
        E1000_WRITE_REG(&adapter->shared, RCTL, rctl);
        mdelay(5);
        if(adapter->shared.pci_cmd_word & PCI_COMMAND_INVALIDATE) {
            pci_write_config_word(pdev, PCI_COMMAND,
                                  adapter->shared.pci_cmd_word);
        }
        if(test_bit(E1000_BOARD_OPEN, &adapter->flags)) {
            e1000_configure_rx(adapter);
            tasklet_enable(&adapter->rx_fill_tasklet);
        }
    }

    return 0;
}

/**
 * e1000_update_stats - Update the board statistics counters
 * @adapter: board private structure
 **/

static void
e1000_update_stats(struct e1000_adapter *adapter)
{
    unsigned long flags;

#define PHY_IDLE_ERROR_COUNT_MASK 0x00FF

    spin_lock_irqsave(&adapter->stats_lock, flags);

    adapter->stats.crcerrs += E1000_READ_REG(&adapter->shared, CRCERRS);
    adapter->stats.symerrs += E1000_READ_REG(&adapter->shared, SYMERRS);
    adapter->stats.mpc += E1000_READ_REG(&adapter->shared, MPC);
    adapter->stats.scc += E1000_READ_REG(&adapter->shared, SCC);
    adapter->stats.ecol += E1000_READ_REG(&adapter->shared, ECOL);
    adapter->stats.mcc += E1000_READ_REG(&adapter->shared, MCC);
    adapter->stats.latecol += E1000_READ_REG(&adapter->shared, LATECOL);
    adapter->stats.colc += E1000_READ_REG(&adapter->shared, COLC);
    adapter->stats.dc += E1000_READ_REG(&adapter->shared, DC);
    adapter->stats.sec += E1000_READ_REG(&adapter->shared, SEC);
    adapter->stats.rlec += E1000_READ_REG(&adapter->shared, RLEC);
    adapter->stats.xonrxc += E1000_READ_REG(&adapter->shared, XONRXC);
    adapter->stats.xontxc += E1000_READ_REG(&adapter->shared, XONTXC);
    adapter->stats.xoffrxc += E1000_READ_REG(&adapter->shared, XOFFRXC);
    adapter->stats.xofftxc += E1000_READ_REG(&adapter->shared, XOFFTXC);
    adapter->stats.fcruc += E1000_READ_REG(&adapter->shared, FCRUC);
    adapter->stats.prc64 += E1000_READ_REG(&adapter->shared, PRC64);
    adapter->stats.prc127 += E1000_READ_REG(&adapter->shared, PRC127);
    adapter->stats.prc255 += E1000_READ_REG(&adapter->shared, PRC255);
    adapter->stats.prc511 += E1000_READ_REG(&adapter->shared, PRC511);
    adapter->stats.prc1023 += E1000_READ_REG(&adapter->shared, PRC1023);
    adapter->stats.prc1522 += E1000_READ_REG(&adapter->shared, PRC1522);
    adapter->stats.gprc += E1000_READ_REG(&adapter->shared, GPRC);
    adapter->stats.bprc += E1000_READ_REG(&adapter->shared, BPRC);
    adapter->stats.mprc += E1000_READ_REG(&adapter->shared, MPRC);
    adapter->stats.gptc += E1000_READ_REG(&adapter->shared, GPTC);

    /* for the 64-bit byte counters the low dword must be read first */
    /* both registers clear on the read of the high dword */

    adapter->stats.gorcl += E1000_READ_REG(&adapter->shared, GORCL);
    adapter->stats.gorch += E1000_READ_REG(&adapter->shared, GORCH);
    adapter->stats.gotcl += E1000_READ_REG(&adapter->shared, GOTCL);
    adapter->stats.gotch += E1000_READ_REG(&adapter->shared, GOTCH);

    adapter->stats.rnbc += E1000_READ_REG(&adapter->shared, RNBC);
    adapter->stats.ruc += E1000_READ_REG(&adapter->shared, RUC);
    adapter->stats.rfc += E1000_READ_REG(&adapter->shared, RFC);
    adapter->stats.roc += E1000_READ_REG(&adapter->shared, ROC);
    adapter->stats.rjc += E1000_READ_REG(&adapter->shared, RJC);

    adapter->stats.torl += E1000_READ_REG(&adapter->shared, TORL);
    adapter->stats.torh += E1000_READ_REG(&adapter->shared, TORH);
    adapter->stats.totl += E1000_READ_REG(&adapter->shared, TOTL);
    adapter->stats.toth += E1000_READ_REG(&adapter->shared, TOTH);

    adapter->stats.tpr += E1000_READ_REG(&adapter->shared, TPR);
    adapter->stats.tpt += E1000_READ_REG(&adapter->shared, TPT);
    adapter->stats.ptc64 += E1000_READ_REG(&adapter->shared, PTC64);
    adapter->stats.ptc127 += E1000_READ_REG(&adapter->shared, PTC127);
    adapter->stats.ptc255 += E1000_READ_REG(&adapter->shared, PTC255);
    adapter->stats.ptc511 += E1000_READ_REG(&adapter->shared, PTC511);
    adapter->stats.ptc1023 += E1000_READ_REG(&adapter->shared, PTC1023);
    adapter->stats.ptc1522 += E1000_READ_REG(&adapter->shared, PTC1522);
    adapter->stats.mptc += E1000_READ_REG(&adapter->shared, MPTC);
    adapter->stats.bptc += E1000_READ_REG(&adapter->shared, BPTC);

    if(adapter->shared.mac_type >= e1000_82543) {
        adapter->stats.algnerrc += E1000_READ_REG(&adapter->shared, ALGNERRC);
        adapter->stats.rxerrc += E1000_READ_REG(&adapter->shared, RXERRC);
        adapter->stats.tncrs += E1000_READ_REG(&adapter->shared, TNCRS);
        adapter->stats.cexterr += E1000_READ_REG(&adapter->shared, CEXTERR);
        adapter->stats.tsctc += E1000_READ_REG(&adapter->shared, TSCTC);
        adapter->stats.tsctfc += E1000_READ_REG(&adapter->shared, TSCTFC);
    }

    /* Fill out the OS statistics structure */

    adapter->net_stats.rx_packets = adapter->stats.gprc;
    adapter->net_stats.tx_packets = adapter->stats.gptc;
    adapter->net_stats.rx_bytes = adapter->stats.gorcl;
    adapter->net_stats.tx_bytes = adapter->stats.gotcl;
    adapter->net_stats.multicast = adapter->stats.mprc;
    adapter->net_stats.collisions = adapter->stats.colc;

    /* Rx Errors */

    adapter->net_stats.rx_errors =
        adapter->stats.rxerrc + adapter->stats.crcerrs +
        adapter->stats.algnerrc + adapter->stats.rlec + adapter->stats.rnbc +
        adapter->stats.mpc + adapter->stats.cexterr;
    adapter->net_stats.rx_dropped = adapter->stats.rnbc;
    adapter->net_stats.rx_length_errors = adapter->stats.rlec;
    adapter->net_stats.rx_crc_errors = adapter->stats.crcerrs;
    adapter->net_stats.rx_frame_errors = adapter->stats.algnerrc;
    adapter->net_stats.rx_fifo_errors = adapter->stats.mpc;
    adapter->net_stats.rx_missed_errors = adapter->stats.mpc;

    /* Tx Errors */

    adapter->net_stats.tx_errors = adapter->stats.ecol + adapter->stats.latecol;
    adapter->net_stats.tx_aborted_errors = adapter->stats.ecol;
    adapter->net_stats.tx_window_errors = adapter->stats.latecol;

    /* Tx Dropped needs to be maintained elsewhere */

    if(adapter->shared.media_type == e1000_media_type_copper) {
        adapter->phy_stats.idle_errors +=
            (e1000_read_phy_reg(&adapter->shared, PHY_1000T_STATUS)
             & PHY_IDLE_ERROR_COUNT_MASK);
        adapter->phy_stats.receive_errors +=
            e1000_read_phy_reg(&adapter->shared, M88E1000_RX_ERR_CNTR);
    }

    spin_unlock_irqrestore(&adapter->stats_lock, flags);
    return;
}

/**
 * e1000_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/

static inline void
e1000_irq_disable(struct e1000_adapter *adapter)
{
    E1000_DBG("e1000_irq_disable\n");

    /* Mask off all interrupts */

    E1000_WRITE_REG(&adapter->shared, IMC, ~0);
    return;
}

/**
 * e1000_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/

static inline void
e1000_irq_enable(struct e1000_adapter *adapter)
{
    E1000_DBG("e1000_irq_enable\n");

    E1000_WRITE_REG(&adapter->shared, IMS, adapter->int_mask);
    return;
}

/**
 * e1000_intr - Interrupt Handler
 * @irq: interrupt number
 * @data: pointer to a network interface device structure
 * @pt_regs: CPU registers structure
 **/

void
e1000_intr(int irq,
           void *data,
           struct pt_regs *regs)
{
    struct net_device *netdev = (struct net_device *) data;
    struct e1000_adapter *adapter = netdev->priv;
    uint32_t icr;
    uint loop_count = E1000_MAX_INTR;

    E1000_DBG("e1000_intr\n");

    e1000_irq_disable(adapter);

    while(loop_count > 0 && (icr = E1000_READ_REG(&adapter->shared, ICR)) != 0) {

        if(icr & (E1000_ICR_RXSEQ | E1000_ICR_LSC)) {
            adapter->shared.get_link_status = 1;
            set_bit(E1000_LINK_STATUS_CHANGED, &adapter->flags);
            /* run the watchdog ASAP */
            mod_timer(&adapter->timer_id, jiffies);
        }

        e1000_clean_rx_irq(adapter);
        e1000_clean_tx_irq(adapter);
        loop_count--;
    }

    e1000_irq_enable(adapter);

    return;
}

/**
 * e1000_clean_tx_irq - Reclaim resources after transmit completes
 * @adapter: board private structure
 **/

static void
e1000_clean_tx_irq(struct e1000_adapter *adapter)
{
    struct pci_dev *pdev = adapter->pdev;
    int i;

    struct e1000_tx_desc *tx_desc;
    struct net_device *netdev = adapter->netdev;

    E1000_DBG("e1000_clean_tx_irq\n");

    i = adapter->tx_ring.next_to_clean;
    tx_desc = E1000_TX_DESC(adapter->tx_ring, i);

    while(tx_desc->upper.data & cpu_to_le32(E1000_TXD_STAT_DD)) {

        if(adapter->tx_ring.buffer_info[i].dma != 0) {
            pci_unmap_page(pdev, adapter->tx_ring.buffer_info[i].dma,
                           adapter->tx_ring.buffer_info[i].length,
                           PCI_DMA_TODEVICE);
            adapter->tx_ring.buffer_info[i].dma = 0;
        }

        if(adapter->tx_ring.buffer_info[i].skb != NULL) {
            dev_kfree_skb_irq(adapter->tx_ring.buffer_info[i].skb);
            adapter->tx_ring.buffer_info[i].skb = NULL;
        }

        atomic_inc(&adapter->tx_ring.unused);
        i = (i + 1) % adapter->tx_ring.count;

        tx_desc->upper.data = 0;
        tx_desc = E1000_TX_DESC(adapter->tx_ring, i);
    }

    adapter->tx_ring.next_to_clean = i;

    if(adapter->tx_ring.next_to_clean == adapter->tx_ring.next_to_use)
        atomic_set(&adapter->tx_timeout, 0);
    else
        atomic_set(&adapter->tx_timeout, 3);

    if(netif_queue_stopped(netdev) &&
       (atomic_read(&adapter->tx_ring.unused) >
        (adapter->tx_ring.count * 3 / 4))) {

#ifdef IANS
        if((adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) &&
           (adapter->iANSdata->reporting_mode == IANS_STATUS_REPORTING_ON))
            if(ans_notify)
                ans_notify(netdev, IANS_IND_XMIT_QUEUE_READY);
#endif
        netif_wake_queue(netdev);
    }

    return;
}

/**
 * e1000_clean_rx_irq - Send received data up the network stack,
 * @adapter: board private structure
 **/

static void
e1000_clean_rx_irq(struct e1000_adapter *adapter)
{
    struct net_device *netdev = adapter->netdev;
    struct pci_dev *pdev = adapter->pdev;
    struct e1000_rx_desc *rx_desc;
    int i;
    uint32_t length;
    struct sk_buff *skb;
    uint8_t last_byte;
    unsigned long flags;

    E1000_DBG("e1000_clean_rx_irq\n");

    i = adapter->rx_ring.next_to_clean;
    rx_desc = E1000_RX_DESC(adapter->rx_ring, i);

    while(rx_desc->status & E1000_RXD_STAT_DD) {
        pci_unmap_single(pdev, adapter->rx_ring.buffer_info[i].dma,
                         adapter->rx_ring.buffer_info[i].length,
                         PCI_DMA_FROMDEVICE);

        skb = adapter->rx_ring.buffer_info[i].skb;
        length = le16_to_cpu(rx_desc->length);

        if(!(rx_desc->status & E1000_RXD_STAT_EOP)) {

            /* All receives must fit into a single buffer */

            E1000_DBG("Receive packet consumed multiple buffers\n");

            dev_kfree_skb_irq(skb);
            memset(rx_desc, 0, 16);
            mb();
            adapter->rx_ring.buffer_info[i].skb = NULL;

            atomic_inc(&adapter->rx_ring.unused);

            i = (i + 1) % adapter->rx_ring.count;

            rx_desc = E1000_RX_DESC(adapter->rx_ring, i);
            continue;
        }

        if(rx_desc->errors & E1000_RXD_ERR_FRAME_ERR_MASK) {

            last_byte = *(skb->data + length - 1);

            if(TBI_ACCEPT
               (&adapter->shared, rx_desc->status, rx_desc->errors, length,
                last_byte)) {
                spin_lock_irqsave(&adapter->stats_lock, flags);
                e1000_tbi_adjust_stats(&adapter->shared, &adapter->stats,
                                       length, skb->data);
                spin_unlock_irqrestore(&adapter->stats_lock, flags);
                length--;
            } else {

                E1000_DBG("Receive Errors Reported by Hardware\n");

                dev_kfree_skb_irq(skb);
                memset(rx_desc, 0, 16);
                mb();
                adapter->rx_ring.buffer_info[i].skb = NULL;

                atomic_inc(&adapter->rx_ring.unused);
                i = (i + 1) % adapter->rx_ring.count;

                rx_desc = E1000_RX_DESC(adapter->rx_ring, i);
                continue;
            }
        }

        /* Good Receive */
        skb_put(skb, length - CRC_LENGTH);

        /* Adjust socket buffer accounting to only cover the ethernet frame
         * Not what the stack intends, but there exist TCP problems that
         * break NFS for network interfaces that need 2k receive buffers
         */
        skb->truesize = skb->len;

        /* Receive Checksum Offload */
        e1000_rx_checksum(adapter, rx_desc, skb);

#ifdef IANS
        if(adapter->iANSdata->iANS_status == IANS_COMMUNICATION_UP) {
            if(bd_ans_os_Receive(adapter, rx_desc, skb) == BD_ANS_FAILURE)
                dev_kfree_skb_irq(skb);
            else
                netif_rx(skb);
        } else {
            skb->protocol = eth_type_trans(skb, netdev);
            netif_rx(skb);
        }
#else
        skb->protocol = eth_type_trans(skb, netdev);
        netif_rx(skb);
#endif
        memset(rx_desc, 0, 16);
        mb();
        adapter->rx_ring.buffer_info[i].skb = NULL;

        atomic_inc(&adapter->rx_ring.unused);

        i = (i + 1) % adapter->rx_ring.count;

        rx_desc = E1000_RX_DESC(adapter->rx_ring, i);
    }

    /* if the Rx ring is less than 3/4 full, allocate more sk_buffs */

    if(atomic_read(&adapter->rx_ring.unused) > (adapter->rx_ring.count / 4)) {
        tasklet_schedule(&adapter->rx_fill_tasklet);
    }
    adapter->rx_ring.next_to_clean = i;

    return;
}

/**
 * e1000_alloc_rx_buffers - Replace used receive buffers
 * @data: address of board private structure
 **/

static void
e1000_alloc_rx_buffers(unsigned long data)
{
    struct e1000_adapter *adapter = (struct e1000_adapter *) data;
    struct net_device *netdev = adapter->netdev;
    struct pci_dev *pdev = adapter->pdev;
    struct e1000_rx_desc *rx_desc;
    struct sk_buff *skb;
    int i;
    int reserve_len;

    E1000_DBG("e1000_alloc_rx_buffers\n");

    /* kernel 2.4.7 seems to be broken with respect to tasklet locking */
    if(!spin_trylock(&adapter->rx_fill_lock))
        return;

    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) {
        spin_unlock(&adapter->rx_fill_lock);
        return;
    }

#ifdef IANS
    reserve_len = E1000_ROUNDUP2(BD_ANS_INFO_SIZE, 16) + 2;
#else
    reserve_len = 2;
#endif

    i = adapter->rx_ring.next_to_use;

    while(adapter->rx_ring.buffer_info[i].skb == NULL) {
        rx_desc = E1000_RX_DESC(adapter->rx_ring, i);

        skb = alloc_skb(adapter->rx_buffer_len + reserve_len, GFP_ATOMIC);

        if(skb == NULL) {
            /* Alloc Failed; If we could not allocate a
             *  skb during this schedule. Wait for a while before
             *  tasklet to allocate skb is called again.
             */
            set_bit(E1000_RX_REFILL, &adapter->flags);
            break;
        }

        /* Make buffer alignment 2 beyond a 16 byte boundary
         * this will result in a 16 byte aligned IP header after
         * the 14 byte MAC header is removed
         */
        skb_reserve(skb, reserve_len);

        skb->dev = netdev;

        adapter->rx_ring.buffer_info[i].skb = skb;
        adapter->rx_ring.buffer_info[i].length = adapter->rx_buffer_len;
        adapter->rx_ring.buffer_info[i].dma =
            pci_map_single(pdev, skb->data, adapter->rx_buffer_len,
                           PCI_DMA_FROMDEVICE);

        rx_desc->buffer_addr = cpu_to_le64(adapter->rx_ring.buffer_info[i].dma);

        /* move tail */
        E1000_WRITE_REG(&adapter->shared, RDT, i);

        atomic_dec(&adapter->rx_ring.unused);

        i = (i + 1) % adapter->rx_ring.count;

        if(test_and_clear_bit(E1000_RX_REFILL, &adapter->flags)) {
            /* Trigger Soft Interrupt */
            E1000_WRITE_REG(&adapter->shared, ICS, E1000_ICS_RXT0);
        }
    }

    adapter->rx_ring.next_to_use = i;

    spin_unlock(&adapter->rx_fill_lock);
    return;
}

/**
 * e1000_ioctl - 
 * @netdev:
 * @ifreq:
 * @cmd:
 **/

int
e1000_ioctl(struct net_device *netdev,
            struct ifreq *ifr,
            int cmd)
{
#ifdef IANS
    IANS_BD_PARAM_HEADER *header;
#endif

    E1000_DBG("e1000_do_ioctl\n");

    switch (cmd) {

#ifdef IANS
    case IANS_BASE_SIOC:
        header = (IANS_BD_PARAM_HEADER *) ifr->ifr_data;
        if((header->Opcode != IANS_OP_EXT_GET_STATUS) &&
           (!capable(CAP_NET_ADMIN)))
            return -EPERM;
        return bd_ans_os_Ioctl(netdev, ifr, cmd);
        break;
#endif

#ifdef IDIAG
    case IDIAG_PRO_BASE_SIOC:
        if(!capable(CAP_NET_ADMIN))
            return -EPERM;

#ifdef DIAG_DEBUG
        printk("Entering diagnostics\n");
#endif
        e1000_diag_ioctl(netdev, ifr);
        break;
#endif /* IDIAG */

#ifdef SIOCETHTOOL
    case SIOCETHTOOL:

        return e1000_ethtool_ioctl(netdev, ifr);

        break;
#endif

    default:
        return -EOPNOTSUPP;
    }

    return 0;
}

/**
 * e1000_rx_checksum - Receive Checksum Offload for 82543
 * @adapter: board private structure
 * @rx_desc: receive descriptor
 * @sk_buff: socket buffer with received data
 **/

static inline void
e1000_rx_checksum(struct e1000_adapter *adapter,
                  struct e1000_rx_desc *rx_desc,
                  struct sk_buff *skb)
{
    /* 82543 or newer only */
    if((adapter->shared.mac_type < e1000_82543) ||
       /* Ignore Checksum bit is set */
       (rx_desc->status & E1000_RXD_STAT_IXSM) ||
       /* TCP Checksum has not been calculated */
       (!(rx_desc->status & E1000_RXD_STAT_TCPCS))) {

        skb->ip_summed = CHECKSUM_NONE;
        return;
    }

    /* At this point we know the hardware did the TCP checksum */
    /* now look at the TCP checksum error bit */
    if(rx_desc->errors & E1000_RXD_ERR_TCPE) {
        /* let the stack verify checksum errors */
        skb->ip_summed = CHECKSUM_NONE;
        adapter->XsumRXError++;
    } else {
        /* TCP checksum is good */
        skb->ip_summed = CHECKSUM_UNNECESSARY;
        adapter->XsumRXGood++;
    }

    return;
}

void
e1000_hibernate_adapter(struct net_device *netdev)
{
    uint32_t icr;
    struct e1000_adapter *adapter = netdev->priv;

    e1000_irq_disable(adapter);
    netif_stop_queue(netdev);
    adapter->shared.adapter_stopped = 0;
    e1000_adapter_stop(&adapter->shared);

    if(test_bit(E1000_BOARD_OPEN, &adapter->flags)) {

        /* Disable tasklet only when interface is opened. */
        tasklet_disable(&adapter->rx_fill_tasklet);

        /* clean out old buffers */
        e1000_clean_rx_ring(adapter);
        e1000_clean_tx_ring(adapter);

        /* Delete watchdog timer */
        del_timer(&adapter->timer_id);

        /* Unhook irq */
        e1000_irq_disable(adapter);
        icr = E1000_READ_REG(&adapter->shared, ICR);
        free_irq(netdev->irq, netdev);
    }
}

void
e1000_wakeup_adapter(struct net_device *netdev)
{
    uint32_t icr;
    struct e1000_adapter *adapter = netdev->priv;

    adapter->shared.adapter_stopped = 0;
    e1000_adapter_stop(&adapter->shared);
    adapter->shared.adapter_stopped = 0;
    adapter->shared.fc = adapter->shared.original_fc;

    if(!e1000_init_hw(&adapter->shared))
        printk("Hardware Init Failed at wakeup\n");

    if(test_bit(E1000_BOARD_OPEN, &adapter->flags)) {

        /* Setup Rctl */
        e1000_setup_rctl(adapter);
        e1000_configure_rx(adapter);
        e1000_alloc_rx_buffers((unsigned long) adapter);
        e1000_set_multi(netdev);
        e1000_configure_tx(adapter);

#ifdef IANS
        if((IANS_BD_TAGGING_MODE) (ANS_PRIVATE_DATA_FIELD(adapter)->tag_mode)
           != IANS_BD_TAGGING_NONE)
            bd_ans_hw_EnableVLAN(adapter);
#endif

        /* Set the watchdog timer for 2 seconds */
        init_timer(&adapter->timer_id);
        adapter->timer_id.function = &e1000_watchdog;
        adapter->timer_id.data = (unsigned long) netdev;
        mod_timer(&adapter->timer_id, (jiffies + 2 * HZ));

        tasklet_enable(&adapter->rx_fill_tasklet);

        /* Hook irq */
        e1000_irq_disable(adapter);
        icr = E1000_READ_REG(&adapter->shared, ICR);
        if(request_irq
           (netdev->irq, &e1000_intr, SA_SHIRQ, e1000_driver_name, netdev) != 0)
            printk(KERN_ERR "e1000: Unable to hook irq.\n");

        e1000_irq_enable(adapter);
        netif_start_queue(netdev);
    }
}

#ifdef IDIAG
int
e1000_xmit_lbtest_frame(struct sk_buff *skb,
                        struct e1000_adapter *adapter)
{
    /*struct e1000_adapter *adapter = netdev->priv; */
    struct pci_dev *pdev = adapter->pdev;
    struct e1000_tx_desc *tx_desc;
    int i;

    i = adapter->tx_ring.next_to_use;
    tx_desc = E1000_TX_DESC(adapter->tx_ring, i);

    adapter->tx_ring.buffer_info[i].skb = skb;
    adapter->tx_ring.buffer_info[i].length = skb->len;
    adapter->tx_ring.buffer_info[i].dma =
        pci_map_page(pdev, virt_to_page(skb->data),
                     (unsigned long) skb->data & ~PAGE_MASK, skb->len,
                     PCI_DMA_TODEVICE);

    tx_desc->buffer_addr = cpu_to_le64(adapter->tx_ring.buffer_info[i].dma);
    tx_desc->lower.data = cpu_to_le32(skb->len);

    /* zero out the status field in the descriptor */

    tx_desc->upper.data = 0;

    tx_desc->lower.data |= E1000_TXD_CMD_EOP;
    tx_desc->lower.data |= E1000_TXD_CMD_IFCS;
    tx_desc->lower.data |= E1000_TXD_CMD_IDE;

    if(adapter->shared.report_tx_early == 1)
        tx_desc->lower.data |= E1000_TXD_CMD_RS;
    else
        tx_desc->lower.data |= E1000_TXD_CMD_RPS;

    /* Move the HW Tx Tail Pointer */

    adapter->tx_ring.next_to_use++;
    adapter->tx_ring.next_to_use %= adapter->tx_ring.count;

    E1000_WRITE_REG(&adapter->shared, TDT, adapter->tx_ring.next_to_use);
    mdelay(10);

    atomic_dec(&adapter->tx_ring.unused);

    if(atomic_read(&adapter->tx_ring.unused) <= 1) {

        /* this driver never actually drops transmits,
         * so use tx_dropped count to indicate the number of times
         * netif_stop_queue is called due to no available descriptors
         */

        adapter->net_stats.tx_dropped++;
        return (0);
    }
    return (1);
}

int
e1000_rcv_lbtest_frame(struct e1000_adapter *adapter,
                       unsigned int frame_size)
{
    struct pci_dev *pdev = adapter->pdev;
    struct e1000_rx_desc *rx_desc;
    int i, j = 0, rcved_pkt = 0;
    uint32_t Length;
    struct sk_buff *skb;

    mdelay(500);
    i = adapter->rx_ring.next_to_clean;
    rx_desc = E1000_RX_DESC(adapter->rx_ring, i);

    while(rx_desc->status & E1000_RXD_STAT_DD) {
        Length = le16_to_cpu(rx_desc->length) - CRC_LENGTH;
        skb = adapter->rx_ring.buffer_info[i].skb;

        /* Snoop the packet for pattern */
        rcved_pkt = e1000_check_lbtest_frame(skb, frame_size);

        pci_unmap_single(pdev, adapter->rx_ring.buffer_info[i].dma,
                         adapter->rx_ring.buffer_info[i].length,
                         PCI_DMA_FROMDEVICE);

        dev_kfree_skb_irq(skb);
        adapter->rx_ring.buffer_info[i].skb = NULL;

        rx_desc->status = 0;
        atomic_inc(&adapter->rx_ring.unused);

        i++;
        i %= adapter->rx_ring.count;
        rx_desc = E1000_RX_DESC(adapter->rx_ring, i);

        if(rcved_pkt)
            break;

        /* waited enough */
        if(j++ >= adapter->rx_ring.count)
            return 0;

        mdelay(5);

    }

    adapter->rx_ring.next_to_clean = i;

    return (rcved_pkt);

}

void
e1000_selective_wakeup_adapter(struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;
    uint32_t ctrl, txcw;

    e1000_init_hw(&adapter->shared);

    if((adapter->link_active == FALSE) &&
       (adapter->shared.mac_type == e1000_82543)) {

        txcw = E1000_READ_REG(&adapter->shared, TXCW);
        ctrl = E1000_READ_REG(&adapter->shared, CTRL);
        E1000_WRITE_REG(&adapter->shared, TXCW, txcw & ~E1000_TXCW_ANE);
        E1000_WRITE_REG(&adapter->shared, CTRL,
                        (ctrl | E1000_CTRL_SLU | E1000_CTRL_ILOS |
                         E1000_CTRL_FD));
        mdelay(20);
    }

    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) {
        set_bit(E1000_BOARD_OPEN, &adapter->flags);
        set_bit(E1000_DIAG_OPEN, &adapter->flags);
        e1000_setup_tx_resources(adapter);
        e1000_setup_rx_resources(adapter);
    }
    e1000_setup_rctl(adapter);
    e1000_configure_rx(adapter);
    e1000_alloc_rx_buffers((unsigned long) adapter);
    e1000_configure_tx(adapter);
}

void
e1000_selective_hibernate_adapter(struct net_device *netdev)
{
    struct e1000_adapter *adapter = netdev->priv;
    uint32_t ctrl, txcw;

    if((adapter->link_active == FALSE) &&
       (adapter->shared.mac_type == e1000_82543)) {

        txcw = E1000_READ_REG(&adapter->shared, TXCW);
        ctrl = E1000_READ_REG(&adapter->shared, CTRL);
        ctrl &= ~E1000_CTRL_SLU & ~E1000_CTRL_ILOS;
        E1000_WRITE_REG(&adapter->shared, TXCW, txcw | E1000_TXCW_ANE);
        E1000_WRITE_REG(&adapter->shared, CTRL, ctrl);
        mdelay(20);
    }
    /* clean out old buffers */
    e1000_clean_rx_ring(adapter);
    e1000_clean_tx_ring(adapter);
    if(test_and_clear_bit(E1000_DIAG_OPEN, &adapter->flags)) {
        e1000_free_tx_resources(adapter);
        e1000_free_rx_resources(adapter);
        clear_bit(E1000_BOARD_OPEN, &adapter->flags);
    }
}

static int
e1000_check_lbtest_frame(struct sk_buff *skb,
                         unsigned int frame_size)
{
    frame_size = (frame_size % 2) ? (frame_size - 1) : frame_size;
    if(*(skb->data + 3) == 0xFF) {
        if((*(skb->data + frame_size / 2 + 10) == 0xBE) &&
           (*(skb->data + frame_size / 2 + 12) == 0xAF)) {
            return 1;
        }
    }
    return 0;
}
#endif /* IDIAG */

#ifdef SIOCETHTOOL
/**
 * e1000_ethtool_ioctl - Ethtool Ioctl Support 
 * @netdev: net device structure 
 * @ifr: interface request structure 
 **/

static int
e1000_ethtool_ioctl(struct net_device *netdev,
                    struct ifreq *ifr)
{
    struct ethtool_cmd eth_cmd;
    struct e1000_adapter *adapter = netdev->priv;
    boolean_t re_initiate = FALSE;

#ifdef ETHTOOL_GLINK
    struct ethtool_value eth_e1000_linkinfo;
#endif
#ifdef ETHTOOL_GDRVINFO
    struct ethtool_drvinfo eth_e1000_info;
#endif
#ifdef ETHTOOL_GWOL
    struct ethtool_wolinfo eth_e1000_wolinfo;
#endif

    /* Get the data structure */
    if(copy_from_user(&eth_cmd, ifr->ifr_data, sizeof(eth_cmd)))
        return -EFAULT;

    switch (eth_cmd.cmd) {
        /* Get the information */
    case ETHTOOL_GSET:
        if(adapter->shared.media_type == e1000_media_type_copper) {
            eth_cmd.supported = E1000_ETHTOOL_COPPER_INTERFACE_SUPPORTS;
            eth_cmd.advertising = E1000_ETHTOOL_COPPER_INTERFACE_ADVERTISE;
            eth_cmd.port = PORT_MII;
            eth_cmd.phy_address = adapter->shared.phy_addr;
            eth_cmd.transceiver =
                (adapter->shared.mac_type >
                 e1000_82543) ? XCVR_INTERNAL : XCVR_EXTERNAL;
        } else {
            eth_cmd.supported = E1000_ETHTOOL_FIBER_INTERFACE_SUPPORTS;
            eth_cmd.advertising = E1000_ETHTOOL_FIBER_INTERFACE_ADVERTISE;
            eth_cmd.port = PORT_FIBRE;
        }

        if(adapter->link_active == TRUE) {
            e1000_get_speed_and_duplex(&adapter->shared, &adapter->link_speed,
                                       &adapter->link_duplex);
            eth_cmd.speed = adapter->link_speed;
            eth_cmd.duplex =
                (adapter->link_duplex ==
                 FULL_DUPLEX) ? DUPLEX_FULL : DUPLEX_HALF;
        } else {
            eth_cmd.speed = 0;
            eth_cmd.duplex = 0;
        }

        if(adapter->shared.autoneg)
            eth_cmd.autoneg = AUTONEG_ENABLE;
        else
            eth_cmd.autoneg = AUTONEG_DISABLE;

        if(copy_to_user(ifr->ifr_data, &eth_cmd, sizeof(eth_cmd)))
            return -EFAULT;

        break;

        /* set information */
    case ETHTOOL_SSET:
        /* need proper permission to do set */
        if(!capable(CAP_NET_ADMIN))
            return -EPERM;

        /* Cannot Force speed/duplex and at the same time autoneg.
         * Autoneg will override forcing. 
         * For example to force speed/duplex pass in 
         *  'speed 100 duplex half autoneg off'
         * pass in 'autoneg on' to start autoneg.
         */
        printk("e1000: Requested link to be forced to %d Speed, %s Duplex "
               "%s\n", eth_cmd.speed, (eth_cmd.duplex ? "Full" : "Half"),
               (eth_cmd.autoneg ? "and Autonegotiate" : "."));

        if(eth_cmd.autoneg && eth_cmd.speed)
            printk("e1000: Autoneg request will over-ride speed forcing\n");

        /* if not in autoneg mode and have been asked to enable autoneg */
        if(eth_cmd.autoneg) {
            if(adapter->shared.autoneg &&
               adapter->shared.autoneg_advertised == AUTONEG_ADV_DEFAULT)
                /* If already in Autoneg */
                return 0;
            else {
                adapter->shared.autoneg = 1;
                adapter->shared.autoneg_advertised = AUTONEG_ADV_DEFAULT;
                re_initiate = TRUE;
            }
        }
        /* Force link to whatever speed and duplex */
        /* Also turning off Autoneg in case of non-gig speeds */
        else if(eth_cmd.speed) {
            /* Check for invalid request */
            if(((eth_cmd.speed != SPEED_10) && (eth_cmd.speed != SPEED_100) &&
                (eth_cmd.speed != SPEED_1000)) ||
               ((eth_cmd.duplex != DUPLEX_HALF) &&
                (eth_cmd.duplex != DUPLEX_FULL)) ||
               (adapter->shared.media_type == e1000_media_type_fiber))
                return -EINVAL;

            e1000_get_speed_and_duplex(&adapter->shared, &adapter->link_speed,
                                       &adapter->link_duplex);
            /* If we are already forced to requested speed and duplex
             * Donot do anything, just return
             */
            if(!adapter->shared.autoneg &&
               (adapter->link_speed == eth_cmd.speed) &&
               (adapter->link_duplex == (eth_cmd.duplex + 1)))

                return 0;

            adapter->shared.autoneg = 0;
            adapter->shared.autoneg_advertised = 0;
            re_initiate = TRUE;
            switch (eth_cmd.speed + eth_cmd.duplex) {
            case (SPEED_10 + DUPLEX_HALF):
                adapter->shared.forced_speed_duplex = e1000_10_half;
                break;
            case (SPEED_100 + DUPLEX_HALF):
                adapter->shared.forced_speed_duplex = e1000_100_half;
                break;
            case (SPEED_10 + DUPLEX_FULL):
                adapter->shared.forced_speed_duplex = e1000_10_full;
                break;
            case (SPEED_100 + DUPLEX_FULL):
                adapter->shared.forced_speed_duplex = e1000_100_full;
                break;
            case (SPEED_1000 + DUPLEX_HALF):
                printk("Half Duplex is not supported at 1000 Mbps\n");
            case (SPEED_1000 + DUPLEX_FULL):
                printk("Using Auto-neg at 1000 Mbps Full Duplex\n");
            default:
                adapter->shared.autoneg = 1;
                adapter->shared.autoneg_advertised = ADVERTISE_1000_FULL;
                break;
            }
        }

        /* End of force */
        /* Put the adapter to new settings */
        if(re_initiate == TRUE) {
            e1000_hibernate_adapter(netdev);
            e1000_wakeup_adapter(netdev);
        } else if(!eth_cmd.autoneg && !eth_cmd.speed) {
            printk("Cannot turn off autoneg without "
                   "knowing what speed to force the link\n");
            printk("Speed specified was %dMbps\n", eth_cmd.speed);
            return -EINVAL;
        }
        /* We donot support setting of 
         * whatever else that was requested */
        else
            return -EOPNOTSUPP;

        break;

#ifdef ETHTOOL_NWAY_RST
    case ETHTOOL_NWAY_RST:
        /* need proper permission to restart auto-negotiation */
        if(!capable(CAP_NET_ADMIN))
            return -EPERM;

        adapter->shared.autoneg = 1;
        adapter->shared.autoneg_advertised = AUTONEG_ADV_DEFAULT;
        e1000_hibernate_adapter(netdev);
        e1000_wakeup_adapter(netdev);

        break;
#endif

#ifdef ETHTOOL_GLINK
    case ETHTOOL_GLINK:
        eth_e1000_linkinfo.data = adapter->link_active;
        if(copy_to_user(ifr->ifr_data, &eth_e1000_linkinfo, sizeof(eth_e1000_linkinfo)))
            return -EFAULT;
        break;
#endif

#ifdef ETHTOOL_GDRVINFO
    case ETHTOOL_GDRVINFO:
        strcpy(eth_e1000_info.driver, e1000_driver_name);
        strcpy(eth_e1000_info.version, e1000_driver_version);
        strcpy(eth_e1000_info.fw_version, "None");
        strcpy(eth_e1000_info.bus_info, adapter->pdev->slot_name);
        if(copy_to_user(ifr->ifr_data, &eth_e1000_info, sizeof(eth_e1000_info)))
            return -EFAULT;
        break;
#endif

#ifdef ETHTOOL_GWOL
    case ETHTOOL_GWOL:
        eth_e1000_wolinfo.supported = eth_e1000_wolinfo.wolopts = WAKE_MAGIC;
        if(copy_to_user
           (ifr->ifr_data, &eth_e1000_wolinfo, sizeof(eth_e1000_wolinfo)))
            return -EFAULT;
        break;
#endif

    default:
        return -EOPNOTSUPP;
    }

    return 0;

}
#endif /* SIOCETHTOOL */

/**
 * e1000_enable_WOL - Wake On Lan Support (Magic Pkt)
 * @adapter: Adapter structure 
 **/

static void
e1000_enable_WOL(struct e1000_adapter *adapter)
{
    uint32_t wuc_val;

    if(adapter->shared.mac_type <= e1000_82543)
        return;

    /* Set up Wake-Up Ctrl reg */
    wuc_val = E1000_READ_REG(&adapter->shared, WUC);
    wuc_val &= ~(E1000_WUC_APME | E1000_WUC_APMPME);
    wuc_val |= (E1000_WUC_PME_STATUS | E1000_WUC_PME_EN);

    E1000_WRITE_REG(&adapter->shared, WUC, wuc_val);

    /* Set up Wake-up Filter */
    E1000_WRITE_REG(&adapter->shared, WUFC, E1000_WUFC_MAG);

    return;
}

/**
 * e1000_write_pci_cg -
 * @shared:
 * @reg:
 * @value:
 **/

void
e1000_write_pci_cfg(struct e1000_shared_adapter *shared,
                    uint32_t reg,
                    uint16_t *value)
{
    struct e1000_adapter *adapter = (struct e1000_adapter *) shared->back;

    pci_write_config_word(adapter->pdev, reg, *value);
    return;
}

/* e1000_main.c */
