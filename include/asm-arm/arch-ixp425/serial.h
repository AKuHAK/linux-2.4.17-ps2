/*
 * include/asm-arm/arch-ixp425/serial.h
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 * Modified for ixp425 offsets pbarry-intel
 * 
 *
 * Copyright (c) 2001 MontaVista Software, Inc.
 * 
 * 2002: Modified for IXP425 by Intel Corporation.
 *
 */

#ifndef _ARCH_SERIAL_H_
#define _ARCH_SERIAL_H_


/* Standard COM flags */
#define STD_COM_FLAGS ( ASYNC_SKIP_TEST)

/*
 * We use SERIAL_IO_MEM type since these are memory mapped UARTs,
 * not ISA style I/O ports.
 */
 
#if defined(CONFIG_ARCH_IXP425)

#undef BASE_BAUD

/*
 * IXP425 uses 15.6MHz clock for uart
 */
#define BASE_BAUD ( IXP425_UART_XTAL / 16 )

#define	RS_TABLE_SIZE	2

#define STD_SERIAL_PORT_DEFNS				\
	{						\
	  type: PORT_IXP425,				\
	  xmit_fifo_size: 64,				\
	  baud_base: BASE_BAUD,				\
	  irq: IRQ_IXP425_INTC_UART1,	       		\
	  flags: STD_COM_FLAGS,				\
	  iomem_base: (IXP425_UART1_BASE_VIRT+3),		\
	  io_type: SERIAL_IO_MEM,			\
	  iomem_reg_shift: 2				\
	}, /* ttyS0 */					\
	{						\
	  type: PORT_IXP425,				\
	  xmit_fifo_size: 64,				\
	  baud_base: BASE_BAUD,				\
	  irq: IRQ_IXP425_INTC_UART2,		       	\
	  flags: STD_COM_FLAGS,				\
	  iomem_base: (IXP425_UART2_BASE_VIRT+3),    	\
	  io_type: SERIAL_IO_MEM,			\
	  iomem_reg_shift: 2				\
	} /* ttyS1 */
#endif

#define EXTRA_SERIAL_PORT_DEFNS

#endif // _ARCH_SERIAL_H_
