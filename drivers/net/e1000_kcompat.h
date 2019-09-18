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

/*******************************************************************************

  Recipient has requested a license and Intel Corporation ("Intel") is
  willing to grant a license for the software entitled Linux Base Driver
  for the Intel(R) PRO/1000 Familty of Adapters (e1000) (the "Software")
  being provided by Intel Corporation.
  
  The following definitions apply to this License:
  
  "Licensed Patents" means patent claims licensable by Intel Corporation
  which are necessarily infringed by the use or sale of the Software alone
  or when combined with the operating system referred to below.
  "Recipient" means the party to whom Intel delivers this Software.
  "Licensee" means Recipient and those third parties that receive a license
  to any operating system available under the GNU Public License version
  2.0 or later.
  
  Copyright (c) 1999-2002 Intel Corporation All rights reserved.
  
  The license is provided to Recipient and Recipient's Licensees under
  the following terms.
  
  Redistribution and use in source and binary forms of the Software,
  with or without modification, are permitted provided that the following
  conditions are met:
  
  Redistributions of source code of the Software may retain the above
  copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form of the Software may reproduce the above
  copyright notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the distribution.
  Neither the name of Intel Corporation nor the names of its contributors
  shall be used to endorse or promote products derived from this Software
  without specific prior written permission.
  
  Intel hereby grants Recipient and Licensees a non-exclusive, worldwide,
  royalty-free patent license under Licensed Patents to make, use, sell,
  offer to sell, import and otherwise transfer the Software, if any, in
  source code and object code form. This license shall include changes
  to the Software that are error corrections or other minor changes
  to the Software that do not add functionality or features when the
  Software is incorporated in any version of a operating system that has
  been distributed under the GNU General Public License 2.0 or later.
  This patent license shall apply to the combination of the Software and
  any operating system licensed under the GNU Public License version 2.0
  or later if, at the time Intel provides the Software to Recipient, such
  addition of the Software to the then publicly available versions of such
  operating system available under the GNU Public License version 2.0 or
  later (whether in gold, beta or alpha form) causes such combination to
  be covered by the Licensed Patents. The patent license shall not apply
  to any other combinations which include the Software. No hardware per
  se is licensed hereunder.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS CONTRIBUTORS
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/* Macros to make drivers compatible with 2.2, 2.4 Linux kernels
 *
 * In order to make a single network driver work with all 2.2, 2.4 kernels
 * these compatibility macros can be used.
 * They are backwards compatible implementations of the latest APIs.
 * The idea is that these macros will let you use the newest driver with old
 * kernels, but can be removed when working with the latest and greatest.
 */

/* When replacing a kernel function, an inline function is used instead of
 * a macro to ensure compile time type checking of the arguments.
 *
 * All inline functions are prefixed with _kc and a #define is used to map
 * it to the real name - because otherwise things get broken when a new API
 * is back-ported in someones modified kernel.
 */

#ifndef E1000_LINUX_KERNEL_COMPAT_H
#define E1000_LINUX_KERNEL_COMPAT_H

#include <linux/version.h>

/*****************************************************************************
 **
 **  PCI Bus Changes
 **
 *****************************************************************************/

