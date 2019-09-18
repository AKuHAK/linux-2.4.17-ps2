/*
 * adapter.c
 *
 * Xilinx System ACE Adapter component to interface System ACE to Linux
 *
 * Author: MontaVista Software, Inc.
 *         source@mvista.com
 *
 * Copyright 2002 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This driver is a bit unusual in that it is composed of two logical
 * parts where one part is the OS independent code and the other part is
 * the OS dependent code.  Xilinx provides their drivers split in this
 * fashion.  This file represents the Linux OS dependent part known as
 * the Linux adapter.  The other files in this directory are the OS
 * independent files as provided by Xilinx with no changes made to them.
 * The names exported by those files begin with XSysAce_.  All functions
 * in this file that are called by Linux have names that begin with
 * xsysace_.  Any other functions are static helper functions.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/hdreg.h>
#include <linux/slab.h>
#include <linux/blkpg.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define NR_HD		1	/* System ACE only handles one CompactFlash */
#define PARTN_BITS 4		/* Only allow 15 partitions. */

#define XSA_IRQ (31 - XPAR_INTC_0_SYSACE_0_VEC_ID)

static int xsa_major = 125;	/* EXPERIMENTAL major now.  Dynamic later. */
#define MAJOR_NR (xsa_major)
#define MAJOR_NAME "xsysace"

#define DEVICE_NAME "System ACE"
#define DEVICE_INTR do_xsa
#define DEVICE_REQUEST do_xsa_request
#define DEVICE_NR(device) (MINOR(device) >> PARTN_BITS)
#include <linux/blk.h>
#include <linux/blkdev.h>

#include <xbasic_types.h>
#include "xsysace.h"

MODULE_AUTHOR("MontaVista Software, Inc. <source@mvista.com>");
MODULE_DESCRIPTION("Xilinx System ACE block driver");
MODULE_LICENSE("GPL");

DECLARE_COMPLETION(received_int);	/* for waiting for interrupts */
static u32 ErrorMask;

static u32 save_BaseAddress;	/* Saved physical base address */
static void (*old_restart) (char *cmd) = NULL;	/* old ppc_md.restart */

static unsigned char heads;
static unsigned char sectors;
static unsigned short cylinders;

static int access_count = 0;
static char busy = 0;
static DECLARE_WAIT_QUEUE_HEAD(busy_wait);

/*
 * The following variables are used to keep track of what all has been
 * done to make error handling easier.
 */
static char reqirq = 0;		/* Has request_irq() been called? */
static char registered = 0;	/* Has devfs_register_blkdev() been called? */

/*
 * The underlying OS independent code needs space as well.  A pointer to
 * the following XSysAce structure will be passed to any XSysAce_
 * function that requires it.  However, we treat the data as an opaque
 * object in this file (meaning that we never reference any of the
 * fields inside of the structure).
 */
static XSysAce SysAce;

/* These tables are indexed by major and minor numbers. */
static int xsa_sizes[NR_HD << PARTN_BITS];	/* Size of the device (kb) */
static int xsa_blocksizes[NR_HD << PARTN_BITS];	/* Block size (bytes) */
static int xsa_hardsectsizes[NR_HD << PARTN_BITS];	/* Sector size (bytes) */
static int xsa_maxsect[NR_HD << PARTN_BITS];	/* Max request size (sectors) */
static struct hd_struct xsa_hd[NR_HD << PARTN_BITS];	/* Partition Table */

/* SAATODO: Xilinx is going to add this function.  Nuke when they do. */
unsigned int XSysAce_GetCfgAddr(XSysAce *InstancePtr)
{
    u32 Status;

    XASSERT_NONVOID(InstancePtr != NULL);
    XASSERT_NONVOID(InstancePtr->IsReady == XCOMPONENT_IS_READY);

    Status = XSysAce_mGetControlReg(InstancePtr->BaseAddress);
    if (!(Status & XSA_CR_FORCECFGADDR_MASK))
        Status = XSysAce_mGetStatusReg(InstancePtr->BaseAddress);

    return (unsigned int)((Status & XSA_SR_CFGADDR_MASK) >>
			  XSA_CR_CFGADDR_SHIFT);
}

