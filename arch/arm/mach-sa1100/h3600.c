/*
 * Hardware definitions for Compaq iPAQ H3xxx Handheld Computers
 *
 * Copyright 2000,1 Compaq Computer Corporation.
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Author: Jamey Hicks.
 *
 * History:
 *
 * 2001-10-??   Andrew Christian   Added support for iPAQ H3800
 *                                 and abstracted EGPIO interface.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/pm.h>

#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/setup.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/serial_sa1100.h>
#include <linux/serial_core.h>

#if defined (CONFIG_SA1100_H3600) || defined (CONFIG_SA1100_H3100)
#include <asm/arch/h3600_gpio.h>
#endif

#ifdef CONFIG_SA1100_H3800
#include <asm/arch/h3600_asic.h>
#endif

#include "generic.h"


struct ipaq_model_ops ipaq_model_ops;
EXPORT_SYMBOL(ipaq_model_ops);


static void h3600_lcd_power(int on)
{
	if (on)
		set_h3600_egpio(IPAQ_EGPIO_LCD_ON);
	else
		clr_h3600_egpio(IPAQ_EGPIO_LCD_ON);
}

/*
 * low-level UART features
 */

static void h3600_uart_set_mctrl(struct uart_port *port, u_int mctrl)
{
	if (port->mapbase == _Ser3UTCR0) {
		if (mctrl & TIOCM_RTS)
			GPCR = GPIO_H3600_COM_RTS;
		else
			GPSR = GPIO_H3600_COM_RTS;
	}
}

static u_int h3600_uart_get_mctrl(struct uart_port *port)
{
	u_int ret = TIOCM_CD | TIOCM_CTS | TIOCM_DSR;

	if (port->mapbase == _Ser3UTCR0) {
		int gplr = GPLR;
                /* DCD and CTS bits are inverted in GPLR by RS232 transceiver */
		if (gplr & GPIO_H3600_COM_DCD)
			ret &= ~TIOCM_CD;
		if (gplr & GPIO_H3600_COM_CTS)
			ret &= ~TIOCM_CTS;
	}

	return ret;
}

static void h3600_dcd_intr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_info *info = dev_id;
	/* Note: should only call this if something has changed */
        /* DCD and CTS bits are inverted in GPLR by RS232 transceiver */
	spin_lock_irq(&info->lock);
	uart_handle_dcd_change(info, !(GPLR & GPIO_H3600_COM_DCD));
	spin_unlock_irq(&info->lock);
}

static void h3600_cts_intr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_info *info = dev_id;
	/* Note: should only call this if something has changed */
        /* DCD and CTS bits are inverted in GPLR by RS232 transceiver */
	spin_lock_irq(&info->lock);
	uart_handle_cts_change(info, !(GPLR & GPIO_H3600_COM_CTS));
	spin_unlock_irq(&info->lock);
}

static void h3600_uart_pm(struct uart_port *port, u_int state, u_int oldstate)
{
	if (port->mapbase == _Ser2UTCR0) { /* TODO: REMOVE THIS */
		assign_h3600_egpio( IPAQ_EGPIO_IR_ON, !state );
	} else if (port->mapbase == _Ser3UTCR0) {
		assign_h3600_egpio( IPAQ_EGPIO_RS232_ON, !state );
	}
}

/*
 * Enable/Disable wake up events for this serial port.
 * Obviously, we only support this on the normal COM port.
 */
static int h3600_uart_set_wake(struct uart_port *port, int enable)
{
	int err = -EINVAL;

	if (port->mapbase == _Ser3UTCR0) {
		if (enable)
			PWER |= PWER_GPIO23 | PWER_GPIO25;
		else
			PWER &= ~(PWER_GPIO23 | PWER_GPIO25);
		err = 0;
	}
	return err;
}