/* Accessing the BAR registers from the PCI device structure
 * Changed from base_address[bar] to resource[bar].start in 2.3.13
 * The pci_resource_start inline function was introduced in 2.3.43 
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,13) )
#ifndef pci_resource_start
#define pci_resource_start(dev, bar) \
        (((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_SPACE) ? \
         ((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_IO_MASK) : \
         ((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_MEM_MASK))
#endif
#elif ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,43) )
#ifndef pci_resource_start
#define pci_resource_start(dev, bar) \
        (((dev)->resource[(bar)] & PCI_BASE_ADDRESS_SPACE) ? \
         ((dev)->resource[(bar)] & PCI_BASE_ADDRESS_IO_MASK) : \
         ((dev)->resource[(bar)] & PCI_BASE_ADDRESS_MEM_MASK))
#endif
#endif

/* Starting with 2.3.23 drivers are supposed to call pci_enable_device
 * to make sure I/O and memory regions have been mapped and potentially 
 * bring the device out of a low power state
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,23) )
#include <linux/pci.h>
#ifndef pci_enable_device
#define pci_enable_device _kc_pci_enable_device
static inline int
_kc_pci_enable_device(struct pci_dev *dev)
{ return 0; }
#endif
#endif

/* Dynamic DMA mapping
 * Instead of using virt_to_bus, bus mastering PCI drivers should use the DMA 
 * mapping API to get bus addresses.  This lets some platforms use dynamic 
 * mapping to use PCI devices that do not support DAC in a 64-bit address space
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,4,3) )
#if   ( LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,41) )
#include <linux/pci.h>
#ifndef pci_set_dma_mask
#define pci_set_dma_mask _kc_pci_set_dma_mask
static inline int
_kc_pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
    if(!pci_dma_supported(dev, mask))
        return -EIO;
    dev->dma_mask = mask;
    return 0;
}
#endif
#endif
#endif

#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,41) )

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <asm/io.h>

#if (( LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18) ) || \
     ( LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0) ) )

typedef unsigned long dma_addr_t;

#endif

#define PCI_DMA_TODEVICE   1
#define PCI_DMA_FROMDEVICE 2
#ifndef pci_alloc_consistent
#define pci_alloc_consistent _kc_pci_alloc_consistent
static inline void *
_kc_pci_alloc_consistent (struct pci_dev *dev, size_t size,
                          dma_addr_t *dma_handle)
{
    void *vaddr = kmalloc(size, GFP_KERNEL);

    if(vaddr != NULL) {
        *dma_handle = virt_to_bus(vaddr);
    }
    return vaddr; 
}
#endif

#ifndef pci_dma_supported
#define pci_dma_supported _kc_pci_dma_supported
static inline int
_kc_pci_dma_supported(struct pci_dev *dev, u64 mask)
{ return 1; }
#endif

#ifndef pci_set_dma_mask
#define pci_set_dma_mask _kc_pci_set_dma_mask
static inline int
_kc_pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{ return 0; }
#endif

#ifndef pci_free_consistent
#define pci_free_consistent _kc_pci_free_consistent
static inline void _kc_pci_free_consistent(struct pci_dev *hwdev,
                                           size_t size, 
                                           void *cpu_addr,
                                           dma_addr_t dma_handle)
{ kfree(cpu_addr); return; }
#endif

#ifndef pci_map_single
#define pci_map_single _kc_pci_map_single
static inline u64 _kc_pci_map_single(struct pci_dev *hwdev, void *ptr,
                                            size_t size, int direction)
{ return virt_to_bus(ptr); }
#endif

#ifndef pci_unmap_single
#define pci_unmap_single _kc_pci_unmap_single
static inline void
_kc_pci_unmap_single(struct pci_dev *hwdev, u64 dma_addr, 
                     size_t size, int direction)
{ return; }
#endif

#ifndef pci_resource_len
#define pci_resource_len _kc_pci_resource_len
static inline unsigned long
_kc_pci_resource_len(struct pci_dev *pdev, int bar)
{
    u32 old, len;
    
    int bar_reg = PCI_BASE_ADDRESS_0 + (bar << 2);
    
    pci_read_config_dword(pdev, bar_reg, &old);
    pci_write_config_dword(pdev, bar_reg, ~0);
    pci_read_config_dword(pdev, bar_reg, &len);
    pci_write_config_dword(pdev, bar_reg, old);

    if((len & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_MEMORY)
        len = ~(len & PCI_BASE_ADDRESS_MEM_MASK);
    else
        len = ~(len & PCI_BASE_ADDRESS_IO_MASK) & 0xffff;
    
    return (len + 1);
}
#endif

#ifndef request_mem_region
#define request_mem_region _kc_request_mem_region
static inline int
_kc_request_mem_region(unsigned long addr, ...)
{ return 1; }
#endif

#ifndef release_mem_region
#define release_mem_region _kc_release_mem_region
static inline int
_kc_release_mem_region(unsigned long addr, ...)
{ return 0; }
#endif

#endif

/*****************************************************************************
 **
 **  Network Device API Changes
 **
 *****************************************************************************/