/* SAATODO: Nuke the following line when Config stuff moved out. */
extern XSysAce_Config XSysAce_ConfigTable[];
/* SAATODO: This function will be moved into the Xilinx code. */
/*****************************************************************************/
/**
*
* Lookup the device configuration based on the sysace instance.  The table
* XSysAce_ConfigTable contains the configuration info for each device in the system.
*
* @param Instance is the index of the interface being looked up.
*
* @return
*
* A pointer to the configuration table entry corresponding to the given
* device ID, or NULL if no match is found.
*
* @note
*
* None.
*
******************************************************************************/
XSysAce_Config *
XSysAce_GetConfig(int Instance)
{
	if (Instance < 0 || Instance >= XPAR_XSYSACE_NUM_INSTANCES) {
		return NULL;
	}

	return &XSysAce_ConfigTable[Instance];
}

#ifndef CONFIG_PROC_FS
#define proc_init() 0
#define proc_cleanup()
#else
#define CFGADDR_NAME "cfgaddr"
static struct proc_dir_entry *xsysace_dir = NULL;
static struct proc_dir_entry *cfgaddr_file = NULL;

static int
cfgaddr_read(char *page, char **start,
	     off_t off, int count, int *eof, void *data)
{
	unsigned int cfgaddr;

	/* Make sure we have room for a digit (0-7), a newline and a NULL */
	if (count < 3)
		return -EINVAL;

	MOD_INC_USE_COUNT;

	cfgaddr = XSysAce_GetCfgAddr(&SysAce);

	count = sprintf(page+off, "%d\n", cfgaddr);
	*eof = 1;

	MOD_DEC_USE_COUNT;

	return count;
}

static int
cfgaddr_write(struct file *file,
	      const char *buffer, unsigned long count, void *data)
{
	char val[2];

	if (count < 1 || count > 2)
		return -EINVAL;

	MOD_INC_USE_COUNT;

	if (copy_from_user(val, buffer, count)) {
		MOD_DEC_USE_COUNT;
		return -EFAULT;
	}

	if (val[0] < '0' || val[0] > '7' || (count == 2 && !(val[1] == '\n' ||
							     val[1] == '\0'))) {
		MOD_DEC_USE_COUNT;
		return -EINVAL;
	}

	XSysAce_SetCfgAddr(&SysAce, val[0] - '0');

	MOD_DEC_USE_COUNT;

	return count;
}

static int
proc_init(void)
{
	xsysace_dir = proc_mkdir(MAJOR_NAME, NULL);
	if (!xsysace_dir)
		return -ENOMEM;
	xsysace_dir->owner = THIS_MODULE;

	cfgaddr_file = create_proc_entry(CFGADDR_NAME, 0644, xsysace_dir);
	if (!xsysace_dir) {
		remove_proc_entry(MAJOR_NAME, NULL);
		return -ENOMEM;
	}
	cfgaddr_file->read_proc = cfgaddr_read;
	cfgaddr_file->write_proc = cfgaddr_write;
	cfgaddr_file->owner = THIS_MODULE;
	return 0;
}

static void
proc_cleanup(void)
{
	if (cfgaddr_file)
		remove_proc_entry(CFGADDR_NAME, xsysace_dir);
	if (xsysace_dir)
		remove_proc_entry(MAJOR_NAME, NULL);
}
#endif

static void
xsysace_restart(char *cmd)
{
	XSysAce_ResetCfg(&SysAce);

	/* Wait for reset. */
	for (;;) ;
}

static void
lockCF(u32 force)
{
	while (XSysAce_Lock(&SysAce, force) == XST_DEVICE_BUSY) {
		/*
		 * The CompactFlash is being accessed already.  Delay
		 * and then try again.
		 */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ / 10);
	}
}
static void
unlockCF(void)
{
	XSysAce_Unlock(&SysAce);
}

static void
xsysace_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	XSysAce_InterruptHandler(&SysAce);
}