static int h3600_uart_open(struct uart_port *port, struct uart_info *info)
{
	int ret = 0;

	if (port->mapbase == _Ser2UTCR0) {  /* TODO: REMOVE THIS */
		Ser2UTCR4 = UTCR4_HSE;
		Ser2HSCR0 = 0;
		Ser2HSSR0 = HSSR0_EIF | HSSR0_TUR |
			    HSSR0_RAB | HSSR0_FRE;
	} else if (port->mapbase == _Ser3UTCR0) {
		set_GPIO_IRQ_edge(GPIO_H3600_COM_DCD|GPIO_H3600_COM_CTS,
				  GPIO_BOTH_EDGES);

		ret = request_irq(IRQ_GPIO_H3600_COM_DCD, h3600_dcd_intr,
				  0, "RS232 DCD", info);
		if (ret)
			return ret;

		ret = request_irq(IRQ_GPIO_H3600_COM_CTS, h3600_cts_intr,
				  0, "RS232 CTS", info);
		if (ret)
			free_irq(IRQ_GPIO_H3600_COM_DCD, info);
	}
	return ret;
}

static void h3600_uart_close(struct uart_port *port, struct uart_info *info)
{
	if (port->mapbase == _Ser3UTCR0) {
		free_irq(IRQ_GPIO_H3600_COM_DCD, info);
		free_irq(IRQ_GPIO_H3600_COM_CTS, info);
	}
}

static struct sa1100_port_fns h3600_port_fns __initdata = {
	set_mctrl:	h3600_uart_set_mctrl,
	get_mctrl:	h3600_uart_get_mctrl,
	pm:		h3600_uart_pm,
	set_wake:	h3600_uart_set_wake,
	open:		h3600_uart_open,
	close:		h3600_uart_close,
};

static struct map_desc h3600_io_desc[] __initdata = {
 /* virtual            physical    length      domain     r  w  c  b */
  { 0xe8000000,        0x00000000, 0x02000000, DOMAIN_IO, 1, 1, 0, 0 }, /* Flash bank 0          CS#0 */
  { H3600_EGPIO_VIRT,  0x49000000, 0x01000000, DOMAIN_IO, 1, 1, 0, 0 }, /* EGPIO 0               CS#5 */
  { H3600_BANK_2_VIRT, 0x10000000, 0x02800000, DOMAIN_IO, 1, 1, 0, 0 }, /* static memory bank 2  CS#2 */
  { H3600_BANK_4_VIRT, 0x40000000, 0x00800000, DOMAIN_IO, 1, 1, 0, 0 }, /* static memory bank 4  CS#4 */
  LAST_DESC
};

/*
 * Common map_io initialization
 */

static void __init h3xxx_map_io(void)
{
	sa1100_map_io();
	iotable_init(h3600_io_desc);

	sa1100_register_uart_fns(&h3600_port_fns);
	sa1100_register_uart(0, 3); /* Common serial port */
//	sa1100_register_uart(1, 1); /* Microcontroller on 3100/3600 */

	/* Ensure those pins are outputs and driving low  */
 	PPDR |= PPC_TXD4 | PPC_SCLK | PPC_SFRM;
 	PPSR &= ~(PPC_TXD4 | PPC_SCLK | PPC_SFRM);

	/* Configure suspend conditions */
	PGSR = 0;
	PWER = PWER_GPIO0 | PWER_RTC;
	PCFR = PCFR_OPDE;
	PSDR = 0;

	sa1100fb_lcd_power = h3600_lcd_power;
}

static __inline__ void do_blank( int setp ) {
	if ( ipaq_model_ops.blank_callback )
		ipaq_model_ops.blank_callback(1-setp);
}

/************************* H3100 *************************/

#ifdef CONFIG_SA1100_H3100

#define H3100_EGPIO	(*(volatile unsigned int *)H3600_EGPIO_VIRT)
static unsigned int h3100_egpio = 0;

static void h3100_init_egpio( void )
{
	GPDR |= H3100_DIRECT_EGPIO;
	GPCR = H3100_DIRECT_EGPIO;   /* Initially all off */

	/* Older bootldrs put GPIO2-9 in alternate mode on the
	   assumption that they are used for video */
	GAFR &= ~H3100_DIRECT_EGPIO;

	h3100_egpio = EGPIO_H3600_RS232_ON;
	H3100_EGPIO = h3100_egpio;
}

