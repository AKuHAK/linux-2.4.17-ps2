/*
 * linux/drivers/ide/ide-pmac.c
 *
 * Support for IDE interfaces on PowerMacs.
 * These IDE interfaces are memory-mapped and have a DBDMA channel
 * for doing DMA.
 *
 *  Copyright (C) 1998-2001 Paul Mackerras & Ben. Herrenschmidt
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * Some code taken from drivers/ide/ide-dma.c:
 *
 *  Copyright (c) 1995-1998  Mark Lord
 *
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <asm/prom.h>
#include <asm/io.h>
#include <asm/dbdma.h>
#include <asm/ide.h>
#include <asm/mediabay.h>
#include <asm/machdep.h>
#include <asm/pmac_feature.h>
#include <asm/sections.h>
#ifdef CONFIG_PMAC_PBOOK
#include <linux/adb.h>
#include <linux/pmu.h>
#include <asm/irq.h>
#endif
#include "ide_modes.h"

extern char *ide_dmafunc_verbose(ide_dma_action_t dmafunc);
extern void ide_do_request(ide_hwgroup_t *hwgroup, int masked_irq);

#define IDE_PMAC_DEBUG

#define DMA_WAIT_TIMEOUT	500

struct pmac_ide_hwif {
	ide_ioreg_t			regbase;
	int				irq;
	int				kind;
	int				aapl_bus_id;
	struct device_node*		node;
	u32				timings[2];
	struct resource*		reg_resource;
#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC
	volatile struct dbdma_regs*	dma_regs;
	struct dbdma_cmd*		dma_table;
	struct resource*		dma_resource;
#endif
	
} pmac_ide[MAX_HWIFS] __pmacdata;

static int pmac_ide_count;

enum {
	controller_ohare,	/* OHare based */
	controller_heathrow,	/* Heathrow/Paddington */
	controller_kl_ata3,	/* KeyLargo ATA-3 */
	controller_kl_ata4,	/* KeyLargo ATA-4 */
	controller_kl_ata4_80	/* KeyLargo ATA-4 with 80 conductor cable */
};

/*
 * Timing register definitions
 */

/* Number of IDE_SYSCLK_NS ticks, argument is in nanoseconds */
#define SYSCLK_TICKS(t)		(((t) + IDE_SYSCLK_NS - 1) / IDE_SYSCLK_NS)
#define SYSCLK_TICKS_66(t)	(((t) + IDE_SYSCLK_66_NS - 1) / IDE_SYSCLK_66_NS)
#define IDE_SYSCLK_NS		30	/* 33Mhz cell */
#define IDE_SYSCLK_66_NS	15	/* 66Mhz cell */

/* 66Mhz cell, found in KeyLargo. Can do ultra mode 0 to 2 on
 * 40 connector cable and to 4 on 80 connector one.
 * Clock unit is 15ns (66Mhz)
 * 
 * 3 Values can be programmed:
 *  - Write data setup, which appears to match the cycle time. They
 *    also call it DIOW setup.
 *  - Ready to pause time (from spec)
 *  - Address setup. That one is weird. I don't see where exactly
 *    it fits in UDMA cycles, I got it's name from an obscure piece
 *    of commented out code in Darwin. They leave it to 0, we do as
 *    well, despite a comment that would lead to think it has a
 *    min value of 45ns.
 * Apple also add 60ns to the write data setup (or cycle time ?) on
 * reads. I can't explain that, I tried it and it broke everything
 * here.
 */
#define TR_66_UDMA_MASK			0xfff00000
#define TR_66_UDMA_EN			0x00100000 /* Enable Ultra mode for DMA */
#define TR_66_UDMA_ADDRSETUP_MASK	0xe0000000 /* Address setup */
#define TR_66_UDMA_ADDRSETUP_SHIFT	29
#define TR_66_UDMA_RDY2PAUS_MASK	0x1e000000 /* Ready 2 pause time */
#define TR_66_UDMA_RDY2PAUS_SHIFT	25
#define TR_66_UDMA_WRDATASETUP_MASK	0x01e00000 /* Write data setup time */
#define TR_66_UDMA_WRDATASETUP_SHIFT	21
#define TR_66_MDMA_MASK			0x000ffc00
#define TR_66_MDMA_RECOVERY_MASK	0x000f8000
#define TR_66_MDMA_RECOVERY_SHIFT	15
#define TR_66_MDMA_ACCESS_MASK		0x00007c00
#define TR_66_MDMA_ACCESS_SHIFT		10
#define TR_66_PIO_MASK			0x000003ff
#define TR_66_PIO_RECOVERY_MASK		0x000003e0
#define TR_66_PIO_RECOVERY_SHIFT	5
#define TR_66_PIO_ACCESS_MASK		0x0000001f
#define TR_66_PIO_ACCESS_SHIFT		0

/* 33Mhz cell, found in OHare, Heathrow (& Paddington) and KeyLargo
 * Can do pio & mdma modes, clock unit is 30ns (33Mhz)
 * 
 * The access time and recovery time can be programmed. Some older
 * Darwin code base limit OHare to 150ns cycle time. I decided to do
 * the same here fore safety against broken old hardware ;)
 * The HalfTick bit, when set, adds half a clock (15ns) to the access
 * time and removes one from recovery. It's not supported on KeyLargo
 * implementation afaik. The E bit appears to be set for PIO mode 0 and
 * is used to reach long timings used in this mode.
 */
#define TR_33_MDMA_MASK			0x003ff800
#define TR_33_MDMA_RECOVERY_MASK	0x001f0000
#define TR_33_MDMA_RECOVERY_SHIFT	16
#define TR_33_MDMA_ACCESS_MASK		0x0000f800
#define TR_33_MDMA_ACCESS_SHIFT		11
#define TR_33_MDMA_HALFTICK		0x00200000
#define TR_33_PIO_MASK			0x000007ff
#define TR_33_PIO_E			0x00000400
#define TR_33_PIO_RECOVERY_MASK		0x000003e0
#define TR_33_PIO_RECOVERY_SHIFT	5
#define TR_33_PIO_ACCESS_MASK		0x0000001f
#define TR_33_PIO_ACCESS_SHIFT		0

#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC

/* Rounded Multiword DMA timings
 * 
 * I gave up finding a generic formula for all controller
 * types and instead, built tables based on timing values
 * used by Apple in Darwin's implementation.
 */
struct mdma_timings_t {
	int	accessTime;
	int	recoveryTime;
	int	cycleTime;
};

struct mdma_timings_t mdma_timings_33[] __pmacdata =
{
    { 240, 240, 480 },
    { 180, 180, 360 },
    { 135, 135, 270 },
    { 120, 120, 240 },
    { 105, 105, 210 },
    {  90,  90, 180 },
    {  75,  75, 150 },
    {  75,  45, 120 },
    {   0,   0,   0 }
};

struct mdma_timings_t mdma_timings_33k[] __pmacdata =
{
    { 240, 240, 480 },
    { 180, 180, 360 },
    { 150, 150, 300 },
    { 120, 120, 240 },
    {  90, 120, 210 },
    {  90,  90, 180 },
    {  90,  60, 150 },
    {  90,  30, 120 },
    {   0,   0,   0 }
};