/* In 2.3.14 the device structure was renamed to net_device 
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,14) )
#ifndef net_device
#define net_device device
#endif
#endif

/* 'Softnet' network stack changes merged in 2.3.43 
 * these are 2.2 compatible defines for the new network interface API
 * 2.3.47 added some more inline functions for softnet to remove explicit 
 * bit tests in drivers
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,43) )
#ifndef netif_start_queue
#define netif_start_queue(dev)   clear_bit  (0, &(dev)->tbusy)
#endif
#ifndef netif_stop_queue
#define netif_stop_queue(dev)    set_bit    (0, &(dev)->tbusy)
#endif
#ifndef netif_wake_queue
#define netif_wake_queue(dev)    { clear_bit(0, &(dev)->tbusy); \
                                                mark_bh(NET_BH); }
#endif
#ifndef netif_running
#define netif_running(dev)       test_bit(0, &(dev)->start)
#endif
#ifndef netif_queue_stopped
#define netif_queue_stopped(dev) test_bit(0, &(dev)->tbusy)
#endif
#elif ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,47) )
#ifndef netif_running
#define netif_running(dev)       test_bit(LINK_STATE_START, &(dev)->state)
#endif
#ifndef netif_queue_stopped
#define netif_queue_stopped(dev) test_bit(LINK_STATE_XOFF,  &(dev)->state)
#endif
#endif

/* Softnet changes also affected how SKBs are handled
 * Special calls need to be made now while in an interrupt handler
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,43) )
#ifndef dev_kfree_skb_irq
#define dev_kfree_skb_irq dev_kfree_skb
#endif
#endif

/* To avoid possible race conditions,
 * we allocate a net_device first, and register it later.
 * This function was copied from the 2.4.17 kernel and
 * modified to rid of the last two input parameters.
 */
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,4,3) )
#ifndef alloc_etherdev
#define alloc_etherdev _kc_alloc_etherdev
static inline struct net_device *_kc_alloc_etherdev(int sizeof_priv)
{
	struct net_device *dev;
	int alloc_size;

	alloc_size = sizeof (*dev) + sizeof_priv + 31 + IFNAMSIZ;

	dev = (struct net_device *) kmalloc (alloc_size, GFP_KERNEL);
	if (dev == NULL) {
		printk(KERN_ERR "alloc_dev: Unable to allocate device memory.\n");
		return NULL;
	}

	memset(dev, 0, alloc_size);

	if (sizeof_priv)
		dev->priv = (void *) (((long)(dev + 1) + 31) & ~31);

	ether_setup(dev);
#if   ( LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0) )
	dev->name = (char *) dev->priv + sizeof_priv;
#endif
	strcpy(dev->name, "");

	return dev;
}
#endif
#endif
/*****************************************************************************
 **
 **  General Module / Driver / Kernel API Changes
 **
 *****************************************************************************/

/* New module_init macro added in 2.3.13 - replaces init_module entry point
 * If MODULE is defined, it expands to an init_module definition
 * If the driver is staticly linked to the kernel, it creates the proper 
 * function pointer for the initialization routine to be called
 * (no more Space.c)
 * module_exit does the same thing for cleanup_module
 */
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,13) )
#ifndef module_init
#define module_init(fn) int  init_module   (void) { return fn(); }
#endif
#ifndef module_exit
#define module_exit(fn) void cleanup_module(void) { return fn(); }
#endif
#endif

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,47) )
#include <linux/pci.h>
#include <linux/module.h>

#ifndef PCI_ANY_ID
#define PCI_ANY_ID (~0U)
#endif

struct pci_device_id {
    unsigned int vendor, device;
    unsigned int subvendor, subdevice;
    unsigned int class, classmask;
    unsigned long driver_data;
};

#ifndef MODULE_DEVICE_TABLE
#define MODULE_DEVICE_TABLE(bus, dev_table)
#endif

struct pci_driver {
    char *name;
    struct pci_device_id *id_table;
    int (*probe)(struct pci_dev *dev, const struct pci_device_id *id);
    void (*remove)(struct pci_dev *dev);
    void (*suspend)(struct pci_dev *dev);
    void (*resume)(struct pci_dev *dev);
    /* track devices on Linux 2.2, used by module_init and unregister_driver */
    /* not to be used by the driver directly */
    /* assumes single function device with function #0 to simplify */
    uint32_t pcimap[256];
};

#ifndef pci_module_init
#define pci_module_init _kc_pci_module_init
static inline int _kc_pci_module_init(struct pci_driver *drv)
{
    struct pci_dev *pdev;
    struct pci_device_id *pciid;
    uint16_t subvendor, subdevice;
    int board_count = 0;

    /* walk the global pci device list looking for matches */
    for(pdev = pci_devices; pdev != NULL; pdev = pdev->next) {

        pciid = &drv->id_table[0];
        pci_read_config_word(pdev, PCI_SUBSYSTEM_VENDOR_ID, &subvendor);
        pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &subdevice);
        
        while(pciid->vendor != 0) {
            if(((pciid->vendor == pdev->vendor) ||
                (pciid->vendor == PCI_ANY_ID)) &&

               ((pciid->device == pdev->device) ||
                (pciid->device == PCI_ANY_ID)) &&

               ((pciid->subvendor == subvendor) ||
                (pciid->subvendor == PCI_ANY_ID)) &&

               ((pciid->subdevice == subdevice) ||
                (pciid->subdevice == PCI_ANY_ID))) {

                if(drv->probe(pdev, pciid) == 0) {
                    board_count++;

                    /* keep track of pci devices found */
                    set_bit((pdev->devfn >> 3),
                            &(drv->pcimap[pdev->bus->number]));
                }
                break;
            }
            pciid++;
        }
    }

    return (board_count > 0) ? 0 : -ENODEV;
}
#endif