static void h3100_control_egpio( enum ipaq_egpio_type x, int setp )
{
	unsigned int egpio = 0;
	long         gpio = 0;
	unsigned long flags;

	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		egpio |= EGPIO_H3600_LCD_ON;
		gpio  |= GPIO_H3100_LCD_3V_ON;
		do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
		egpio |= EGPIO_H3600_CODEC_NRESET;
		break;
	case IPAQ_EGPIO_AUDIO_ON:
		gpio |= GPIO_H3100_AUD_PWR_ON
			| GPIO_H3100_AUD_ON;
		break;
	case IPAQ_EGPIO_QMUTE:
		gpio |= GPIO_H3100_QMUTE;
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		egpio |= EGPIO_H3600_OPT_NVRAM_ON;
		break;
	case IPAQ_EGPIO_OPT_ON:
		egpio |= EGPIO_H3600_OPT_ON;
		break;
	case IPAQ_EGPIO_CARD_RESET:
		egpio |= EGPIO_H3600_CARD_RESET;
		break;
	case IPAQ_EGPIO_OPT_RESET:
		egpio |= EGPIO_H3600_OPT_RESET;
		break;
	case IPAQ_EGPIO_IR_ON:
		gpio |= GPIO_H3100_IR_ON;
		break;
	case IPAQ_EGPIO_IR_FSEL:
		gpio |= GPIO_H3100_IR_FSEL;
		break;
	case IPAQ_EGPIO_RS232_ON:
		egpio |= EGPIO_H3600_RS232_ON;
		break;
	case IPAQ_EGPIO_VPP_ON:
		egpio |= EGPIO_H3600_VPP_ON;
		break;
	}

	if ( egpio || gpio ) {
		local_irq_save(flags);
		if ( setp ) {
			h3100_egpio |= egpio;
			GPSR = gpio;
		} else {
			h3100_egpio &= ~egpio;
			GPCR = gpio;
		}
		H3100_EGPIO = h3100_egpio;
		local_irq_restore(flags);
	}
}

static unsigned long h3100_read_egpio( void )
{
	return h3100_egpio;
}

static int h3100_pm_callback( int req )
{
	if ( ipaq_model_ops.pm_callback_aux )
		return ipaq_model_ops.pm_callback_aux(req);
	return 0;
}

static struct ipaq_model_ops h3100_model_ops __initdata = {
	model	     : IPAQ_H3100,
	generic_name : "3100",
	initialize   : h3100_init_egpio,
	control      : h3100_control_egpio,
	pm_callback  : h3100_pm_callback,
	read	     : h3100_read_egpio
};

#define H3100_DIRECT_EGPIO (GPIO_H3100_BT_ON      \
                          | GPIO_H3100_GPIO3      \
                          | GPIO_H3100_QMUTE      \
                          | GPIO_H3100_LCD_3V_ON  \
	                  | GPIO_H3100_AUD_ON     \
		          | GPIO_H3100_AUD_PWR_ON \
			  | GPIO_H3100_IR_ON      \
			  | GPIO_H3100_IR_FSEL)

static void __init h3100_map_io(void)
{
	h3xxx_map_io();

	/* Initialize h3100-specific values here */
	GPCR = 0x0fffffff;       /* All outputs are set low by default */
	GPDR = GPIO_H3600_COM_RTS  | GPIO_H3600_L3_CLOCK |
	       GPIO_H3600_L3_MODE  | GPIO_H3600_L3_DATA  |
	       GPIO_H3600_CLK_SET1 | GPIO_H3600_CLK_SET0 |
	       H3100_DIRECT_EGPIO;

	/* Older bootldrs put GPIO2-9 in alternate mode on the 
	   assumption that they are used for video */
	GAFR &= ~H3100_DIRECT_EGPIO;

	H3100_EGPIO = h3100_egpio;
	ipaq_model_ops = h3100_model_ops;
}

MACHINE_START(H3100, "Compaq iPAQ H3100")
	BOOT_MEM(0xc0000000, 0x80000000, 0xf8000000)
	BOOT_PARAMS(0xc0000100)
	MAPIO(h3100_map_io)
	INITIRQ(sa1100_init_irq)
MACHINE_END