struct mdma_timings_t mdma_timings_66[] __pmacdata =
{
    { 240, 240, 480 },
    { 180, 180, 360 },
    { 135, 135, 270 },
    { 120, 120, 240 },
    { 105, 105, 210 },
    {  90,  90, 180 },
    {  90,  75, 165 },
    {  75,  45, 120 },
    {   0,   0,   0 }
};

/* Ultra DMA timings (rounded) */
struct {
	int	addrSetup; /* ??? */
	int	rdy2pause;
	int	wrDataSetup;
} udma_timings[] __pmacdata =
{
    {   0, 180,  120 },	/* Mode 0 */
    {   0, 150,  90 },	/*      1 */
    {   0, 120,  60 },	/*      2 */
    {   0, 90,   45 },	/*      3 */
    {   0, 90,   30 }	/*      4 */
};

/* allow up to 256 DBDMA commands per xfer */
#define MAX_DCMDS		256

/* Wait 2s for disk to answer on IDE bus after
 * enable operation.
 * NOTE: There is at least one case I know of a disk that needs about 10sec
 *       before anwering on the bus. I beleive we could add a kernel command
 *       line arg to override this delay for such cases.
 */
#define IDE_WAKEUP_DELAY_MS	2000

static void pmac_ide_setup_dma(struct device_node *np, int ix);
static int pmac_ide_dmaproc(ide_dma_action_t func, ide_drive_t *drive);
static int pmac_ide_build_dmatable(ide_drive_t *drive, int ix, int wr);
static int pmac_ide_tune_chipset(ide_drive_t *drive, byte speed);
static void pmac_ide_tuneproc(ide_drive_t *drive, byte pio);
static void pmac_ide_selectproc(ide_drive_t *drive);

#endif /* CONFIG_BLK_DEV_IDEDMA_PMAC */

#ifdef CONFIG_PMAC_PBOOK
static int idepmac_notify_sleep(struct pmu_sleep_notifier *self, int when);
struct pmu_sleep_notifier idepmac_sleep_notifier = {
	idepmac_notify_sleep, SLEEP_LEVEL_BLOCK,
};
#endif /* CONFIG_PMAC_PBOOK */

static int pmac_ide_notify_reboot(struct notifier_block *, unsigned long, void *);
static struct notifier_block pmac_ide_reboot_notifier = {
	pmac_ide_notify_reboot,
	NULL,
	0
};

static int __pmac
pmac_ide_find(ide_drive_t *drive)
{
	ide_hwif_t *hwif = HWIF(drive);
	ide_ioreg_t base;
	int i;
	
	for (i=0; i<pmac_ide_count; i++) {
		base = pmac_ide[i].regbase;
		if (base && base == hwif->io_ports[0])
			return i;
	}
	return -1;
}

/*
 * N.B. this can't be an initfunc, because the media-bay task can
 * call ide_[un]register at any time.
 */
void __pmac
pmac_ide_init_hwif_ports(hw_regs_t *hw,
			      ide_ioreg_t data_port, ide_ioreg_t ctrl_port,
			      int *irq)
{
	int i, ix;

	if (data_port == 0)
		return;

	for (ix = 0; ix < MAX_HWIFS; ++ix)
		if (data_port == pmac_ide[ix].regbase)
			break;

	if (ix >= MAX_HWIFS) {
		/* Probably a PCI interface... */
		for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; ++i)
			hw->io_ports[i] = data_port + i - IDE_DATA_OFFSET;
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
		return;
	}

	for (i = 0; i < 8; ++i)
		hw->io_ports[i] = data_port + i * 0x10;
	hw->io_ports[8] = data_port + 0x160;

	if (irq != NULL)
		*irq = pmac_ide[ix].irq;

	ide_hwifs[ix].tuneproc = pmac_ide_tuneproc;
	ide_hwifs[ix].selectproc = pmac_ide_selectproc;
	ide_hwifs[ix].speedproc = &pmac_ide_tune_chipset;
	if (pmac_ide[ix].dma_regs && pmac_ide[ix].dma_table) {
		ide_hwifs[ix].dmaproc = &pmac_ide_dmaproc;
#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC_AUTO
		if (!noautodma)
			ide_hwifs[ix].autodma = 1;
#endif
	}
}

#if 0
/* This one could be later extended to handle CMD IDE and be used by some kind
 * of /proc interface. I want to be able to get the devicetree path of a block
 * device for yaboot configuration
 */
struct device_node*
pmac_ide_get_devnode(ide_drive_t *drive)
{
	int i = pmac_ide_find(drive);
	if (i < 0)
		return NULL;
	return pmac_ide[i].node;
}
#endif

/* Setup timings for the selected drive (master/slave). I still need to verify if this
 * is enough, I beleive selectproc will be called whenever an IDE command is started,
 * but... */
static void __pmac
pmac_ide_selectproc(ide_drive_t *drive)
{
	int i = pmac_ide_find(drive);
	if (i < 0)
		return;

	if (drive->select.b.unit & 0x01)
		out_le32((unsigned *)(IDE_DATA_REG + 0x200 + _IO_BASE), pmac_ide[i].timings[1]);
	else
		out_le32((unsigned *)(IDE_DATA_REG + 0x200 + _IO_BASE), pmac_ide[i].timings[0]);
	(void)in_le32((unsigned *)(IDE_DATA_REG + 0x200 + _IO_BASE));
}


/* Note: We don't use the generic routine here because for some
 * yet unexplained reasons, it cause some media-bay CD-ROMs to
 * lockup the bus. Strangely, this new version of the code is
 * almost identical to the generic one and works, I've not yet
 * managed to figure out what bit is causing the lockup in the
 * generic code, possibly a timing issue...
 * 
 * --BenH
 */
#undef USE_GENERIC_SETFEATURE

#ifndef USE_GENERIC_SETFEATURE
static int __pmac
wait_for_ready(ide_drive_t *drive)
{
	/* Timeout bumped for some powerbooks */
	int timeout = 2000;
	byte stat;

	while(--timeout) {
		stat = GET_STAT();
		if(!(stat & BUSY_STAT)) {
			if (drive->ready_stat == 0)
				break;
			else if((stat & drive->ready_stat) || (stat & ERR_STAT))
				break;
		}
		mdelay(1);
	}
	if((stat & ERR_STAT) || timeout <= 0) {
		if (stat & ERR_STAT) {
			printk(KERN_ERR "ide_pmac: wait_for_ready, error status: %x\n", stat);
		}
		return 1;
	}
	return 0;
}
#endif /* USE_GENERIC_SETFEATURE */

