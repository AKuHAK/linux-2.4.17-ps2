/*
 * irq.h 
 *
 *  Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _ARCH_IXP425_IRQS_H_
#define _ARCH_IXP425_IRQS_H_

#define NR_IRQS				32

#define IRQ_IXP425_INTC_NPEA		0
#define IRQ_IXP425_INTC_NPEB		1
#define IRQ_IXP425_INTC_NPEC		2
#define IRQ_IXP425_INTC_QM1		3
#define IRQ_IXP425_INTC_QM2		4
#define IRQ_IXP425_INTC_TIMER1		5
#define IRQ_IXP425_INTC_GPIO0		6
#define IRQ_IXP425_INTC_GPIO1		7
#define IRQ_IXP425_INTC_PCI_INT		8
#define IRQ_IXP425_INTC_PCI_DMA1	9
#define IRQ_IXP425_INTC_PCI_DMA2	10
#define IRQ_IXP425_INTC_TIMER2		11
#define IRQ_IXP425_INTC_USB		12
#define IRQ_IXP425_INTC_UART2		13
#define IRQ_IXP425_INTC_TIMESTAMP	14
#define IRQ_IXP425_INTC_UART1		15
#define IRQ_IXP425_INTC_WDOG		16
#define IRQ_IXP425_INTC_AHB_PMU		17
#define IRQ_IXP425_INTC_XSCALE_PMU	18
#define IRQ_IXP425_INTC_GPIO2		19
#define IRQ_IXP425_INTC_GPIO3		20
#define IRQ_IXP425_INTC_GPIO4		21
#define IRQ_IXP425_INTC_GPIO5		22
#define IRQ_IXP425_INTC_GPIO6		23
#define IRQ_IXP425_INTC_GPIO7		24
#define IRQ_IXP425_INTC_GPIO8		25
#define IRQ_IXP425_INTC_GPIO9		26
#define IRQ_IXP425_INTC_GPIO10		27
#define IRQ_IXP425_INTC_GPIO11		28
#define IRQ_IXP425_INTC_GPIO12		29
#define IRQ_IXP425_INTC_SW_INT1		30
#define IRQ_IXP425_INTC_SW_INT2		31

#define	XSCALE_PMU_IRQ		(IRQ_IXP425_INTC_XSCALE_PMU)

#endif