static void
EventHandler(void *CallbackRef, int Event)
{
	u32 i;

	switch (Event) {
	case XSA_EVENT_DATA_DONE:
		ErrorMask = 0;	/* No error. */
		complete(&received_int);
		break;

	case XSA_EVENT_ERROR:
		i = ErrorMask = XSysAce_GetErrors(&SysAce);

		if (i & XSA_ER_CARD_RESET)
			printk(KERN_ERR "CompactFlash failed to reset\n");
		if (i & XSA_ER_CARD_READY)
			printk(KERN_ERR "CompactFlash card failed to ready\n");
		if (i & XSA_ER_CARD_READ)
			printk(KERN_ERR "CompactFlash read command failed\n");
		if (i & XSA_ER_CARD_WRITE)
			printk(KERN_ERR "CompactFlash write command failed\n");
		if (i & XSA_ER_SECTOR_READY)
			printk(KERN_ERR
			       "CompactFlash sector failed to ready\n");
		if (i & XSA_ER_BAD_BLOCK)
			printk(KERN_ERR "CompactFlash bad block detected\n");
		if (i & XSA_ER_UNCORRECTABLE)
			printk(KERN_ERR "CompactFlash uncorrectable error\n");
		if (i & XSA_ER_SECTOR_ID)
			printk(KERN_ERR "CompactFlash sector ID not found\n");
		if (i & XSA_ER_ABORT)
			printk(KERN_ERR "CompactFlash command aborted\n");
		if (i & XSA_ER_GENERAL)
			printk(KERN_ERR "CompactFlash general error\n");

		/* SAATODO: Should we turn off the cfgaddr here? */
		if (i & XSA_ER_CFG_READ)
			printk(KERN_ERR
			       "JTAG controller couldn't read configuration from the CompactFlash\n");
		if (i & XSA_ER_CFG_ADDR)
			printk(KERN_ERR
			       "Invalid address give to JTAG controller\n");
		if (i & XSA_ER_CFG_FAIL)
			printk(KERN_ERR
			       "JTAG controller failed to configure a device\n");
		if (i & XSA_ER_CFG_INSTR)
			printk(KERN_ERR
			       "Invalid instruction during JTAG configuration\n");
		if (i & XSA_ER_CFG_INIT)
			printk(KERN_ERR "JTAG CFGINIT pin error\n");

		/* Check for errors that should reset the CompactFlash */
		if (i & (XSA_ER_CARD_RESET |
			 XSA_ER_CARD_READY |
			 XSA_ER_CARD_READ |
			 XSA_ER_CARD_WRITE |
			 XSA_ER_SECTOR_READY |
			 XSA_ER_BAD_BLOCK |
			 XSA_ER_UNCORRECTABLE |
			 XSA_ER_SECTOR_ID | XSA_ER_ABORT | XSA_ER_GENERAL)) {
			lockCF(1);
			if (XSysAce_ResetCF(&SysAce) != XST_SUCCESS)
				printk(KERN_ERR
				       "Could not reset CompactFlash\n");
			unlockCF();
			complete(&received_int);
		}

		/* Check for errors that should reset the JTAG controller */
		if (i & (XSA_ER_CFG_READ |
			 XSA_ER_CFG_ADDR |
			 XSA_ER_CFG_FAIL | XSA_ER_CFG_INSTR | XSA_ER_CFG_INIT))
			/* SAATODO: This is probably most certainly wrong. */
			XSysAce_ResetCfg(&SysAce);

		break;
	case XSA_EVENT_CFG_DONE:
		printk(KERN_WARNING "XSA_EVENT_CFG_DONE not handled yet.\n");
		break;
	default:
		printk(KERN_ERR "%s: unrecognized event %d\n",
		       DEVICE_NAME, Event);
		break;
	}
}

static void
do_xsa_request(request_queue_t * q)
{
	XStatus stat;
	unsigned long sector;
	char *op_str;
	XStatus(*op_fnc) (XSysAce * InstancePtr, u32 StartSector,
			  int NumSectors, u8 * BufferPtr);

	while (1) {
		INIT_REQUEST;

		switch (CURRENT->cmd) {
		case READ:
			op_str = "reading";
			op_fnc = XSysAce_SectorRead;
			break;
		case WRITE:
			op_str = "writing";
			op_fnc = XSysAce_SectorWrite;
			break;
		default:
			printk(KERN_CRIT "%s: unknown request.\n", DEVICE_NAME);
			end_request(0);
			continue;	/* Go on to the next request. */
		}

		sector = (CURRENT->sector
			  + xsa_hd[MINOR(CURRENT->rq_dev)].start_sect);

	      retry:
		lockCF(0);
		stat = op_fnc(&SysAce, sector, CURRENT->current_nr_sectors,
			      CURRENT->buffer);
		if (stat == XST_DEVICE_BUSY) {
			/* Release the lock while we sleep. */
			unlockCF();
			/*
			 * The CompactFlash is still busy.  Delay
			 * and then try again.
			 */
			printk(KERN_WARNING "%s: CompactFlash was busy.\n",
			       DEVICE_NAME);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(HZ / 10);
			goto retry;
		} else if (stat != XST_SUCCESS) {
			unlockCF();
			printk(KERN_ERR
			       "%s: Error %d when %s sector %lu.\n",
			       DEVICE_NAME, stat, op_str, sector);
			end_request(0);
		}

		wait_for_completion(&received_int);
		unlockCF();

		end_request(ErrorMask ? 0 : 1);
	}
}