static int __pmac
pmac_ide_do_setfeature(ide_drive_t *drive, byte command)
{
#ifdef USE_GENERIC_SETFEATURE
	return ide_config_drive_speed(drive, command);
#else
	int result = 1;
	unsigned long flags;
	ide_hwif_t *hwif = HWIF(drive);
	
	disable_irq(hwif->irq);	/* disable_irq_nosync ?? */
	udelay(1);
	SELECT_DRIVE(HWIF(drive), drive);
	SELECT_MASK(HWIF(drive), drive, 0);
	udelay(1);
	if(wait_for_ready(drive)) {
		printk(KERN_ERR "pmac_ide_do_setfeature disk not ready before SET_FEATURE!\n");
		goto out;
	}
	udelay(10);
	OUT_BYTE(drive->ctl | 2, IDE_CONTROL_REG);
	OUT_BYTE(command, IDE_NSECTOR_REG);
	OUT_BYTE(SETFEATURES_XFER, IDE_FEATURE_REG);
	OUT_BYTE(WIN_SETFEATURES, IDE_COMMAND_REG);
	udelay(1);
	__save_flags(flags);	/* local CPU only */
	ide__sti();		/* local CPU only -- for jiffies */
	result = wait_for_ready(drive);
	__restore_flags(flags); /* local CPU only */
	OUT_BYTE(drive->ctl, IDE_CONTROL_REG);
	if (result)
		printk(KERN_ERR "pmac_ide_do_setfeature disk not ready after SET_FEATURE !\n");
out:
	SELECT_MASK(HWIF(drive), drive, 0);
	enable_irq(hwif->irq);
	return result;
#endif	
}

/* Calculate PIO timings */
static void __pmac
pmac_ide_tuneproc(ide_drive_t *drive, byte pio)
{
	ide_pio_data_t d;
	int i;
	u32 *timings;
	unsigned accessTicks, recTicks;
	unsigned accessTime, recTime;
	
	i = pmac_ide_find(drive);
	if (i < 0)
		return;
		
	pio = ide_get_best_pio_mode(drive, pio, 4, &d);
	accessTicks = SYSCLK_TICKS(ide_pio_timings[pio].active_time);
	if (drive->select.b.unit & 0x01)
		timings = &pmac_ide[i].timings[1];
	else
		timings = &pmac_ide[i].timings[0];

	recTime = d.cycle_time - ide_pio_timings[pio].active_time
			- ide_pio_timings[pio].setup_time;
	recTime = max(recTime, 150U);
	accessTime = ide_pio_timings[pio].active_time;
	accessTime = max(accessTime, 150U);
	if (pmac_ide[i].kind == controller_kl_ata4 ||
		pmac_ide[i].kind == controller_kl_ata4_80) {
		/* 66Mhz cell */
		accessTicks = SYSCLK_TICKS_66(accessTime);
		accessTicks = min(accessTicks, 0x1fU);
		recTicks = SYSCLK_TICKS_66(recTime);
		recTicks = min(recTicks, 0x1fU);
		*timings = ((*timings) & ~TR_66_PIO_MASK) |
				(accessTicks << TR_66_PIO_ACCESS_SHIFT) |
				(recTicks << TR_66_PIO_RECOVERY_SHIFT);
	} else {
		/* 33Mhz cell */
		int ebit = 0;
		accessTicks = SYSCLK_TICKS(accessTime);
		accessTicks = min(accessTicks, 0x1fU);
		accessTicks = max(accessTicks, 4U);
		recTicks = SYSCLK_TICKS(recTime);
		recTicks = min(recTicks, 0x1fU);
		recTicks = max(recTicks, 5U) - 4;
		if (recTicks > 9) {
			recTicks--; /* guess, but it's only for PIO0, so... */
			ebit = 1;
		}
		*timings = ((*timings) & ~TR_33_PIO_MASK) |
				(accessTicks << TR_33_PIO_ACCESS_SHIFT) |
				(recTicks << TR_33_PIO_RECOVERY_SHIFT);
		if (ebit)
			*timings |= TR_33_PIO_E;
	}

#ifdef IDE_PMAC_DEBUG
	printk(KERN_ERR "ide_pmac: Set PIO timing for mode %d, reg: 0x%08x\n",
		pio,  *timings);
#endif	
		
	if (drive->select.all == IN_BYTE(IDE_SELECT_REG))
		pmac_ide_selectproc(drive);
}

#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC
static int __pmac
set_timings_udma(u32 *timings, byte speed)
{
	unsigned rdyToPauseTicks, wrDataSetupTicks, addrTicks;

	rdyToPauseTicks = SYSCLK_TICKS_66(udma_timings[speed & 0xf].rdy2pause);
	wrDataSetupTicks = SYSCLK_TICKS_66(udma_timings[speed & 0xf].wrDataSetup);
	addrTicks = SYSCLK_TICKS_66(udma_timings[speed & 0xf].addrSetup);

	*timings = ((*timings) & ~(TR_66_UDMA_MASK | TR_66_MDMA_MASK)) |
			(wrDataSetupTicks << TR_66_UDMA_WRDATASETUP_SHIFT) | 
			(rdyToPauseTicks << TR_66_UDMA_RDY2PAUS_SHIFT) |
			(addrTicks <<TR_66_UDMA_ADDRSETUP_SHIFT) |
			TR_66_UDMA_EN;
#ifdef IDE_PMAC_DEBUG
	printk(KERN_ERR "ide_pmac: Set UDMA timing for mode %d, reg: 0x%08x\n",
		speed & 0xf,  *timings);
#endif	

	return 0;
}