#ifndef pci_unregister_driver
#define pci_unregister_driver _kc_pci_unregister_driver
static inline void _kc_pci_unregister_driver(struct pci_driver *drv)
{
    int i, bit;
    struct pci_dev *pdev;

    /* search the pci device bitmap and release them all */
    for(i = 0; i < 256; i++) {
        /* ffs = find first set bit */
        for(bit = ffs(drv->pcimap[i]); bit > 0; bit = ffs(drv->pcimap[i])) {
            bit--;
            pdev = pci_find_slot(i, (bit << 3));
            drv->remove(pdev);
            clear_bit(bit, &drv->pcimap[i]);
        }
    }
    return;
}
#endif

#endif

/* Taslets */

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,43) )

#include <linux/interrupt.h>
#ifndef tasklet_struct
#define tasklet_struct tq_struct
#endif

#ifndef tasklet_init
#define tasklet_init _kc_tasklet_init
static inline void _kc_tasklet_init(struct tasklet_struct *t,
                void (*func)(unsigned long), unsigned long data)
{
    t->next = NULL;
    t->sync = 0;
    t->routine = (void *)(void *)func;
    t->data = (void *)data;
}
#endif

#ifndef tasklet_schedule
#define tasklet_schedule _kc_tasklet_schedule
static inline void _kc_tasklet_schedule(struct tasklet_struct *t)
{
    queue_task(t, &tq_immediate);
    mark_bh(IMMEDIATE_BH);
    return;
}
#endif

#ifndef tasklet_disable
#define tasklet_disable _kc_tasklet_disable
static inline void tasklet_disable(struct tasklet_struct *t)
{
    return;
}
#endif

#ifndef tasklet_enable
#define tasklet_enable _kc_tasklet_enable
static inline void _kc_tasklet_enable(struct tasklet_struct *t)
{
    return;
}
#endif

#ifndef tasklet_kill
#define tasklet_kill _kc_tasklet_kill
static inline void _kc_tasklet_kill(struct tasklet_struct *t)
{
    return;
}
#endif

#endif

/* Timer */
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) )

#ifndef del_timer_sync
#define del_timer_sync(timer_id) del_timer(timer_id)
#endif

#endif
/* End of Timer */

/* pci_map_page / pci_unmap_page */
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,4,13) )

#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/pci.h>
#include <asm/io.h>

#ifndef virt_to_page 
#define virt_to_page(v) (mem_map + (virt_to_phys(v) >> PAGE_SHIFT))
#endif

#ifndef pci_map_page
#define pci_map_page _kc_pci_map_page

#if defined(CONFIG_HIGHMEM)

#ifndef PCI_DRAM_OFFSET
#define PCI_DRAM_OFFSET 0
#endif

static inline u64 _kc_pci_map_page(struct pci_dev *hwdev,
                                      struct page *page,
                                      unsigned long offset,
                                      size_t size,
                                      int direction) {

    return  (((u64)(page - mem_map) << PAGE_SHIFT) + offset + PCI_DRAM_OFFSET);
}

#undef DMAADDR_OFFSET

#else

static inline u64 _kc_pci_map_page(struct pci_dev *hwdev,
                                      struct page *page,
                                      unsigned long offset,
                                      size_t size,
                                      int direction) {
    return pci_map_single(hwdev,
                          (void *) page_address(page) + offset,
                          size,
                          direction);
}

#endif
#endif /* pci_map_page */

#ifndef pci_unmap_page
#define pci_unmap_page _kc_pci_unmap_page
static inline void _kc_pci_unmap_page(struct pci_dev *hwdev,
                                  u64 dma_address,
                                  size_t size,
                                  int direction) {

    return pci_unmap_single(hwdev, dma_address, size, direction);
}
#endif

#endif /* pci_map_page / pci_unmap_page */

#endif /* E1000_LINUX_KERNEL_COMPAT_H */

