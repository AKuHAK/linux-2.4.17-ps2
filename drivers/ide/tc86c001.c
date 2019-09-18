/*
 *  linux/drivers/ide/tc86c001.c	Version 0.00	Aug. 09, 2002
 *
 *  Copyright (C) 2002	Toshiba Corporation
 *  May be copied or modified under the terms of the GNU General Public License
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/hdreg.h>
#include <linux/ide.h>
#include <linux/delay.h>
#include <linux/init.h>

#include <asm/io.h>

#include "ide_modes.h"

/* from ide-cd.h */
#define CD_FRAMESIZE	2048
#define SECTOR_BITS			9
#define SECTORS_PER_FRAME	(CD_FRAMESIZE >> SECTOR_BITS)

/* add command line TC86c001 DMA setteing */
byte tc86c001udma_mode = 0;

static void tc86c001_tune_drive (ide_drive_t *drive, byte pio)	/* PIO */
{
	u16 master_data = 0;
	u16 slave_data = 0;
	u16 mask_data = 0;
	byte speed = XFER_PIO_0;
	int is_slave = (&HWIF(drive)->drives[1] == drive);
	unsigned long master_port =
		pci_resource_start(HWIF(drive)->pci_dev, 5);
	unsigned long slave_port = master_port + 0x02;
	ide_hwif_t *hwif = HWIF(drive);

	if (!hwif->channel) { /* primary */
		pio = ide_get_best_pio_mode(drive, pio, 5, NULL);
		switch (pio) {
		case 4: mask_data = 0x0400; speed = XFER_PIO_4; break;
		case 3: mask_data = 0x0300; speed = XFER_PIO_3; break;
		case 2: mask_data = 0x0200; speed = XFER_PIO_2; break;
		case 1: mask_data = 0x0100; speed = XFER_PIO_1; break;
		case 0:
		default: break;
		}

		if (is_slave) {
			/* System Control2 */
			/* slave_data = inw(slave_port); */
			slave_data |= mask_data;
			outw(slave_data, slave_port);
		} else {	/* master */
			/* System Control1 */
			master_data = inw(master_port);
			master_data &= 0xf80f;	/* PIO(bit4-bit10)clear(???) */
			master_data |= mask_data;
			outw(master_data, master_port);
		}
	}

	drive->current_speed = speed;
	ide_config_drive_speed(drive, speed);
}

static int tc86c001_tune_chipset (ide_drive_t *drive, byte speed)	/* DMA */
{
	u16 master_data = 0;
	u16 slave_data = 0;
	u16 mask_data = 0;
	int is_slave = (&HWIF(drive)->drives[1] == drive);
	unsigned long master_port = pci_resource_start(HWIF(drive)->pci_dev, 5);
	unsigned long slave_port = master_port + 0x02;
	ide_hwif_t *hwif = HWIF(drive);
	int err;

	if (!hwif->channel) {
		switch (speed) {
		case XFER_UDMA_4:	mask_data = 0x00c0; break;
		case XFER_UDMA_3:	mask_data = 0x00b0; break;
		case XFER_UDMA_2:	mask_data = 0x00a0; break;
		case XFER_UDMA_1:	mask_data = 0x0090; break;
		case XFER_UDMA_0:	mask_data = 0x0080; break;
		case XFER_MW_DMA_2:	mask_data = 0x0070; break;
		case XFER_MW_DMA_1:	mask_data = 0x0060; break;
		case XFER_MW_DMA_0:	mask_data = 0x0050; break;
		default: return -1;
		}

		if (is_slave) {	/* slave */
			/* System Control2 */
			/* slave_data = inw(slave_port); */
			if (drive->media != ide_cdrom) {
				slave_data |= mask_data;
				outw(slave_data, slave_port);
			} else {
				outw(0x0400,slave_port);
			}
		} else {	/* master */
			if (drive->media != ide_cdrom) {
				/* System Control1 */
				master_data = inw(master_data);
				master_data &= 0xf80f;	/* DMA(bit4-bit10)clear(???) */
				master_data |= mask_data;
				outw(master_data, master_port);
			} else {
				outw(0x0400,master_port);
			}
		}
	}

	if (!drive->init_speed)
		drive->init_speed = speed;
	err = ide_config_drive_speed(drive, speed);
	drive->current_speed = speed;
	return err;
}