static int __pmac
set_timings_mdma(int intf_type, u32 *timings, byte speed, int drive_cycle_time)
{
	int cycleTime, accessTime, recTime;
	unsigned accessTicks, recTicks;
	struct mdma_timings_t* tm;
	int i;

	/* Get default cycle time for mode */
	switch(speed & 0xf) {
		case 0: cycleTime = 480; break;
		case 1: cycleTime = 150; break;
		case 2: cycleTime = 120; break;
		default:
			return -1;
	}
	/* Adjust for drive */
	if (drive_cycle_time && drive_cycle_time > cycleTime)
		cycleTime = drive_cycle_time;
	/* OHare limits according to some old Apple sources */	
	if ((intf_type == controller_ohare) && (cycleTime < 150))
		cycleTime = 150;
	/* Get the proper timing array for this controller */
	switch(intf_type) {
		case controller_kl_ata4:
		case controller_kl_ata4_80:
			tm = mdma_timings_66;
			break;
		case controller_kl_ata3:
			tm = mdma_timings_33k;
			break;
		default:
			tm = mdma_timings_33;
			break;
	}
	/* Lookup matching access & recovery times */
	i = -1;
	for (;;) {
		if (tm[i+1].cycleTime < cycleTime)
			break;
		i++;
	}
	if (i < 0)
		return -1;
	cycleTime = tm[i].cycleTime;
	accessTime = tm[i].accessTime;
	recTime = tm[i].recoveryTime;

#ifdef IDE_PMAC_DEBUG
	printk(KERN_ERR "ide_pmac: MDMA, cycleTime: %d, accessTime: %d, recTime: %d\n",
		cycleTime, accessTime, recTime);
#endif	
	if (intf_type == controller_kl_ata4 || intf_type == controller_kl_ata4_80) {
		/* 66Mhz cell */
		accessTicks = SYSCLK_TICKS_66(accessTime);
		accessTicks = min(accessTicks, 0x1fU);
		accessTicks = max(accessTicks, 0x1U);
		recTicks = SYSCLK_TICKS_66(recTime);
		recTicks = min(recTicks, 0x1fU);
		recTicks = max(recTicks, 0x3U);
		/* Clear out mdma bits and disable udma */
		*timings = ((*timings) & ~(TR_66_MDMA_MASK | TR_66_UDMA_MASK)) |
			(accessTicks << TR_66_MDMA_ACCESS_SHIFT) |
			(recTicks << TR_66_MDMA_RECOVERY_SHIFT);
	} else if (intf_type == controller_kl_ata3) {
		/* 33Mhz cell on KeyLargo */
		accessTicks = SYSCLK_TICKS(accessTime);
		accessTicks = max(accessTicks, 1U);
		accessTicks = min(accessTicks, 0x1fU);
		accessTime = accessTicks * IDE_SYSCLK_NS;
		recTicks = SYSCLK_TICKS(recTime);
		recTicks = max(recTicks, 1U);
		recTicks = min(recTicks, 0x1fU);
		*timings = ((*timings) & ~TR_33_MDMA_MASK) |
				(accessTicks << TR_33_MDMA_ACCESS_SHIFT) |
				(recTicks << TR_33_MDMA_RECOVERY_SHIFT);
	} else {
		/* 33Mhz cell on others */
		int halfTick = 0;
		int origAccessTime = accessTime;
		int origRecTime = recTime;
		
		accessTicks = SYSCLK_TICKS(accessTime);
		accessTicks = max(accessTicks, 1U);
		accessTicks = min(accessTicks, 0x1fU);
		accessTime = accessTicks * IDE_SYSCLK_NS;
		recTicks = SYSCLK_TICKS(recTime);
		recTicks = max(recTicks, 2U) - 1;
		recTicks = min(recTicks, 0x1fU);
		recTime = (recTicks + 1) * IDE_SYSCLK_NS;
		if ((accessTicks > 1) &&
		    ((accessTime - IDE_SYSCLK_NS/2) >= origAccessTime) &&
		    ((recTime - IDE_SYSCLK_NS/2) >= origRecTime)) {
            		halfTick = 1;
			accessTicks--;
		}
		*timings = ((*timings) & ~TR_33_MDMA_MASK) |
				(accessTicks << TR_33_MDMA_ACCESS_SHIFT) |
				(recTicks << TR_33_MDMA_RECOVERY_SHIFT);
		if (halfTick)
			*timings |= TR_33_MDMA_HALFTICK;
	}
#ifdef IDE_PMAC_DEBUG
	printk(KERN_ERR "ide_pmac: Set MDMA timing for mode %d, reg: 0x%08x\n",
		speed & 0xf,  *timings);
#endif	
	return 0;
}
#endif /* #ifdef CONFIG_BLK_DEV_IDEDMA_PMAC */

/* You may notice we don't use this function on normal operation,
 * our, normal mdma function is supposed to be more precise
 */
static int __pmac
pmac_ide_tune_chipset (ide_drive_t *drive, byte speed)
{
	int intf		= pmac_ide_find(drive);
	int unit		= (drive->select.b.unit & 0x01);
	int ret			= 0;
	u32 *timings;

	if (intf < 0)
		return 1;
		
	timings = &pmac_ide[intf].timings[unit];
	
	switch(speed) {
#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC
		case XFER_UDMA_4:
		case XFER_UDMA_3:
			if (pmac_ide[intf].kind != controller_kl_ata4_80)
				return 1;		
		case XFER_UDMA_2:
		case XFER_UDMA_1:
		case XFER_UDMA_0:
			if (pmac_ide[intf].kind != controller_kl_ata4 &&
				pmac_ide[intf].kind != controller_kl_ata4_80)
				return 1;		
			ret = set_timings_udma(timings, speed);
			break;
		case XFER_MW_DMA_2:
		case XFER_MW_DMA_1:
		case XFER_MW_DMA_0:
			ret = set_timings_mdma(pmac_ide[intf].kind, timings, speed, 0);
			break;
		case XFER_SW_DMA_2:
		case XFER_SW_DMA_1:
		case XFER_SW_DMA_0:
			return 1;
#endif /* CONFIG_BLK_DEV_IDEDMA_PMAC */
		case XFER_PIO_4:
		case XFER_PIO_3:
		case XFER_PIO_2:
		case XFER_PIO_1:
		case XFER_PIO_0:
			pmac_ide_tuneproc(drive, speed & 0x07);
			break;
		default:
			ret = 1;
	}
	if (ret)
		return ret;

	ret = pmac_ide_do_setfeature(drive, speed);
	if (ret)
		return ret;
		
	pmac_ide_selectproc(drive);	
	drive->current_speed = speed;

	return 0;
}

static void __pmac
sanitize_timings(int i)
{
	unsigned value;
	
	switch(pmac_ide[i].kind) {
		case controller_kl_ata4:
		case controller_kl_ata4_80:
			value = 0x0008438c;
			break;
		case controller_kl_ata3:
			value = 0x00084526;
			break;
		case controller_heathrow:
		case controller_ohare:
		default:
			value = 0x00074526;
			break;
	}
	pmac_ide[i].timings[0] = pmac_ide[i].timings[1] = value;
}

ide_ioreg_t __pmac
pmac_ide_get_base(int index)
{
	return pmac_ide[index].regbase;
}

int __pmac
pmac_ide_check_base(ide_ioreg_t base)
{
	int ix;
	
 	for (ix = 0; ix < MAX_HWIFS; ++ix)
		if (base == pmac_ide[ix].regbase)
			return ix;
	return -1;
}

int __pmac
pmac_ide_get_irq(ide_ioreg_t base)
{
	int ix;

	for (ix = 0; ix < MAX_HWIFS; ++ix)
		if (base == pmac_ide[ix].regbase)
			return pmac_ide[ix].irq;
	return 0;
}

static int ide_majors[]  __pmacdata = { 3, 22, 33, 34, 56, 57 };

kdev_t __init
pmac_find_ide_boot(char *bootdevice, int n)
{
	int i;
	
	/*
	 * Look through the list of IDE interfaces for this one.
	 */
	for (i = 0; i < pmac_ide_count; ++i) {
		char *name;
		if (!pmac_ide[i].node || !pmac_ide[i].node->full_name)
			continue;
		name = pmac_ide[i].node->full_name;
		if (memcmp(name, bootdevice, n) == 0 && name[n] == 0) {
			/* XXX should cope with the 2nd drive as well... */
			return MKDEV(ide_majors[i], 0);
		}
	}

	return 0;
}