#endif /* CONFIG_SA1100_H3100 */

/************************* H3600 *************************/

#ifdef CONFIG_SA1100_H3600

#define H3600_EGPIO	(*(volatile unsigned int *)H3600_EGPIO_VIRT)
static unsigned int h3600_egpio = EGPIO_H3600_RS232_ON;

static void h3600_init_egpio( void )
{
	h3600_egpio = EGPIO_H3600_RS232_ON;
	H3600_EGPIO = h3600_egpio;
}

static void h3600_control_egpio( enum ipaq_egpio_type x, int setp )
{
	unsigned int egpio = 0;
	unsigned long flags;

	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		egpio |= EGPIO_H3600_LCD_ON |
			 EGPIO_H3600_LCD_PCI |
			 EGPIO_H3600_LCD_5V_ON |
			 EGPIO_H3600_LVDD_ON;
		do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
		egpio |= EGPIO_H3600_CODEC_NRESET;
		break;
	case IPAQ_EGPIO_AUDIO_ON:
		egpio |= EGPIO_H3600_AUD_AMP_ON |
			EGPIO_H3600_AUD_PWR_ON;
		break;
	case IPAQ_EGPIO_QMUTE:
		egpio |= EGPIO_H3600_QMUTE;
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		egpio |= EGPIO_H3600_OPT_NVRAM_ON;
		break;
	case IPAQ_EGPIO_OPT_ON:
		egpio |= EGPIO_H3600_OPT_ON;
		break;
	case IPAQ_EGPIO_CARD_RESET:
		egpio |= EGPIO_H3600_CARD_RESET;
		break;
	case IPAQ_EGPIO_OPT_RESET:
		egpio |= EGPIO_H3600_OPT_RESET;
		break;
	case IPAQ_EGPIO_IR_ON:
		egpio |= EGPIO_H3600_IR_ON;
		break;
	case IPAQ_EGPIO_IR_FSEL:
		egpio |= EGPIO_H3600_IR_FSEL;
		break;
	case IPAQ_EGPIO_RS232_ON:
		egpio |= EGPIO_H3600_RS232_ON;
		break;
	case IPAQ_EGPIO_VPP_ON:
		egpio |= EGPIO_H3600_VPP_ON;
		break;
	}

	if ( egpio ) {
		local_irq_save(flags);
		if ( setp )
			h3600_egpio |= egpio;
		else
			h3600_egpio &= ~egpio;
		H3600_EGPIO = h3600_egpio;
		local_irq_restore(flags);
	}
}

static unsigned long h3600_read_egpio( void )
{
	return h3600_egpio;
}

static int h3600_pm_callback( int req )
{
	if ( ipaq_model_ops.pm_callback_aux )
		return ipaq_model_ops.pm_callback_aux(req);
	return 0;
}

static struct ipaq_model_ops h3600_model_ops __initdata = {
	model	     : IPAQ_H3600,
	generic_name : "3600",
	initialize   : h3600_init_egpio,
	control      : h3600_control_egpio,
	pm_callback  : h3600_pm_callback,
	read	     : h3600_read_egpio
};

static void __init h3600_map_io(void)
{
	h3xxx_map_io();

	/* Initialize h3600-specific values here */

	GPCR = 0x0fffffff;       /* All outputs are set low by default */
	GPDR = GPIO_H3600_COM_RTS  | GPIO_H3600_L3_CLOCK |
	       GPIO_H3600_L3_MODE  | GPIO_H3600_L3_DATA  |
	       GPIO_H3600_CLK_SET1 | GPIO_H3600_CLK_SET0 |
	       GPIO_LDD15 | GPIO_LDD14 | GPIO_LDD13 | GPIO_LDD12 |
	       GPIO_LDD11 | GPIO_LDD10 | GPIO_LDD9  | GPIO_LDD8;

	H3600_EGPIO = h3600_egpio;         /* Maintains across sleep? */
	ipaq_model_ops = h3600_model_ops;
}

MACHINE_START(H3600, "Compaq iPAQ H3600")
	BOOT_MEM(0xc0000000, 0x80000000, 0xf8000000)
	BOOT_PARAMS(0xc0000100)
	MAPIO(h3600_map_io)
	INITIRQ(sa1100_init_irq)
