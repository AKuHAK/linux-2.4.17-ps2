/*
 * ixp425-io.c 
 *
 *  Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <asm/io.h>
#include <asm/arch/ixp425-pci.h>

void outb(u8 v, u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (IXP425_PCI_BOTTOM_NIBBLE_OF_LONG_MASK & ~BIT(n))
		<< IXP425_PCI_NP_CBE_BESL;
	data = v << (8*n);
	ixp425_pci_write(p, byte_enables | NP_CMD_IOWRITE, data);
}

void outw(u16 v, u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (IXP425_PCI_BOTTOM_NIBBLE_OF_LONG_MASK & ~(BIT(n) | BIT(n+1)))
		<< IXP425_PCI_NP_CBE_BESL;
	data = v << (8*n);
	ixp425_pci_write(p, byte_enables | NP_CMD_IOWRITE, data);
}

void outl(u32 v, u32 p)
{
	ixp425_pci_write(p, NP_CMD_IOWRITE, v);
}

u8 inb(u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (IXP425_PCI_BOTTOM_NIBBLE_OF_LONG_MASK & ~BIT(n))
		<< IXP425_PCI_NP_CBE_BESL;
	if (ixp425_pci_read(p, byte_enables | NP_CMD_IOREAD, &data))
		return 0xff;

	return data >> (8*n);
}

u16 inw(u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (IXP425_PCI_BOTTOM_NIBBLE_OF_LONG_MASK & ~(BIT(n) | BIT(n+1)))
		<< IXP425_PCI_NP_CBE_BESL;
	if (ixp425_pci_read(p, byte_enables | NP_CMD_IOREAD, &data))
		return 0xffff;

	return data>>(8*n);
}

u32 inl(u32 p)
{
	u32 data;
	if (ixp425_pci_read(p, NP_CMD_IOREAD, &data))
		return 0xffffffff;

	return data;
}

void outsb(u32 p, u8 *addr, u32 count)
{
	while (count--)
		outb(*addr++, p);
}

void outsw(u32 p, u16 *addr, u32 count)
{
	while (count--)
		outw(*addr++, p);
}

void outsl(u32 p, u32 *addr, u32 count)
{
	while (count--)
		outl(*addr++, p);
}

void insb(u32 p, u8 *addr, u32 count)
{
	while (count--)
		*addr++ = inb(p);
}

void insw(u32 p, u16 *addr, u32 count)
{
	while (count--)
		*addr++ = inw(p);
}

void insl(u32 p, u32 *addr, u32 count)
{
	while (count--)
		*addr++ = inl(p);
}

EXPORT_SYMBOL(outb);
EXPORT_SYMBOL(outw);
EXPORT_SYMBOL(outl);
EXPORT_SYMBOL(inb);
EXPORT_SYMBOL(inw);
EXPORT_SYMBOL(inl);
EXPORT_SYMBOL(outsb);
EXPORT_SYMBOL(outsw);
EXPORT_SYMBOL(outsl);
EXPORT_SYMBOL(insb);
EXPORT_SYMBOL(insw);
EXPORT_SYMBOL(insl);
