/*
 * arch/arm/mm/mm-ixp1200.c
 *
 * Low level memory initialization for IXP1200 based systems
 *
 * Maintainer: Deepak Saxena <dsaxena@mvista.com>
 *
 * Mar-27-2000 - Uday Naik
 * 	Created for IXP1200 Eval board 
 * Nov-11-2001 - Deepak Saxena
 *      Cleanup, deleted SDRAM_PACKET mapping as we use all of SDRAM
 *      for the kernel.  If an application needs to reserve RAM for the
 *      uEngines, it should do so through bootmem_alloc() APIs.
 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/init.h>

#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/hardware.h>

#include <asm/mach/map.h>

struct map_desc ixp1200_io[] __initdata =
{
 { SRAM_BASE,    PHYS_SRAM_BASE,  SRAM_SIZE, DOMAIN_IO, 1, 1, 0, 0}, 
 { PCICFG0_BASE, PHYS_PCICFG0_BASE, PCICFG0_SIZE, DOMAIN_IO, 1, 1, 0, 0}, 
 { PCICFG1_BASE, PHYS_PCICFG1_BASE, PCICFG1_SIZE, DOMAIN_IO, 1, 1, 0, 0}, 
 { ARMCSR_BASE,  PHYS_ARMCSR_BASE,  ARMCSR_SIZE, DOMAIN_IO, 1, 1, 0, 0}, 
 { PC_IO_BASE, PHYS_PCI_IO, PCIO_SIZE, DOMAIN_IO, 1, 1, 0, 0}, 
 { PCI_CSR_BASE, PHYS_PCICSR_BASE, PCI_CSR_SIZE, DOMAIN_IO, 1, 1, 0, 0}, 
 { MICROENGINE_FBI_BASE, PHYS_UENGCSR_BASE, MICROENGINE_FBI_SIZE, DOMAIN_IO, 1, 1,0,0 }, 
 { SRAM_SLOW_PORT_BASE, PHYS_SRAM_SLOW_PORT_BASE, SRAM_SLOW_PORT_SIZE, DOMAIN_IO, 1, 1, 0, 0},
 { SRAM_TEST_SET_BITS, PHYS_SRAM_TEST_SET_BITS, SRAM_TEST_SET_BITS_SIZE, DOMAIN_IO, 1,1,0, 0}, 
 { SRAM_TEST_CLEAR_BITS, PHYS_SRAM_TEST_CLEAR_BITS,SRAM_TEST_CLEAR_BITS_SIZE, DOMAIN_IO, 1,1,0, 0}, 
 { SRAM_SET_BITS, PHYS_SRAM_SET_BITS, SRAM_SET_BITS_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SRAM_CLEAR_BITS, PHYS_SRAM_CLEAR_BITS, SRAM_CLEAR_BITS_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SRAM_CAM_UNLOCK, PHYS_SRAM_CAM_UNLOCK_BASE,SRAM_CAM_UNLOCK_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SRAM_WRITE_UNLOCK, PHYS_SRAM_WRITE_UNLOCK_BASE, SRAM_WRITE_UNLOCK_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SRAM_READ_LOCK, PHYS_SRAM_READ_LOCK_BASE, SRAM_READ_LOCK_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SRAM_PUSH_POPQ_BASE, PHYS_PUSH_POPQ_BASE, SRAM_PUSH_POPQ_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SRAM_CSR_BASE, PHYS_SRAM_CSR_BASE, SRAM_CSR_SIZE, DOMAIN_IO, 1,1,0, 0},
 { SDRAM_CSR_BASE, PHYS_SDRAM_CSR_BASE, SDRAM_CSR_SIZE, DOMAIN_IO, 1,1,0, 0},
 LAST_DESC
};

void __init ixp1200_map_io(void)
{
	iotable_init(ixp1200_io);
}
