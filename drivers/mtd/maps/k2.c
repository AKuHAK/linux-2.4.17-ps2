/*
 * Mapping for K2 user flash
 *
 * Matt Porter <mporter@mvista.com>
 *
 * Copyright 2002 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>
#include <asm/io.h>

static struct mtd_info *flash;

static __u8 k2_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

static __u16 k2_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

static __u32 k2_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

static void k2_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, map->map_priv_1 + from, len);
}

static void k2_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

static void k2_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

static void k2_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

static void k2_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio(map->map_priv_1 + to, from, len);
}

static struct map_info k2_map = {
	name: "K2 flash",
	size: 0x200000,
	buswidth: 1,
	read8: k2_read8,
	read16: k2_read16,
	read32: k2_read32,
	copy_from: k2_copy_from,
	write8: k2_write8,
	write16: k2_write16,
	write32: k2_write32,
	copy_to: k2_copy_to,
};

static struct mtd_partition k2_partitions[] = {
	{
		name: "Partition 0",
		offset: 0,
		size: (1024*1024)
	},
	{
		name: "Partition 1",
		offset: MTDPART_OFS_NXTBLK,
		size: (1024*1024)
	}
};

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

int __init init_k2(void)
{
	k2_map.map_priv_1 =
		(unsigned long)ioremap(0xffe00000, k2_map.size);

	if (!k2_map.map_priv_1) {
		printk("Failed to ioremap flash\n");
		return -EIO;
	}

	flash = do_map_probe("cfi_probe", &k2_map);
	if (flash) {
		flash->module = THIS_MODULE;
		add_mtd_partitions(flash, k2_partitions,
					NB_OF(k2_partitions));
	} else {
		printk("map probe failed for flash\n");
		return -ENXIO;
	}

	return 0;
}

static void __exit cleanup_k2(void)
{
	if (flash) {
		del_mtd_partitions(flash);
		map_destroy(flash);
	}

	if (k2_map.map_priv_1) {
		iounmap((void *)k2_map.map_priv_1);
		k2_map.map_priv_1 = 0;
	}
}

module_init(init_k2);
module_exit(cleanup_k2);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("K2 flash map");