static int tc86c001_config_drive_for_dma (ide_drive_t *drive)
{
	struct hd_driveid *id	= drive->id;
	int ultra		= 1;
	byte speed		= 0;
	byte udma_66	= eighty_ninty_three(drive);	/* hwif->udma_four 0:default 1:ATA66 */

	if ((id->dma_ultra & 0x0010) && (ultra)) {
		speed = (udma_66) ? XFER_UDMA_4 : XFER_UDMA_2;
	} else if ((id->dma_ultra & 0x0008) && (ultra)) {
		speed = (udma_66) ? XFER_UDMA_3 : XFER_UDMA_1;
	} else if ((id->dma_ultra & 0x0004) && (ultra)) {
		speed = XFER_UDMA_2;
	} else if ((id->dma_ultra & 0x0002) && (ultra)) {
		speed = XFER_UDMA_1;
	} else if ((id->dma_ultra & 0x0001) && (ultra)) {
		speed = XFER_UDMA_0;
	} else if (id->dma_mword & 0x0004) {
		speed = XFER_MW_DMA_2;
	} else if (id->dma_mword & 0x0002) {
		speed = XFER_MW_DMA_1;
	} else if (id->dma_1word & 0x0004) {
		speed = XFER_SW_DMA_2;
	} else {
/*		speed = XFER_PIO_0 + ide_get_best_pio_mode(drive, 255, 5, NULL);*/
		return ide_dma_off_quietly;
	}

	/* add command line TC86c001 DMA setteing */
	if(tc86c001udma_mode && speed >= XFER_UDMA_0) {
		speed = (tc86c001udma_mode > speed) ? speed : tc86c001udma_mode;
		printk("tc86c001udma_mode = 0x%04x speed = 0x%04x\n",tc86c001udma_mode,speed);
	}

	(void) tc86c001_tune_chipset(drive, speed);

	return ((int)	((id->dma_ultra >> 11) & 7) ? ide_dma_on :
			((id->dma_ultra >> 8) & 7) ? ide_dma_on :
			((id->dma_mword >> 8) & 7) ? ide_dma_on :
			((id->dma_1word >> 8) & 7) ? ide_dma_on :
							 ide_dma_off_quietly);
}

static int tc86c001_dmaproc(ide_dma_action_t func, ide_drive_t *drive)
{
	int rc;
	unsigned long control_base = pci_resource_start(HWIF(drive)->pci_dev, 5);
	unsigned short	mask_data = 0x0440;

	switch (func) {
	case ide_dma_check:
		return ide_dmaproc((ide_dma_action_t) tc86c001_config_drive_for_dma(drive), drive);
	case ide_dma_read:
	case ide_dma_write:
		if (drive->media == ide_cdrom) {
			struct request *rq = HWGROUP(drive)->rq;
			int nsect, sector, nframes, frame;

			HWIF(drive)->dmaproc(ide_dma_check, drive); /* DMA ON */
			switch(drive->init_speed) {
			case XFER_UDMA_4:	mask_data = 0x00c0; break;
			case XFER_UDMA_3:	mask_data = 0x00b0; break;
			case XFER_UDMA_2:	mask_data = 0x00a0; break;
			case XFER_UDMA_1:	mask_data = 0x0090; break;
			case XFER_UDMA_0:	mask_data = 0x0080; break;
			case XFER_MW_DMA_2:	mask_data = 0x0070; break;
			case XFER_MW_DMA_1:	mask_data = 0x0060; break;
			case XFER_MW_DMA_0:	mask_data = 0x0050; break;
			}

			/* set transmit value(DMA mode Transmit) */
			if (drive->select.b.unit & 0x1) {
				/* slave device */
				outw(mask_data, control_base + 0x02);
			} else { /* master device */
				outw(mask_data, control_base + 0x00);
			}

			/* Sector Count Set */
			/* Number of sectors to transfer. */
			nsect = rq->nr_sectors;
			/* Starting sector. */
			sector = rq->sector;

			/* Convert from sectors to cdrom blocks,
			   rounding up the transfer length if
			   needed. */
			nframes = (nsect + SECTORS_PER_FRAME-1) / SECTORS_PER_FRAME;
			frame = sector / SECTORS_PER_FRAME;

			outw(nframes, control_base + 0x0a);

			/* request DMA transmit */
			if ((rc = ide_dmaproc(func, drive)) == 1) {
				/* rc=0: transmit normal , rc=1: request PIO transmit */
				/* set transmit value(PIO mode4 Transmit) */
				if (drive->select.b.unit & 0x1) {
					/* slave device */
					outw(0x0400, control_base + 0x02);
				} else { /* master device */
					outw(0x0400, control_base + 0x00);
				}
			}
			/* request pio transmit */
			return rc;
		}
		break;
	case ide_dma_end:
		if (drive->media == ide_cdrom) {
			rc = ide_dmaproc(func, drive);	/* use standard DMA stuff */
			/* set transmit value(PIO mode4 Transmit) */
			if (drive->select.b.unit & 0x1) {
				/* slave device */
				outw(0x0400, control_base + 0x02);
			} else { /* master device */
				outw(0x0400, control_base + 0x00);
			}
			return rc;
		}
		break;
	default :
		break;
	}
	/* Other cases are done by generic IDE-DMA code. */
	return ide_dmaproc(func, drive);
}

