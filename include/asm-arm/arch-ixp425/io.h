/*
 * linux/include/asm-arm/arch-ixp425/io.h
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 *
 * Copyright (C) 2001  MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#define IO_SPACE_LIMIT 0xffffffff

#define __io_pci(a)		(PCIO_BASE + (a))
#define __mem_pci(a)		((unsigned long)(a))
#define __mem_isa(a)		((unsigned long)(a))

#define __ioaddr(p)		__io_pci(p)
#define __io(p)			__io_pci(p)

/* undefine __io so we can provide machine specific {out,in,outs,ins}[bwl] */
#undef __io

void outb(u8 v, u32 p);
void outw(u16 v, u32 p);
void outl(u32 v, u32 p);

u8 inb(u32 p);
u16 inw(u32 p);
u32 inl(u32 p);

void outsb(u32 p, u8 *addr, u32 count);
void outsw(u32 p, u16 *addr, u32 count);
void outsl(u32 p, u32 *addr, u32 count);

void insb(u32 p, u8 *addr, u32 count);
void insw(u32 p, u16 *addr, u32 count);
void insl(u32 p, u32 *addr, u32 count);
/*
 * Generic virtual read/write
 */
#define __arch_getb(a)		(*(volatile unsigned char *)(a))
#define __arch_getl(a)		(*(volatile unsigned int  *)(a))

extern __inline__ unsigned int __arch_getw(unsigned long a)
{
	unsigned int value;
	__asm__ __volatile__("ldr%?h	%0, [%1, #0]	@ getw"
		: "=&r" (value)
		: "r" (a));
	return value;
}


#define __arch_putb(v,a)	(*(volatile unsigned char *)(a) = (v))
#define __arch_putl(v,a)	(*(volatile unsigned int  *)(a) = (v))

extern __inline__ void __arch_putw(unsigned int value, unsigned long a)
{
	__asm__ __volatile__("str%?h	%0, [%1, #0]	@ putw"
		: : "r" (value), "r" (a));
}

#define __arch_ioremap		__ioremap
#define __arch_iounmap		__iounmap

#endif