extern struct gendisk xsa_gendisk;
static int
xsa_revalidate(kdev_t kdev)
{
	int target, max_p, start, i;
	long flags;

	target = DEVICE_NR(kdev);

	save_flags(flags);
	cli();
	if (busy || access_count > 1) {
		restore_flags(flags);
		return -EBUSY;
	}
	busy = 1;
	restore_flags(flags);

	max_p = xsa_gendisk.max_p;
	start = target << PARTN_BITS;
	for (i = max_p - 1; i >= 0; i--) {
		int minor = start + i;
		invalidate_device(MKDEV(MAJOR_NR, minor), 1);
		xsa_gendisk.part[minor].start_sect = 0;
		xsa_gendisk.part[minor].nr_sects = 0;
	}

	grok_partitions(&xsa_gendisk, target, 1 << PARTN_BITS,
			(long) cylinders * (long) heads * (long) sectors);
	busy = 0;
	wake_up(&busy_wait);
	return 0;
}

#ifdef XSA_REMOVABLE
static int
xsa_check_change(kdev_t kdev)
{
	/*
	 * For right now, there isn't a way to tell the hardware to spin
	 * down the drive.  For IBM Microdrives, removing them without a
	 * spindown is probably not very good.  Thus, we don't even want
	 * to have the System ACE driver handle removable media.  If and
	 * when we do, we'll need to figure out if we can tell if the
	 * media was changed.  This code is just a place-holder for that
	 * time and always says that the media was changed.
	 */
	return 1;
}
#endif

static int
xsa_open(struct inode *inode, struct file *file)
{
	while (busy)
		sleep_on(&busy_wait);
	access_count++;

	MOD_INC_USE_COUNT;
	return 0;
}
static int
xsa_release(struct inode *inode, struct file *file)
{
	access_count--;

	MOD_DEC_USE_COUNT;
	return 0;
}
static int
xsa_ioctl(struct inode *inode, struct file *file,
	  unsigned int cmd, unsigned long arg)
{
	if ((!inode) || !(inode->i_rdev))
		return -EINVAL;

	switch (cmd) {
	case BLKGETSIZE:
		return put_user(xsa_hd[MINOR(inode->i_rdev)].nr_sects,
				(unsigned long *) arg);
	case BLKGETSIZE64:
		return put_user((u64) xsa_hd[MINOR(inode->i_rdev)].nr_sects,
				(u64 *) arg);
	case BLKRRPART:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;

		return xsa_revalidate(inode->i_rdev);

	case HDIO_GETGEO:
		{
			struct hd_geometry *loc, g;
			loc = (struct hd_geometry *) arg;
			if (!loc)
				return -EINVAL;
			g.heads = heads;
			g.sectors = sectors;
			g.cylinders = cylinders;
			g.start = xsa_hd[MINOR(inode->i_rdev)].start_sect;
			return copy_to_user(loc, &g, sizeof g) ? -EFAULT : 0;
		}
	default:
		/* Let the block layer handle all the others. */
		return blk_ioctl(inode->i_rdev, cmd, arg);
	}
}

static struct block_device_operations xsa_fops = {
	open:xsa_open,
	release:xsa_release,
	ioctl:xsa_ioctl,
#ifdef XSA_REMOVABLE
	check_media_change:xsa_check_change,
	revalidate:xsa_revalidate
#endif
};
static struct gendisk xsa_gendisk = {	/* Generic Hard Disk */
	major_name:MAJOR_NAME,
	minor_shift:PARTN_BITS,
	max_p:1 << PARTN_BITS,
	part:xsa_hd,
	sizes:xsa_sizes,
	fops:&xsa_fops,
};

static void
cleanup(void)
{
	XSysAce_Config *cfg;

	proc_cleanup();

	if (registered) {
		blk_cleanup_queue(BLK_DEFAULT_QUEUE(xsa_major));
		del_gendisk(&xsa_gendisk);
		if (devfs_unregister_blkdev(xsa_major, DEVICE_NAME) != 0) {
			printk(KERN_ERR "%s: unable to release major %d\n",
			       DEVICE_NAME, xsa_major);
		}
	}

	if (reqirq) {
		XSysAce_DisableInterrupt(&SysAce);
		disable_irq(XSA_IRQ);
		free_irq(XSA_IRQ, NULL);
	}

	cfg = XSysAce_GetConfig(0);
	iounmap((void *) cfg->BaseAddress);
	cfg->BaseAddress = save_BaseAddress;

	if (old_restart)
		ppc_md.restart = old_restart;
}