void __init
pmac_ide_probe(void)
{
	struct device_node *np;
	int i;
	struct device_node *atas;
	struct device_node *p, **pp, *removables, **rp;
	unsigned long base;
	int irq, big_delay;
	ide_hwif_t *hwif;

	if (_machine != _MACH_Pmac)
		return;
	pp = &atas;
	rp = &removables;
	p = find_devices("ATA");
	if (p == NULL)
		p = find_devices("IDE");
	if (p == NULL)
		p = find_type_devices("ide");
	if (p == NULL)
		p = find_type_devices("ata");
	/* Move removable devices such as the media-bay CDROM
	   on the PB3400 to the end of the list. */
	for (; p != NULL; p = p->next) {
		if (p->parent && p->parent->type
		    && strcasecmp(p->parent->type, "media-bay") == 0) {
			*rp = p;
			rp = &p->next;
		} else {
			*pp = p;
			pp = &p->next;
		}
	}
	*rp = NULL;
	*pp = removables;
	big_delay = 0;

	for (i = 0, np = atas; i < MAX_HWIFS && np != NULL; np = np->next) {
		struct device_node *tp;
		struct pmac_ide_hwif* pmhw;
		int *bidp;
		int in_bay = 0;

		/*
		 * If this node is not under a mac-io or dbdma node,
		 * leave it to the generic PCI driver.
		 */
		for (tp = np->parent; tp != 0; tp = tp->parent)
			if (tp->type && (strcmp(tp->type, "mac-io") == 0
					 || strcmp(tp->type, "dbdma") == 0))
				break;
		if (tp == 0)
			continue;

		if (np->n_addrs == 0) {
			printk(KERN_WARNING "ide: no address for device %s\n",
			       np->full_name);
			continue;
		}

		/*
		 * If this slot is taken (e.g. by ide-pci.c) try the next one.
		 */
		while (i < MAX_HWIFS
		       && ide_hwifs[i].io_ports[IDE_DATA_OFFSET] != 0)
			++i;
		if (i >= MAX_HWIFS)
			break;
		pmhw = &pmac_ide[i];

		/*
		 * Some older OFs have bogus sizes, causing request_OF_resource
		 * to fail. We fix them up here
		 */
		if (np->addrs[0].size > 0x1000)
			np->addrs[0].size = 0x1000;
		if (np->n_addrs > 1 && np->addrs[1].size > 0x100)
			np->addrs[1].size = 0x100;

		pmhw->reg_resource = request_OF_resource(np, 0, "  (mac-io IDE IO)");
		if (!pmhw->reg_resource) {
			printk(KERN_ERR "ide-pmac(%s): can't request IO resource !\n", np->name);
			continue;
		}

		base = (unsigned long) ioremap(np->addrs[0].address, 0x200) - _IO_BASE;

		/* XXX This is bogus. Should be fixed in the registry by checking
		   the kind of host interrupt controller, a bit like gatwick
		   fixes in irq.c
		 */
		if (np->n_intrs == 0) {
			printk(KERN_WARNING "ide: no intrs for device %s, using 13\n",
			       np->full_name);
			irq = 13;
		} else {
			irq = np->intrs[0].line;
		}
		pmhw->regbase = base;
		pmhw->irq = irq;
		pmhw->node = np;
		if (device_is_compatible(np, "keylargo-ata")) {
			if (strcmp(np->name, "ata-4") == 0)
				pmhw->kind = controller_kl_ata4;
			else
				pmhw->kind = controller_kl_ata3;
		} else if (device_is_compatible(np, "heathrow-ata"))
			pmhw->kind = controller_heathrow;
		else
			pmhw->kind = controller_ohare;

		bidp = (int *)get_property(np, "AAPL,bus-id", NULL);
		pmhw->aapl_bus_id =  bidp ? *bidp : 0;

		if (pmhw->kind == controller_kl_ata4) {
			char* cable = get_property(np, "cable-type", NULL);
			if (cable && !strncmp(cable, "80-", 3))
				pmhw->kind = controller_kl_ata4_80;
		}

		/* Make sure we have sane timings */
		sanitize_timings(i);

		if (np->parent && np->parent->name
		    && strcasecmp(np->parent->name, "media-bay") == 0) {
#ifdef CONFIG_PMAC_PBOOK
			media_bay_set_ide_infos(np->parent,base,irq,i);
#endif /* CONFIG_PMAC_PBOOK */
			in_bay = 1;
			if (!bidp)
				pmhw->aapl_bus_id = 1;
		} else if (pmhw->kind == controller_ohare) {
			/* The code below is having trouble on some ohare machines
			 * (timing related ?). Until I can put my hand on one of these
			 * units, I keep the old way
			 */
			ppc_md.feature_call(PMAC_FTR_IDE_ENABLE, np, 0, 1);
		} else {
 			/* This is necessary to enable IDE when net-booting */
			printk(KERN_INFO "pmac_ide: enabling IDE bus ID %d\n",
				pmhw->aapl_bus_id);
			ppc_md.feature_call(PMAC_FTR_IDE_RESET, np, pmhw->aapl_bus_id, 1);
			ppc_md.feature_call(PMAC_FTR_IDE_ENABLE, np, pmhw->aapl_bus_id, 1);
			mdelay(10);
			ppc_md.feature_call(PMAC_FTR_IDE_RESET, np, pmhw->aapl_bus_id, 0);
			big_delay = 1;
		}

		hwif = &ide_hwifs[i];
		pmac_ide_init_hwif_ports(&hwif->hw, base, 0, &hwif->irq);
		memcpy(hwif->io_ports, hwif->hw.io_ports, sizeof(hwif->io_ports));
		hwif->chipset = ide_pmac;
		hwif->noprobe = !hwif->io_ports[IDE_DATA_OFFSET] || in_bay;
#ifdef CONFIG_PMAC_PBOOK
		if (in_bay && check_media_bay_by_base(base, MB_CD) == 0)
			hwif->noprobe = 0;
#endif /* CONFIG_PMAC_PBOOK */

#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC
		if (np->n_addrs >= 2) {
			/* has a DBDMA controller channel */
			pmac_ide_setup_dma(np, i);
		}
#endif /* CONFIG_BLK_DEV_IDEDMA_PMAC */

		++i;
	}
	pmac_ide_count = i;
	if (big_delay)
		mdelay(IDE_WAKEUP_DELAY_MS);

#ifdef CONFIG_PMAC_PBOOK
	pmu_register_sleep_notifier(&idepmac_sleep_notifier);
#endif /* CONFIG_PMAC_PBOOK */
	register_reboot_notifier(&pmac_ide_reboot_notifier);
}

#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC

static void __init 
pmac_ide_setup_dma(struct device_node *np, int ix)
{
	pmac_ide[ix].dma_resource = request_OF_resource(np, 1, " (mac-io IDE DMA)");
	if (!pmac_ide[ix].dma_resource) {
		printk(KERN_ERR "ide-pmac(%s): can't request DMA resource !\n", np->name);
		return;
	}

	pmac_ide[ix].dma_regs =
		(volatile struct dbdma_regs*)ioremap(np->addrs[1].address, 0x200);

	/*
	 * Allocate space for the DBDMA commands.
	 * The +2 is +1 for the stop command and +1 to allow for
	 * aligning the start address to a multiple of 16 bytes.
	 */
	pmac_ide[ix].dma_table = (struct dbdma_cmd*)
	       kmalloc((MAX_DCMDS + 2) * sizeof(struct dbdma_cmd), GFP_KERNEL);
	if (pmac_ide[ix].dma_table == 0) {
		printk(KERN_ERR "%s: unable to allocate DMA command list\n",
		       ide_hwifs[ix].name);
		return;
	}

	ide_hwifs[ix].dmaproc = &pmac_ide_dmaproc;
#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC_AUTO
	if (!noautodma)
		ide_hwifs[ix].autodma = 1;
#endif
}

/*
 * pmac_ide_build_dmatable builds the DBDMA command list
 * for a transfer and sets the DBDMA channel to point to it.
 */