unsigned int __init pci_init_tc86c001 (struct pci_dev *dev, const char *name)
{
#if 0
	unsigned short	mask_b15 = 0x8000;	/* bit15(Soft Reset) mask */
	unsigned short	mask_b14 = 0x4000;	/* bit14(FIFO Reset) mask */
	unsigned short	mask_b11 = 0x0800;	/* bit11(ATA Hard Reset) mask */
	unsigned long	control_base = pci_resource_start(dev, 5);
	unsigned short	val;

	/* System Control1 Register bit15(Soft Reset) Set */
	val = inw(control_base);
	outw(val | mask_b15, control_base);
	mdelay(20);

	/* System Control1 Register bit14(FIFO Reset) Set */
	val = inw(control_base);
	outw(val | mask_b14, control_base);
	mdelay(20);

	/* System Control1 Register bit11(ATA Hard Reset) Set */
	val = inw(control_base);
	outw(val | mask_b11, control_base);
	mdelay(40);

	/* System Control1 Register 0x0000(Reset Clear) Set */
	outw(0x0000, control_base);
#endif
	return dev->irq;
}

unsigned int __init ata66_tc86c001 (ide_hwif_t *hwif)
{
	unsigned int ata66 = 0;		/* Return */
	unsigned long control_base = pci_resource_start(hwif->pci_dev, 5);

	if (!hwif->channel) { /* primary */
		/* System Control1 Register */
		/* bit13(PDIAGN) = 0:(80pin cable) 1:(40pin cable) */
		ata66 = (inw(control_base) & 0x2000) ? 0 : 1;
	}
	return ata66;
}

void __init ide_init_tc86c001 (ide_hwif_t *hwif)
{

	hwif->tuneproc = &tc86c001_tune_drive;	/* routine to tune PIO mode for drives */
	hwif->drives[0].autotune = 1;
	hwif->drives[1].autotune = 1;

	if (!hwif->dma_base){
		printk("ide_init_tc86c001 :dma_base is not found\n");
		return;
	}

	hwif->autodma = 0;

#ifdef CONFIG_BLK_DEV_IDEDMA
	if (!noautodma){
		hwif->autodma = 1;
		hwif->dmaproc = &tc86c001_dmaproc;
		hwif->speedproc = &tc86c001_tune_chipset;
	}
	else { /* "ide=nodma" */
		hwif->dmaproc = &ide_dmaproc;	/* original */
	}

#endif /* !CONFIG_BLK_DEV_IDEDMA */
}

/* called from ide_cdrom_setup */
void tc86c001_ide_cdrom_setup(ide_drive_t *drive)
{
	ide_hwif_t *hwif = HWIF(drive);
	int is_slave = (&hwif->drives[1] == drive);
	unsigned long control_base = pci_resource_start(hwif->pci_dev, 5);
	unsigned short scc;

	if (!hwif->channel) {
		scc = inw(control_base + 0x0c); /* Sector Count Control */
		if (is_slave) {
			scc |= 0x0002;
			outw(0x0400, control_base + 0x06); /* TransferWordCount2 */
		} else {
			scc |= 0x0001;
			outw(0x0400, control_base + 0x04); /* TransferWordCount1 */
		}
		outw(scc, control_base + 0x0c); /* Sector Count Control */
	}
}

/* called from cdrom_transfer_packet_command */
void tc86c001_atapi_output_command (ide_drive_t *drive, void *buffer, unsigned int bytecount)
{
	unsigned long port = pci_resource_start(HWIF(drive)->pci_dev, 5) + 0x08;
	if (bytecount != 12) {
		printk(KERN_ERR "tc86c001_atapi_output_command: bad count %d",
		       bytecount);
		return;
	}
	outsw (port, buffer, 12 / 2);
}

/* add command line TC86c001 DMA setteing */
static int __init tc86c001_ide_dma_setup (char *line)
{
	int ints[2];

	(void) get_options(line, 2, ints);
	if (ints[0] == 1) {
		switch (ints[1]) {
			case 4: tc86c001udma_mode = XFER_UDMA_4; break;
			case 3: tc86c001udma_mode = XFER_UDMA_3; break;
			case 2: tc86c001udma_mode = XFER_UDMA_2; break;
			case 1: tc86c001udma_mode = XFER_UDMA_1; break;
			case 0: tc86c001udma_mode = XFER_UDMA_0; break;
		}
	}

	return 1;
}

__setup("tc86c001_udma=", tc86c001_ide_dma_setup);

