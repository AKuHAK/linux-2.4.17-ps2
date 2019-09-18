#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/ioport.h>
#include <asm/io.h>

#define WINDOW_ADDR 	0x50000000
#define BUSWIDTH 	2
#define WINDOW_SIZE	0x01000000

#ifndef __ARMEB__
#define	B0(h)	((h) & 0xFF)
#define	B1(h)	(((h) >> 8) & 0xFF)
#else
#define	B0(h)	(((h) >> 8) & 0xFF)
#define	B1(h)	((h) & 0xFF)
#endif

static __u16 ixdp425_read16(struct map_info *map, unsigned long ofs)
{
    return *(__u16 *)(map->map_priv_1 + ofs);
}

/*
 * The IXP425 expansion bus only allows 16-bit wide acceses
 * when attached to a 16-bit wide device (such as the 28F128J3A),
 * so we can't use a memcpy_fromio as it does byte acceses.
 */
static void ixdp425_copy_from(struct map_info *map, void *to,
    unsigned long from, ssize_t len)
{
	int i;
	u8 *dest = (u8*)to;
	u16 *src = (u16 *)(map->map_priv_1 + from);
	u16 data;

	for(i = 0; i < (len / 2); i++) {
		data = src[i];
		dest[i * 2] = B0(data);
		dest[i * 2 + 1] = B1(data);
	}

	if(len & 1)
		dest[len - 1] = B0(src[i]);
}

static void ixdp425_write16(struct map_info *map, __u16 d, unsigned long adr)
{
    *(__u16 *)(map->map_priv_1 + adr) = d;
}

static struct map_info ixdp425_map = {
	name: 		"ixdp425 Flash",
	buswidth: 	BUSWIDTH,
	read16:		ixdp425_read16,
	copy_from:	ixdp425_copy_from,
	write16:	ixdp425_write16,
};

#ifdef CONFIG_MTD_REDBOOT_PARTS
static struct mtd_partition *parsed_parts;
#endif

static struct mtd_partition ixdp425_partitions[] = {
    {
	name:	"image",
	offset:	0x00040000,
	size:	0x00400000, /* 4M for linux kernel + cramfs + initrd image */
    },
    {
	name:	"user",
	offset:	0x00440000,
	size:	0x00B80000, /* Rest of flash space minus redboot configuration
	                     * sectors (-256k) at the end of flash
			     */
    },
};

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

static struct mtd_info *ixdp425_mtd;
static struct resource *mtd_resource;

static void ixdp425_exit(void)
{
    if (ixdp425_mtd)
    {
	del_mtd_partitions(ixdp425_mtd);
	map_destroy(ixdp425_mtd);
    }
    if (ixdp425_map.map_priv_1)
	iounmap((void *)ixdp425_map.map_priv_1);
    if (mtd_resource)
	release_mem_region(WINDOW_ADDR, WINDOW_SIZE);
    
#ifdef CONFIG_MTD_REDBOOT_PARTS
    if (parsed_parts)
	kfree(parsed_parts);
#endif

    /* Disable flash write */
    *IXP425_EXP_CS0 &= ~IXP425_FLASH_WRITABLE;
}

static int __init ixdp425_init(void)
{
    int res, npart;

    /* Enable flash write */
    *IXP425_EXP_CS0 |= IXP425_FLASH_WRITABLE;

    ixdp425_map.map_priv_1 = 0;
    mtd_resource = 
	request_mem_region(WINDOW_ADDR, WINDOW_SIZE, "ixdp425 Flash");
    if (!mtd_resource)
    {
	printk(KERN_ERR "ixdp425 flash: Could not request mem region.\n" );
	res = -ENOMEM;
	goto Error;
    }

    ixdp425_map.map_priv_1 =
	(unsigned long)ioremap(WINDOW_ADDR, WINDOW_SIZE);
    if (!ixdp425_map.map_priv_1)
    {
	printk("ixdp425 Flash: Failed to map IO region. (ioremap)\n");
	res = -EIO;
	goto Error;
    }
    ixdp425_map.size = WINDOW_SIZE;


    /* 
     * Probe for the CFI complaint chip
     * suposed to be 28F128J3A
     */
    ixdp425_mtd = do_map_probe("cfi_probe", &ixdp425_map);
    if (!ixdp425_mtd)
    {
	res = -ENXIO;
	goto Error;
    }
    ixdp425_mtd->module = THIS_MODULE;
   
    /* Initialize flash partiotions 
     * Note: Redeboot partition info table can be parsed by MTD, and used
     *       instead of hard-coded partions. TBD
     */

#ifdef CONFIG_MTD_REDBOOT_PARTS
    /* Try to parse RedBoot partitions */
    npart = parse_redboot_partitions(ixdp425_mtd, &parsed_parts);
    if (npart > 0)
    {
	/* found "npart" RedBoot partitions */
	
	res = add_mtd_partitions(ixdp425_mtd, parsed_parts, npart);
    }
    else   
	res = -EIO;

    if (res)
#endif
    {
	printk("Using predefined MTD partitions.\n");
	/* RedBoot partitions not found - use hardcoded partition table */
	res = add_mtd_partitions(ixdp425_mtd, ixdp425_partitions,
	    NB_OF(ixdp425_partitions));
    }

    if (res)
	goto Error;

    return res;
Error:
    ixdp425_exit();
    return res;
}

module_init(ixdp425_init);
module_exit(ixdp425_exit);

MODULE_DESCRIPTION("MTD map driver for ixdp425 evaluation board");