static int __pmac
pmac_ide_build_dmatable(ide_drive_t *drive, int ix, int wr)
{
	struct dbdma_cmd *table, *tstart;
	int count = 0;
	struct request *rq = HWGROUP(drive)->rq;
	struct buffer_head *bh = rq->bh;
	unsigned int size, addr;
	volatile struct dbdma_regs *dma = pmac_ide[ix].dma_regs;

	table = tstart = (struct dbdma_cmd *) DBDMA_ALIGN(pmac_ide[ix].dma_table);

#ifdef IDE_PMAC_DEBUG
	if (in_le32(&dma->status) & (RUN|ACTIVE))
		printk("ide-pmac: channel status not stopped ! (%x)\n",
			in_le32(&dma->status));
#endif	
	/* Make sure channel is stopped and all error conditions are clear */
	out_le32(&dma->control, (RUN|PAUSE|FLUSH|WAKE|DEAD) << 16);
	while (in_le32(&dma->status) & RUN)
		udelay(1);

	do {
		/*
		 * Determine addr and size of next buffer area.  We assume that
		 * individual virtual buffers are always composed linearly in
		 * physical memory.  For example, we assume that any 8kB buffer
		 * is always composed of two adjacent physical 4kB pages rather
		 * than two possibly non-adjacent physical 4kB pages.
		 */
		if (bh == NULL) {  /* paging requests have (rq->bh == NULL) */
			addr = virt_to_bus(rq->buffer);
			size = rq->nr_sectors << 9;
		} else {
			/* group sequential buffers into one large buffer */
			addr = virt_to_bus(bh->b_data);
			size = bh->b_size;
			while ((bh = bh->b_reqnext) != NULL) {
				if ((addr + size) != virt_to_bus(bh->b_data))
					break;
				size += bh->b_size;
			}
		}

		/*
		 * Fill in the next DBDMA command block.
		 * Note that one DBDMA command can transfer
		 * at most 65535 bytes.
		 */
#ifdef IDE_PMAC_DEBUG
		if (size & 0x01)
			printk("ide-pmac: odd size transfer ! (%d)\n", size);
#endif			
		while (size) {
			unsigned int tc = (size < 0xfe00)? size: 0xfe00;

			if (++count >= MAX_DCMDS) {
				printk(KERN_WARNING "%s: DMA table too small\n",
				       drive->name);
				return 0; /* revert to PIO for this request */
			}
			st_le16(&table->command, wr? OUTPUT_MORE: INPUT_MORE);
			st_le16(&table->req_count, tc);
			st_le32(&table->phy_addr, addr);
			table->cmd_dep = 0;
			table->xfer_status = 0;
			table->res_count = 0;
			addr += tc;
			size -= tc;
			++table;
		}
	} while (bh != NULL);

	/* convert the last command to an input/output last command */
	if (count)
		st_le16(&table[-1].command, wr? OUTPUT_LAST: INPUT_LAST);
	else
		printk(KERN_DEBUG "%s: empty DMA table?\n", drive->name);

	/* add the stop command to the end of the list */
	memset(table, 0, sizeof(struct dbdma_cmd));
	out_le16(&table->command, DBDMA_STOP);

	out_le32(&dma->cmdptr, virt_to_bus(tstart));
	return 1;
}


static __inline__ unsigned char
dma_bits_to_command(unsigned char bits)
{
	if(bits & 0x04)
		return XFER_MW_DMA_2;
	if(bits & 0x02)
		return XFER_MW_DMA_1;
	if(bits & 0x01)
		return XFER_MW_DMA_0;
	return 0;
}

static __inline__ unsigned char
udma_bits_to_command(unsigned char bits, int high_speed)
{
	if (high_speed) {
		if(bits & 0x10)
			return XFER_UDMA_4;
		if(bits & 0x08)
			return XFER_UDMA_3;
	}
	if(bits & 0x04)
		return XFER_UDMA_2;
	if(bits & 0x02)
		return XFER_UDMA_1;
	if(bits & 0x01)
		return XFER_UDMA_0;
	return 0;
}

/* Calculate MultiWord DMA timings */
static int __pmac
pmac_ide_mdma_enable(ide_drive_t *drive, int idx)
{
	byte bits = drive->id->dma_mword & 0x07;
	byte feature = dma_bits_to_command(bits);
	u32 *timings;
	int drive_cycle_time;
	struct hd_driveid *id = drive->id;
	int ret;

	/* Set feature on drive */
    	printk(KERN_INFO "%s: Enabling MultiWord DMA %d\n", drive->name, feature & 0xf);
	ret = pmac_ide_do_setfeature(drive, feature);
	if (ret) {
	    	printk(KERN_WARNING "%s: Failed !\n", drive->name);
	    	return 0;
	}

	if (!drive->init_speed)
		drive->init_speed = feature;
	
	/* which drive is it ? */
	if (drive->select.b.unit & 0x01)
		timings = &pmac_ide[idx].timings[1];
	else
		timings = &pmac_ide[idx].timings[0];

	/* Check if drive provide explicit cycle time */
	if ((id->field_valid & 2) && (id->eide_dma_time))
		drive_cycle_time = id->eide_dma_time;
	else
		drive_cycle_time = 0;

	/* Calculate controller timings */
	set_timings_mdma(pmac_ide[idx].kind, timings, feature, drive_cycle_time);

	drive->current_speed = feature;	
	return 1;
}

/* Calculate Ultra DMA timings */
static int __pmac
pmac_ide_udma_enable(ide_drive_t *drive, int idx, int high_speed)
{
	byte bits = drive->id->dma_ultra & 0x1f;
	byte feature = udma_bits_to_command(bits, high_speed);
	u32 *timings;
	int ret;

	/* Set feature on drive */
    	printk(KERN_INFO "%s: Enabling Ultra DMA %d\n", drive->name, feature & 0xf);
	ret = pmac_ide_do_setfeature(drive, feature);
	if (ret) {
		printk(KERN_WARNING "%s: Failed !\n", drive->name);
		return 0;
	}

	if (!drive->init_speed)
		drive->init_speed = feature;

	/* which drive is it ? */
	if (drive->select.b.unit & 0x01)
		timings = &pmac_ide[idx].timings[1];
	else
		timings = &pmac_ide[idx].timings[0];

	set_timings_udma(timings, feature);

	drive->current_speed = feature;	
	return 1;
}

static int __pmac
pmac_ide_check_dma(ide_drive_t *drive)
{
	int ata4, udma, idx;
	struct hd_driveid *id = drive->id;
	int enable = 1;

	drive->using_dma = 0;
	
	idx = pmac_ide_find(drive);
	if (idx < 0)
		return 0;
		
	if (drive->media == ide_floppy)
		enable = 0;
	if (((id->capability & 1) == 0) && !check_drive_lists(drive, GOOD_DMA_DRIVE))
		enable = 0;
	if (check_drive_lists(drive, BAD_DMA_DRIVE))
		enable = 0;

	udma = 0;
	ata4 = (pmac_ide[idx].kind == controller_kl_ata4 ||
		pmac_ide[idx].kind == controller_kl_ata4_80);
			
	if(enable) {
		if (ata4 && (drive->media == ide_disk) &&
		    (id->field_valid & 0x0004) && (id->dma_ultra & 0x1f)) {
			/* UltraDMA modes. */
			drive->using_dma = pmac_ide_udma_enable(drive, idx,
				pmac_ide[idx].kind == controller_kl_ata4_80);
		}
		if (!drive->using_dma && (id->dma_mword & 0x0007)) {
			/* Normal MultiWord DMA modes. */
			drive->using_dma = pmac_ide_mdma_enable(drive, idx);
		}
		OUT_BYTE(0, IDE_CONTROL_REG);
		/* Apply settings to controller */
		pmac_ide_selectproc(drive);
	}
	return 0;
}