MACHINE_END

#endif /* CONFIG_SA1100_H3600 */

/************************* H3800 *************************/

#ifdef CONFIG_SA1100_H3800

#define SET_ASIC1(x) \
   do {if ( setp ) { H3800_ASIC1_GPIO_Out |= (x); } else { H3800_ASIC1_GPIO_Out &= ~(x); }} while(0)

#define SET_ASIC2(x) \
   do {if ( setp ) { H3800_ASIC2_GPIOPIOD |= (x); } else { H3800_ASIC2_GPIOPIOD &= ~(x); }} while(0)

#define CLEAR_ASIC1(x) \
   do {if ( setp ) { H3800_ASIC1_GPIO_Out &= ~(x); } else { H3800_ASIC1_GPIO_Out |= (x); }} while(0)

#define CLEAR_ASIC2(x) \
   do {if ( setp ) { H3800_ASIC2_GPIOPIOD &= ~(x); } else { H3800_ASIC2_GPIOPIOD |= (x); }} while(0)


static void h3800_control_egpio( enum ipaq_egpio_type x, int setp )
{
	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		SET_ASIC1( GPIO_H3800_ASIC1_LCD_5V_ON 
			   | GPIO_H3800_ASIC1_LCD_ON 
			   | GPIO_H3800_ASIC1_LCD_PCI
			   | GPIO_H3800_ASIC1_VGH_ON 
			   | GPIO_H3800_ASIC1_VGL_ON );
		do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
		break;
	case IPAQ_EGPIO_AUDIO_ON:
		break;
	case IPAQ_EGPIO_QMUTE:
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		SET_ASIC2( GPIO2_OPT_ON_NVRAM );
		break;
	case IPAQ_EGPIO_OPT_ON:
		SET_ASIC2( GPIO2_OPT_ON );
		break;
	case IPAQ_EGPIO_CARD_RESET:
		SET_ASIC2( GPIO2_OPT_PCM_RESET );
		break;
	case IPAQ_EGPIO_OPT_RESET:
		SET_ASIC2( GPIO2_OPT_RESET );
		break;
	case IPAQ_EGPIO_IR_ON:
		CLEAR_ASIC1( GPIO_H3800_ASIC1_IR_ON_N );
		break;
	case IPAQ_EGPIO_IR_FSEL:
		break;
	case IPAQ_EGPIO_RS232_ON:
		SET_ASIC1( GPIO_H3800_ASIC1_RS232_ON );
		break;
	case IPAQ_EGPIO_VPP_ON:
		H3800_ASIC2_FlashWP_VPP_ON = setp;
		break;
	}
}

static unsigned long h3800_read_egpio( void )
{
	return H3800_ASIC1_GPIO_Out | (H3800_ASIC2_GPIOPIOD << 16);
}

/* We need to fix ASIC2 GPIO over suspend/resume.  At the moment,
   it doesn't appear that ASIC1 GPIO has the same problem */

static int h3800_pm_callback( int req )
{
	static u16 asic2_data;
	int result = 0;

	printk(__FUNCTION__ " %d\n", req);
	printk("*** ASIC1 GPIO Direction = %x\n", H3800_ASIC1_GPIO_Direction );
	printk("*** ASIC1 GPIO Data      = %x\n", H3800_ASIC1_GPIO_Out );
	printk("*** ASIC2 GPIO Direction = %x\n", H3800_ASIC2_GPIODIR );
	printk("*** ASIC2 GPIO Data      = %x\n", H3800_ASIC2_GPIOPIOD );

	switch (req) {
	case PM_RESUME:
		H3800_ASIC2_GPIOPIOD = asic2_data;
		H3800_ASIC2_GPIODIR = GPIO2_PEN_IRQ 
			| GPIO2_SD_DETECT
			| GPIO2_EAR_IN_N 
			| GPIO2_USB_DETECT_N 
			| GPIO2_SD_CON_SLT;

		if ( ipaq_model_ops.pm_callback_aux )
			result = ipaq_model_ops.pm_callback_aux(req);
		break;

	case PM_SUSPEND:
		if ( ipaq_model_ops.pm_callback_aux &&
		     ((result = ipaq_model_ops.pm_callback_aux(req)) != 0))
			return result;

		asic2_data = H3800_ASIC2_GPIOPIOD;
		break;
	default:
		printk(__FUNCTION__ ": unrecognized PM callback\n");
		break;
	}
	return result;
}

