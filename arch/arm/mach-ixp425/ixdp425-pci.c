/*
 * ixp425-pci.c 
 *
 *  Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <asm/mach/pci.h>
#include <asm/arch/irqs.h>
#include <asm/arch/pci.h>
#include <asm/arch/ixp425-gpio.h>
#include <asm/arch/ixp425-pci.h>

#define INTA    IRQ_IXP425_INTC_GPIO11
#define INTB    IRQ_IXP425_INTC_GPIO10
#define INTC    IRQ_IXP425_INTC_GPIO9
#define INTD    IRQ_IXP425_INTC_GPIO8

#define IXP425_PCI_MAX_DEV      4
#define IXP425_PCI_IRQ_LINES    4

/* PCI controller pin mappings */
#define IXP425_PCI_RESET_GPIO   IXP425_GPIO_PIN_13
#define IXP425_PCI_CLK_PIN      IXP425_GPIO_CLK_0
#define IXP425_PCI_CLK_ENABLE   IXP425_GPIO_CLK0_ENABLE
#define IXP425_PCI_CLK_TC_LSH   IXP425_GPIO_CLK0TC_LSH
#define IXP425_PCI_CLK_DC_LSH   IXP425_GPIO_CLK0DC_LSH

#ifdef CONFIG_PCI_RESET

void __init ixdp425_pci_hw_init(void)
{
	/* Disable PCI clock */
	*IXP425_GPIO_GPCLKR &= ~IXP425_PCI_CLK_ENABLE;

	/* configure PCI-related GPIO */
	gpio_line_config(IXP425_PCI_CLK_PIN, IXP425_GPIO_OUT);
	gpio_line_config(IXP425_PCI_RESET_GPIO, IXP425_GPIO_OUT);

	gpio_line_config(INTA, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);
	gpio_line_config(INTB, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);
	gpio_line_config(INTC, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);
	gpio_line_config(INTD, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);

	gpio_line_isr_clear(INTA);
	gpio_line_isr_clear(INTB);
	gpio_line_isr_clear(INTC);
	gpio_line_isr_clear(INTD);

	/* Assert reset for PCI controller */
	gpio_line_set(IXP425_PCI_RESET_GPIO, IXP425_GPIO_LOW);
	/* wait 1ms to satisfy "minimum reset assertion time" of the PCI spec. */
	udelay(1000);
	/* Config PCI clock */
	*IXP425_GPIO_GPCLKR |= (0xf << IXP425_PCI_CLK_TC_LSH) | 
	    (0xf << IXP425_PCI_CLK_DC_LSH);
	/* Enable PCI clock */
	*IXP425_GPIO_GPCLKR |= IXP425_PCI_CLK_ENABLE;
	/* wait 100us to satisfy "minimum reset assertion time from clock stable"
	 * requirement of the PCI spec. */
	udelay(100);
	/* Deassert reset for PCI controller */
	gpio_line_set(IXP425_PCI_RESET_GPIO, IXP425_GPIO_HIGH);

	/* wait a while to let other devices get ready after PCI reset */
	udelay(1000);
}

#endif

void __init ixdp425_pci_init(void *sysdata)
{
#ifdef CONFIG_PCI_RESET
	if (ixp425_pci_is_host())
		ixdp425_pci_hw_init();
#endif
	ixp425_pci_init(sysdata);
}

static int __init ixdp425_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	static int pci_irq_table[IXP425_PCI_MAX_DEV][IXP425_PCI_IRQ_LINES] = 
	{
		{INTA, INTB, INTC, INTD},
		{INTB, INTC, INTD, INTA},
		{INTC, INTD, INTA, INTB},
		{INTD, INTA, INTB, INTC}
	};

	int irq = -1;

	if (slot >= 1 && slot <= IXP425_PCI_MAX_DEV && 
		pin >= 1 && pin <= IXP425_PCI_IRQ_LINES)
	{
		irq = pci_irq_table[slot-1][pin-1];
	}

	return irq;
}

struct hw_pci ixdp425_pci __initdata = {
	init:		ixdp425_pci_init,
	swizzle:	common_swizzle,
	map_irq:	ixdp425_map_irq,
};