static int __pmac
pmac_ide_dmaproc(ide_dma_action_t func, ide_drive_t *drive)
{
	int ix, dstat;
	volatile struct dbdma_regs *dma;
	byte unit = (drive->select.b.unit & 0x01);
	byte ata4;

	/* Can we stuff a pointer to our intf structure in config_data
	 * or select_data in hwif ?
	 */
	ix = pmac_ide_find(drive);
	if (ix < 0)
		return 0;		
	dma = pmac_ide[ix].dma_regs;
	ata4 = (pmac_ide[ix].kind == controller_kl_ata4 ||
		pmac_ide[ix].kind == controller_kl_ata4_80);
	
	switch (func) {
	case ide_dma_off:
		printk(KERN_INFO "%s: DMA disabled\n", drive->name);
	case ide_dma_off_quietly:
		drive->using_dma = 0;
		break;
	case ide_dma_on:
	case ide_dma_check:
		pmac_ide_check_dma(drive);
		break;
	case ide_dma_read:
	case ide_dma_write:
		if (!pmac_ide_build_dmatable(drive, ix, func==ide_dma_write))
			return 1;
		/* Apple adds 60ns to wrDataSetup on reads */
		if (ata4 && (pmac_ide[ix].timings[unit] & TR_66_UDMA_EN)) {
			out_le32((unsigned *)(IDE_DATA_REG + 0x200 + _IO_BASE),
				pmac_ide[ix].timings[unit] + 
				((func == ide_dma_read) ? 0x00800000UL : 0));
			(void)in_le32((unsigned *)(IDE_DATA_REG + 0x200 + _IO_BASE));
		}
		drive->waiting_for_dma = 1;
		if (drive->media != ide_disk)
			return 0;
		ide_set_handler(drive, &ide_dma_intr, WAIT_CMD, NULL);
		OUT_BYTE(func==ide_dma_write? WIN_WRITEDMA: WIN_READDMA,
			 IDE_COMMAND_REG);
	case ide_dma_begin:
		out_le32(&dma->control, (RUN << 16) | RUN);
		/* Make sure it gets to the controller right now */
		(void)in_le32(&dma->control);
		break;
	case ide_dma_end: /* returns 1 on error, 0 otherwise */
		drive->waiting_for_dma = 0;
		dstat = in_le32(&dma->status);
		out_le32(&dma->control, ((RUN|WAKE|DEAD) << 16));
		/* verify good dma status */
		return (dstat & (RUN|DEAD|ACTIVE)) != RUN;
	case ide_dma_test_irq: /* returns 1 if dma irq issued, 0 otherwise */
		/* We have to things to deal with here:
		 * 
		 * - The dbdma won't stop if the command was started
		 * but completed with an error without transfering all
		 * datas. This happens when bad blocks are met during
		 * a multi-block transfer.
		 * 
		 * - The dbdma fifo hasn't yet finished flushing to
		 * to system memory when the disk interrupt occurs.
		 * 
		 * The trick here is to increment drive->waiting_for_dma,
		 * and return as if no interrupt occured. If the counter
		 * reach a certain timeout value, we then return 1. If
		 * we really got the interrupt, it will happen right away
		 * again.
		 * Apple's solution here may be more elegant. They issue
		 * a DMA channel interrupt (a separate irq line) via a DBDMA
		 * NOP command just before the STOP, and wait for both the
		 * disk and DBDMA interrupts to have completed.
		 */
		 
		/* If ACTIVE is cleared, the STOP command have passed and
		 * transfer is complete.
		 */
		if (!(in_le32(&dma->status) & ACTIVE))
			return 1;
		if (!drive->waiting_for_dma)
			printk(KERN_WARNING "ide%d, ide_dma_test_irq \
				called while not waiting\n", ix);

		/* If dbdma didn't execute the STOP command yet, the
		 * active bit is still set */
		drive->waiting_for_dma++;
		if (drive->waiting_for_dma >= DMA_WAIT_TIMEOUT) {
			printk(KERN_WARNING "ide%d, timeout waiting \
				for dbdma command stop\n", ix);
			return 1;
		}
		udelay(1);
		return 0;

		/* Let's implement tose just in case someone wants them */
	case ide_dma_bad_drive:
	case ide_dma_good_drive:
		return check_drive_lists(drive, (func == ide_dma_good_drive));
	case ide_dma_verbose:
		return report_drive_dmaing(drive);
	case ide_dma_retune:
	case ide_dma_lostirq:
	case ide_dma_timeout:
		printk(KERN_WARNING "ide_pmac_dmaproc: chipset supported %s func only: %d\n", ide_dmafunc_verbose(func),  func);
		return 1;
	default:
		printk(KERN_WARNING "ide_pmac_dmaproc: unsupported %s func: %d\n", ide_dmafunc_verbose(func), func);
		return 1;
	}
	return 0;
}
#endif /* CONFIG_BLK_DEV_IDEDMA_PMAC */

static void __pmac
idepmac_sleep_device(ide_drive_t *drive, int i, unsigned base)
{
	int j;
	
	/* FIXME: We only handle the master IDE disk, we shoud
	 *        try to fix CD-ROMs here
	 */
	switch (drive->media) {
	case ide_disk:
		/* Spin down the drive */
		outb(drive->select.all, base+0x60);
		(void)inb(base+0x60);
		udelay(100);
		outb(0x0, base+0x30);
		outb(0x0, base+0x20);
		outb(0x0, base+0x40);
		outb(0x0, base+0x50);
		outb(0xe0, base+0x70);
		outb(0x2, base+0x160);   
		for (j = 0; j < 10; j++) {
			int status;
			mdelay(100);
			status = inb(base+0x70);
			if (!(status & BUSY_STAT) && (status & DRQ_STAT))
				break;
		}
		break;
	case ide_cdrom:
		// todo
		break;
	case ide_floppy:
		// todo
		break;
	}
}

#ifdef CONFIG_PMAC_PBOOK
static void __pmac
idepmac_wake_device(ide_drive_t *drive, int used_dma)
{
	/* We force the IDE subdriver to check for a media change
	 * This must be done first or we may lost the condition
	 *
	 * Problem: This can schedule. I moved the block device
	 * wakeup almost late by priority because of that.
	 */
	if (DRIVER(drive) && DRIVER(drive)->media_change)
		DRIVER(drive)->media_change(drive);

	/* We kick the VFS too (see fix in ide.c revalidate) */
	check_disk_change(MKDEV(HWIF(drive)->major, (drive->select.b.unit) << PARTN_BITS));
	
#ifdef CONFIG_BLK_DEV_IDEDMA_PMAC
	/* We re-enable DMA on the drive if it was active. */
	/* This doesn't work with the CD-ROM in the media-bay, probably
	 * because of a pending unit attention. The problem if that if I
	 * clear the error, the filesystem dies.
	 */
	if (used_dma && !ide_spin_wait_hwgroup(drive)) {
		/* Lock HW group */
		HWGROUP(drive)->busy = 1;
		pmac_ide_check_dma(drive);
		HWGROUP(drive)->busy = 0;
		spin_unlock_irq(&io_request_lock);
	}
#endif /* CONFIG_BLK_DEV_IDEDMA_PMAC */
}