static struct ipaq_model_ops h3800_model_ops __initdata = {
	model	     : IPAQ_H3800,
	generic_name : "3800",
	initialize   : h3800_init_egpio,
	control      : h3800_control_egpio,
	pm_callback  : h3800_pm_callback,
	read	     : h3800_read_egpio
};

#define ASIC1_OUTPUTS    0x7fff   /* First 15 bits are used */

static void __init h3800_map_io(void)
{
	h3xxx_map_io();

	/* Initialize h3800-specific values here */
	GPCR = 0x0fffffff;       /* All outputs are set low by default */
	GAFR =  GPIO_H3800_CLK_OUT | 
		GPIO_LDD15 | GPIO_LDD14 | GPIO_LDD13 | GPIO_LDD12 |
		GPIO_LDD11 | GPIO_LDD10 | GPIO_LDD9  | GPIO_LDD8;
	GPDR =  GPIO_H3800_CLK_OUT | 
		GPIO_H3600_COM_RTS  | GPIO_H3600_L3_CLOCK |
		GPIO_H3600_L3_MODE  | GPIO_H3600_L3_DATA  |
		GPIO_LDD15 | GPIO_LDD14 | GPIO_LDD13 | GPIO_LDD12 |
		GPIO_LDD11 | GPIO_LDD10 | GPIO_LDD9  | GPIO_LDD8;
	TUCR =  TUCR_3_6864MHz;   /* Seems to be used only for the Bluetooth UART */

	/* Set up ASIC #1 */
	H3800_ASIC1_GPIO_Direction    = ASIC1_OUTPUTS;            /* All outputs */
	H3800_ASIC1_GPIO_Mask         = ASIC1_OUTPUTS;            /* No interrupts */
	H3800_ASIC1_GPIO_SleepMask    = ASIC1_OUTPUTS;
	H3800_ASIC1_GPIO_SleepDir     = ASIC1_OUTPUTS;
	H3800_ASIC1_GPIO_SleepOut     = GPIO_H3800_ASIC1_EAR_ON_N;
	H3800_ASIC1_GPIO_BattFaultDir = ASIC1_OUTPUTS;
	H3800_ASIC1_GPIO_BattFaultOut = GPIO_H3800_ASIC1_EAR_ON_N;

	H3800_ASIC1_GPIO_Out = GPIO_H3800_ASIC1_IR_ON_N   /* TODO: Check IR level */
		| GPIO_H3800_ASIC1_RS232_ON
		| GPIO_H3800_ASIC1_EAR_ON_N;
	
	/* Set up ASIC #2 */
	H3800_ASIC2_GPIOPIOD    = GPIO2_IN_Y1_N | GPIO2_IN_X1_N;
	H3800_ASIC2_GPOBFSTAT   = GPIO2_IN_Y1_N | GPIO2_IN_X1_N;

	H3800_ASIC2_GPIODIR     = GPIO2_PEN_IRQ 
                                      | GPIO2_SD_DETECT
		                      | GPIO2_EAR_IN_N 
		                      | GPIO2_USB_DETECT_N 
		                      | GPIO2_SD_CON_SLT;

	/* TODO : Set sleep states & battery fault states */

	/* Clear VPP Enable */
	H3800_ASIC2_FlashWP_VPP_ON = 0;
	ipaq_model_ops = h3800_model_ops;
}

MACHINE_START(H3800, "Compaq iPAQ H3800")
	BOOT_MEM(0xc0000000, 0x80000000, 0xf8000000)
	BOOT_PARAMS(0xc0000100)
	MAPIO(h3800_map_io)
	INITIRQ(sa1100_init_irq)
MACHINE_END

#endif /* CONFIG_SA1100_H3800 */