static int __init
xsysace_init(void)
{
	static const unsigned long remap_size
	    = XPAR_SYSACE_0_HIGHADDR - XPAR_SYSACE_0_BASEADDR + 1;
	XSysAce_Config *cfg;
	XSysAce_CFParameters ident;
	XStatus stat;
	long size;
	int i;

	/* Find the config for our device. */
	cfg = XSysAce_GetConfig(0);
	if (!cfg)
		return -ENODEV;

	/* Change the addresses to be virtual; save the old ones to restore. */
	save_BaseAddress = cfg->BaseAddress;
	cfg->BaseAddress = (u32) ioremap(save_BaseAddress, remap_size);

	/* Tell the Xilinx code to bring this interface up. */
	if (XSysAce_Initialize(&SysAce, cfg->DeviceId) != XST_SUCCESS) {
		printk(KERN_ERR "%s: Could not initialize device.\n",
		       DEVICE_NAME);
		cleanup();
		return -ENODEV;
	}

	i = request_irq(XSA_IRQ, xsysace_interrupt, 0, DEVICE_NAME, NULL);
	if (i) {
		printk(KERN_ERR "%s: Could not allocate interrupt %d.\n",
		       DEVICE_NAME, XSA_IRQ);
		cleanup();
		return i;
	}
	reqirq = 1;
	XSysAce_SetEventHandler(&SysAce, EventHandler, (void *) NULL);
	XSysAce_EnableInterrupt(&SysAce);

	/* Time to identify the drive. */
	lockCF(0);
	stat = XSysAce_IdentifyCF(&SysAce, &ident);
	if (stat != XST_SUCCESS) {
		unlockCF();
		printk(KERN_ERR "%s: Could not send identify command.\n",
		       DEVICE_NAME);
		cleanup();
		return -ENODEV;
	}
#if 0				/* SAATODO: XSysAce_IdentifyCF always polls at this point. */
	wait_for_completion(&received_int);
	unlockCF();
	if (ErrorMask) {
		printk(KERN_ERR "%s: Could not identify device.\n",
		       DEVICE_NAME);
		cleanup();
		return -ENODEV;
	}
#else
	unlockCF();
#endif

	/* Fill in what we learned. */
	heads = ident.NumHeads;
	sectors = ident.NumSectorsPerTrack;
	cylinders = ident.NumCylinders;
	size = (long) cylinders *heads * sectors;

	i = devfs_register_blkdev(xsa_major, DEVICE_NAME, &xsa_fops);
	if (i < 0) {
		printk(KERN_ERR "%s: unable to register device\n", DEVICE_NAME);
		cleanup();
		return i;
	}
	if (xsa_major == 0)
		xsa_major = i;
	registered = 1;
	xsa_gendisk.major = xsa_major;

	blk_init_queue(BLK_DEFAULT_QUEUE(xsa_major), do_xsa_request);

	read_ahead[xsa_major] = 8;	/* 8 sector (4kB) read-ahead */
	add_gendisk(&xsa_gendisk);

	/* Start with zero-sized partitions, and correctly sized unit */
	memset(xsa_sizes, 0, sizeof (xsa_sizes));
	xsa_sizes[0] = size / 2;
	blk_size[xsa_major] = xsa_sizes;
	memset(xsa_hd, 0, sizeof (xsa_hd));
	xsa_hd[0].nr_sects = size;
	for (i = 0; i < (NR_HD << PARTN_BITS); i++) {
		xsa_blocksizes[i] = 1024;
		xsa_hardsectsizes[i] = 512;
		xsa_maxsect[i] = 255;
/* SAATODO: Figure out why we're only getting 1 sector requests. */
	}
	blksize_size[xsa_major] = xsa_blocksizes;
	hardsect_size[xsa_major] = xsa_hardsectsizes;
	max_sectors[xsa_major] = xsa_maxsect;

	xsa_gendisk.nr_real = NR_HD;

	register_disk(&xsa_gendisk,
		      MKDEV(xsa_major, i << PARTN_BITS), 1 << PARTN_BITS,
		      &xsa_fops, size);

	printk(KERN_INFO "%s at 0x%08X mapped to 0x%08X, irq=%d, %ldKB\n",
	       DEVICE_NAME, save_BaseAddress, cfg->BaseAddress, XSA_IRQ,
	       size / 2);

	old_restart = ppc_md.restart;
	ppc_md.restart = xsysace_restart;

	if (proc_init())
		printk(KERN_WARNING "%s could not register /proc interface.\n",
		       DEVICE_NAME);

	return 0;
}

static void __exit
xsysace_cleanup(void)
{
	cleanup();
}

EXPORT_NO_SYMBOLS;

module_init(xsysace_init);
module_exit(xsysace_cleanup);