static void __pmac
idepmac_sleep_interface(int i, unsigned base, int mediabay)
{
	struct device_node* np = pmac_ide[i].node;

	/* We clear the timings */
	pmac_ide[i].timings[0] = 0;
	pmac_ide[i].timings[1] = 0;
	
	/* The media bay will handle itself just fine */
	if (mediabay)
		return;
	
	/* Disable the bus */
	ppc_md.feature_call(PMAC_FTR_IDE_ENABLE, np, pmac_ide[i].aapl_bus_id, 0);
}

static void __pmac
idepmac_wake_interface(int i, unsigned long base, int mediabay)
{
	struct device_node* np = pmac_ide[i].node;

	if (!mediabay) {
		/* Revive IDE disk and controller */
		ppc_md.feature_call(PMAC_FTR_IDE_RESET, np, pmac_ide[i].aapl_bus_id, 1);
		ppc_md.feature_call(PMAC_FTR_IDE_ENABLE, np, pmac_ide[i].aapl_bus_id, 1);
		mdelay(10);
		ppc_md.feature_call(PMAC_FTR_IDE_RESET, np, pmac_ide[i].aapl_bus_id, 0);
	}
}

static void
idepmac_sleep_drive(ide_drive_t *drive, int idx, unsigned long base)
{
	int unlock = 0;

	/* Wait for HW group to complete operations */
	if (ide_spin_wait_hwgroup(drive)) {
		// What can we do here ? Wake drive we had already
		// put to sleep and return an error ?
	} else {
		unlock = 1;
		/* Lock HW group */
		HWGROUP(drive)->busy = 1;
		/* Stop the device */
		idepmac_sleep_device(drive, idx, base);
	}
	if (unlock)
		spin_unlock_irq(&io_request_lock);
}

static void
idepmac_wake_drive(ide_drive_t *drive, unsigned long base)
{
	unsigned long flags;
	int j;
	
	/* Reset timings */
	pmac_ide_selectproc(drive);
	mdelay(10);
	
	/* Wait up to 20 seconds for the drive to be ready */
	for (j = 0; j < 200; j++) {
		int status;
		mdelay(100);
		outb(drive->select.all, base + 0x60);
		if (inb(base + 0x60) != drive->select.all)
			continue;
		status = inb(base + 0x70);
		if (!(status & BUSY_STAT))
			break;
	}

	/* We resume processing on the HW group */
	spin_lock_irqsave(&io_request_lock, flags);
	HWGROUP(drive)->busy = 0;
	if (!list_empty(&drive->queue.queue_head))
		ide_do_request(HWGROUP(drive), 0);
	spin_unlock_irqrestore(&io_request_lock, flags);			
}

/* Note: We support only master drives for now. This will have to be
 * improved if we want to handle sleep on the iMacDV where the CD-ROM
 * is a slave
 */
static int __pmac
idepmac_notify_sleep(struct pmu_sleep_notifier *self, int when)
{
	int i, ret;
	unsigned long base;
	int big_delay;
 
	switch (when) {
	case PBOOK_SLEEP_REQUEST:
		break;
	case PBOOK_SLEEP_REJECT:
		break;
	case PBOOK_SLEEP_NOW:
		for (i = 0; i < pmac_ide_count; ++i) {
			ide_hwif_t *hwif;
			int dn;

			if ((base = pmac_ide[i].regbase) == 0)
				continue;

			hwif = &ide_hwifs[i];
			for (dn=0; dn<MAX_DRIVES; dn++) {
				if (!hwif->drives[dn].present)
					continue;
				idepmac_sleep_drive(&hwif->drives[dn], i, base);
			}
			/* Disable irq during sleep */
			disable_irq(pmac_ide[i].irq);
			
			/* Check if this is a media bay with an IDE device or not
			 * a media bay.
			 */
			ret = check_media_bay_by_base(base, MB_CD);
			if ((ret == 0) || (ret == -ENODEV))
				idepmac_sleep_interface(i, base, (ret == 0));
		}
		break;
	case PBOOK_WAKE:
		big_delay = 0;
		for (i = 0; i < pmac_ide_count; ++i) {

			if ((base = pmac_ide[i].regbase) == 0)
				continue;
				
			/* Make sure we have sane timings */		
			sanitize_timings(i);

			/* Check if this is a media bay with an IDE device or not
			 * a media bay
			 */
			ret = check_media_bay_by_base(base, MB_CD);
			if ((ret == 0) || (ret == -ENODEV)) {
				idepmac_wake_interface(i, base, (ret == 0));				
				big_delay = 1;
			}

		}
		/* Let hardware get up to speed */
		if (big_delay)
			mdelay(IDE_WAKEUP_DELAY_MS);
	
		for (i = 0; i < pmac_ide_count; ++i) {
			ide_hwif_t *hwif;
			int used_dma, dn;
			int irq_on = 0;
			
			if ((base = pmac_ide[i].regbase) == 0)
				continue;
				
			hwif = &ide_hwifs[i];
			for (dn=0; dn<MAX_DRIVES; dn++) {
				ide_drive_t *drive = &hwif->drives[dn];
				if (!drive->present)
					continue;
				/* We don't have re-configured DMA yet */
				used_dma = drive->using_dma;
				drive->using_dma = 0;
				idepmac_wake_drive(drive, base);
				if (!irq_on) {
					enable_irq(pmac_ide[i].irq);
					irq_on = 1;
				}
				idepmac_wake_device(drive, used_dma);
			}
			if (!irq_on)
				enable_irq(pmac_ide[i].irq);
		}
		break;
	}
	return PBOOK_SLEEP_OK;
}
#endif /* CONFIG_PMAC_PBOOK */

static int __pmac
pmac_ide_notify_reboot(struct notifier_block *this, unsigned long code, void *x)
{
	int i, gotone;
	unsigned long base;

	if (code != SYS_HALT && code != SYS_POWER_OFF)
		return 0;

	gotone = 0;
	for (i = 0; i < pmac_ide_count; ++i) {
		ide_hwif_t *hwif;
		ide_drive_t *drive;
		int unlock = 0;
		int dn;

		if ((base = pmac_ide[i].regbase) == 0)
				continue;	

		hwif = &ide_hwifs[i];
		for (dn=0; dn<MAX_DRIVES; dn++) {
			drive = &hwif->drives[dn];
			if (drive->present) {
				gotone = 1;
				/* Wait for HW group to complete operations */
				if (ide_spin_wait_hwgroup(drive)) {
					// What can we do here ? Wake drive we had already
					// put to sleep and return an error ?
				} else {
					unlock = 1;
					/* Lock HW group */
					HWGROUP(drive)->busy = 1;

					/* Stop the device */
					idepmac_sleep_device(drive, i, base);
				}
			}
			if (unlock)
				spin_unlock_irq(&io_request_lock);
		}
	}
	if (gotone)
		mdelay(1000);
		
	return NOTIFY_DONE;
}
