/*
 * I/O SAPIC support.
 *
 * Copyright (C) 1999 Intel Corp.
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999-2000 Hewlett-Packard Co.
 * Copyright (C) 1999-2000 David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999,2000 Walt Drummond <drummond@valinux.com>
 *
 * 00/04/19	D. Mosberger	Rewritten to mirror more closely the x86 I/O APIC code.
 *				In particular, we now have separate handlers for edge
 *				and level triggered interrupts.
 * 00/10/27	Asit Mallick, Goutham Rao <goutham.rao@intel.com> IRQ vector allocation
 *				PCI to vector mapping, shared PCI interrupts.
 * 00/10/27	D. Mosberger	Document things a bit more to make them more understandable.
 *				Clean up much of the old IOSAPIC cruft.
 */
/*
 * Here is what the interrupt logic between a PCI device and the CPU looks like:
 *
 * (1) A PCI device raises one of the four interrupt pins (INTA, INTB, INTC, INTD).  The
 *     device is uniquely identified by its bus--, and slot-number (the function
 *     number does not matter here because all functions share the same interrupt
 *     lines).
 *
 * (2) The motherboard routes the interrupt line to a pin on a IOSAPIC controller.
 *     Multiple interrupt lines may have to share the same IOSAPIC pin (if they're level
 *     triggered and use the same polarity).  Each interrupt line has a unique IOSAPIC
 *     irq number which can be calculated as the sum of the controller's base irq number
 *     and the IOSAPIC pin number to which the line connects.
 *
 * (3) The IOSAPIC uses an internal table to map the IOSAPIC pin into the IA-64 interrupt
 *     vector.  This interrupt vector is then sent to the CPU.
 *
 * In other words, there are two levels of indirections involved:
 *
 *	pci pin -> iosapic irq -> IA-64 vector
 *
 * Note: outside this module, IA-64 vectors are called "irqs".  This is because that's
 * the traditional name Linux uses for interrupt vectors.
 */
#include <linux/config.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
#include <linux/irq.h>

#include <asm/acpi-ext.h>
#include <asm/acpikcfg.h>
#include <asm/delay.h>
#include <asm/hw_irq.h>
#include <asm/io.h>
#include <asm/iosapic.h>
#include <asm/machvec.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/system.h>


#undef DEBUG_IRQ_ROUTING

static spinlock_t iosapic_lock = SPIN_LOCK_UNLOCKED;

/* PCI pin to IOSAPIC irq routing information.  This info typically comes from ACPI. */

static struct {
	int num_routes;
	struct pci_vector_struct *route;
} pci_irq;

/* This tables maps IA-64 vectors to the IOSAPIC pin that generates this vector. */

static struct iosapic_irq {
	char *addr;			/* base address of IOSAPIC */
	unsigned char base_irq;		/* first irq assigned to this IOSAPIC */
	char pin;			/* IOSAPIC pin (-1 => not an IOSAPIC irq) */
	unsigned char dmode	: 3;	/* delivery mode (see iosapic.h) */
	unsigned char polarity	: 1;	/* interrupt polarity (see iosapic.h) */
	unsigned char trigger	: 1;	/* trigger mode (see iosapic.h) */
} iosapic_irq[IA64_NUM_VECTORS];

/*
 * Translate IOSAPIC irq number to the corresponding IA-64 interrupt vector.  If no
 * entry exists, return -1.
 */
static int
iosapic_irq_to_vector (int irq)
{
	int vector;

	for (vector = 0; vector < IA64_NUM_VECTORS; ++vector)
		if (iosapic_irq[vector].base_irq + iosapic_irq[vector].pin == irq)
			return vector;
	return -1;
}

/*
 * Map PCI pin to the corresponding IA-64 interrupt vector.  If no such mapping exists,
 * return -1.
 */
static int
pci_pin_to_vector (int bus, int slot, int pci_pin)
{
	struct pci_vector_struct *r;

	for (r = pci_irq.route; r < pci_irq.route + pci_irq.num_routes; ++r)
		if (r->bus == bus && (r->pci_id >> 16) == slot && r->pin == pci_pin)
			return iosapic_irq_to_vector(r->irq);
	return -1;
}

static void
set_rte (unsigned int vector, unsigned long dest)
{
	unsigned long pol, trigger, dmode;
	u32 low32, high32;
	char *addr;
	int pin;

	pin = iosapic_irq[vector].pin;
	if (pin < 0)
		return;		/* not an IOSAPIC interrupt */

	addr    = iosapic_irq[vector].addr;
	pol     = iosapic_irq[vector].polarity;
	trigger = iosapic_irq[vector].trigger;
	dmode   = iosapic_irq[vector].dmode;

	low32 = ((pol << IOSAPIC_POLARITY_SHIFT) |
		 (trigger << IOSAPIC_TRIGGER_SHIFT) |
		 (dmode << IOSAPIC_DELIVERY_SHIFT) |
		 vector);

	/* dest contains both id and eid */
	high32 = (dest << IOSAPIC_DEST_SHIFT);

	writel(IOSAPIC_RTE_HIGH(pin), addr + IOSAPIC_REG_SELECT);
	writel(high32, addr + IOSAPIC_WINDOW);
	writel(IOSAPIC_RTE_LOW(pin), addr + IOSAPIC_REG_SELECT);
	writel(low32, addr + IOSAPIC_WINDOW);
}

static void
nop (unsigned int vector)
{
	/* do nothing... */
}

static void
mask_irq (unsigned int irq)
{
	unsigned long flags;
	char *addr;
	u32 low32;
	int pin;
	ia64_vector vec = irq_to_vector(irq);

	addr = iosapic_irq[vec].addr;
	pin = iosapic_irq[vec].pin;

	if (pin < 0)
		return;			/* not an IOSAPIC interrupt! */

	spin_lock_irqsave(&iosapic_lock, flags);
	{
		writel(IOSAPIC_RTE_LOW(pin), addr + IOSAPIC_REG_SELECT);
		low32 = readl(addr + IOSAPIC_WINDOW);

		low32 |= (1 << IOSAPIC_MASK_SHIFT);    /* set only the mask bit */
		writel(low32, addr + IOSAPIC_WINDOW);
	}
	spin_unlock_irqrestore(&iosapic_lock, flags);
}

static void
unmask_irq (unsigned int irq)
{
	unsigned long flags;
	char *addr;
	u32 low32;
	int pin;
	ia64_vector vec = irq_to_vector(irq);

	addr = iosapic_irq[vec].addr;
	pin = iosapic_irq[vec].pin;
	if (pin < 0)
		return;			/* not an IOSAPIC interrupt! */

	spin_lock_irqsave(&iosapic_lock, flags);
	{
		writel(IOSAPIC_RTE_LOW(pin), addr + IOSAPIC_REG_SELECT);
		low32 = readl(addr + IOSAPIC_WINDOW);

		low32 &= ~(1 << IOSAPIC_MASK_SHIFT);    /* clear only the mask bit */
		writel(low32, addr + IOSAPIC_WINDOW);
	}
	spin_unlock_irqrestore(&iosapic_lock, flags);
}


static void
iosapic_set_affinity (unsigned int irq, unsigned long mask)
{
#ifdef CONFIG_SMP
	unsigned long flags;
	u32 high32, low32;
	int dest, pin;
	char *addr;

	mask &= (1UL << smp_num_cpus) - 1;

	if (!mask || irq >= IA64_NUM_VECTORS)
		return;

	dest = cpu_physical_id(ffz(~mask));

	pin = iosapic_irq[irq].pin;
	addr = iosapic_irq[irq].addr;

	if (pin < 0)
		return;			/* not an IOSAPIC interrupt */

	/* dest contains both id and eid */
	high32 = dest << IOSAPIC_DEST_SHIFT;

	spin_lock_irqsave(&iosapic_lock, flags);
	{
		/* get current delivery mode by reading the low32 */
		writel(IOSAPIC_RTE_LOW(pin), addr + IOSAPIC_REG_SELECT);
		low32 = readl(addr + IOSAPIC_WINDOW);

		/* change delivery mode to fixed */
		low32 &= ~(7 << IOSAPIC_DELIVERY_SHIFT);
		low32 |= (IOSAPIC_FIXED << IOSAPIC_DELIVERY_SHIFT);

		writel(IOSAPIC_RTE_HIGH(pin), addr + IOSAPIC_REG_SELECT);
		writel(high32, addr + IOSAPIC_WINDOW);
		writel(IOSAPIC_RTE_LOW(pin), addr + IOSAPIC_REG_SELECT);
		writel(low32, addr + IOSAPIC_WINDOW);
	}
	spin_unlock_irqrestore(&iosapic_lock, flags);
#endif
}

/*
 * Handlers for level-triggered interrupts.
 */

static unsigned int
iosapic_startup_level_irq (unsigned int irq)
{
	unmask_irq(irq);
	return 0;
}

static void
iosapic_end_level_irq (unsigned int irq)
{
	ia64_vector vec = irq_to_vector(irq);

	writel(vec, iosapic_irq[vec].addr + IOSAPIC_EOI);
}

#define iosapic_shutdown_level_irq	mask_irq
#define iosapic_enable_level_irq	unmask_irq
#define iosapic_disable_level_irq	mask_irq
#define iosapic_ack_level_irq		nop

struct hw_interrupt_type irq_type_iosapic_level = {
	typename:	"IO-SAPIC-level",
	startup:	iosapic_startup_level_irq,
	shutdown:	iosapic_shutdown_level_irq,
	enable:		iosapic_enable_level_irq,
	disable:	iosapic_disable_level_irq,
	ack:		iosapic_ack_level_irq,
	end:		iosapic_end_level_irq,
	set_affinity:	iosapic_set_affinity
};

/*
 * Handlers for edge-triggered interrupts.
 */

static unsigned int
iosapic_startup_edge_irq (unsigned int irq)
{
	unmask_irq(irq);
	/*
	 * IOSAPIC simply drops interrupts pended while the
	 * corresponding pin was masked, so we can't know if an
	 * interrupt is pending already.  Let's hope not...
	 */
	return 0;
}

static void
iosapic_ack_edge_irq (unsigned int irq)
{
	irq_desc_t *idesc = irq_desc(irq);
	/*
	 * Once we have recorded IRQ_PENDING already, we can mask the
	 * interrupt for real. This prevents IRQ storms from unhandled
	 * devices.
	 */
	if ((idesc->status & (IRQ_PENDING|IRQ_DISABLED)) == (IRQ_PENDING|IRQ_DISABLED))
		mask_irq(irq);
}

#define iosapic_enable_edge_irq		unmask_irq
#define iosapic_disable_edge_irq	nop
#define iosapic_end_edge_irq		nop

struct hw_interrupt_type irq_type_iosapic_edge = {
	typename:	"IO-SAPIC-edge",
	startup:	iosapic_startup_edge_irq,
	shutdown:	iosapic_disable_edge_irq,
	enable:		iosapic_enable_edge_irq,
	disable:	iosapic_disable_edge_irq,
	ack:		iosapic_ack_edge_irq,
	end:		iosapic_end_edge_irq,
	set_affinity:	iosapic_set_affinity
};

unsigned int
iosapic_version (char *addr)
{
	/*
	 * IOSAPIC Version Register return 32 bit structure like:
	 * {
	 *	unsigned int version   : 8;
	 *	unsigned int reserved1 : 8;
	 *	unsigned int pins      : 8;
	 *	unsigned int reserved2 : 8;
	 * }
	 */
	writel(IOSAPIC_VERSION, addr + IOSAPIC_REG_SELECT);
	return readl(IOSAPIC_WINDOW + addr);
}

/*
 * ACPI can describe IOSAPIC interrupts via static tables and namespace
 * methods.  This provides an interface to register those interrupts and
 * program the IOSAPIC RTE.
 */
int
iosapic_register_irq (u32 global_vector, unsigned long polarity, unsigned long
                      edge_triggered, u32 base_irq, char *iosapic_address)
{
	irq_desc_t *idesc;
	struct hw_interrupt_type *irq_type;
	int vector;

	vector = iosapic_irq_to_vector(global_vector);
	if (vector < 0)
		vector = ia64_alloc_irq();

	/* fill in information from this vector's IOSAPIC */
	iosapic_irq[vector].addr = iosapic_address;
	iosapic_irq[vector].base_irq = base_irq;
	iosapic_irq[vector].pin	= global_vector - iosapic_irq[vector].base_irq;
	iosapic_irq[vector].polarity = polarity ? IOSAPIC_POL_HIGH : IOSAPIC_POL_LOW;
	iosapic_irq[vector].dmode    = IOSAPIC_LOWEST_PRIORITY;

	if (edge_triggered) {
		iosapic_irq[vector].trigger = IOSAPIC_EDGE;
		irq_type = &irq_type_iosapic_edge;
	} else {
		iosapic_irq[vector].trigger = IOSAPIC_LEVEL;
		irq_type = &irq_type_iosapic_level;
	}

	idesc = irq_desc(vector);
	if (idesc->handler != irq_type) {
		if (idesc->handler != &no_irq_type)
			printk("iosapic_register_irq(): changing vector 0x%02x from"
			       "%s to %s\n", vector, idesc->handler->typename, irq_type->typename);
		idesc->handler = irq_type;
	}

	printk("IOSAPIC %x(%s,%s) -> Vector %x\n", global_vector,
	       (polarity ? "high" : "low"), (edge_triggered ? "edge" : "level"), vector);

	/* program the IOSAPIC routing table */
	set_rte(vector, (ia64_get_lid() >> 16) & 0xffff);
	return vector;
}

/*
 * ACPI calls this when it finds an entry for a platform interrupt.
 * Note that the irq_base and IOSAPIC address must be set in iosapic_init().
 */
int
iosapic_register_platform_irq (u32 int_type, u32 global_vector, u32 iosapic_vector,
			       u16 eid, u16 id, unsigned long polarity,
			       unsigned long edge_triggered, u32 base_irq, char *iosapic_address)
{
	struct hw_interrupt_type *irq_type;
	irq_desc_t *idesc;
	int vector;

	switch (int_type) {
	      case ACPI20_ENTRY_PIS_CPEI:
		vector = IA64_PCE_VECTOR;
		iosapic_irq[vector].dmode = IOSAPIC_LOWEST_PRIORITY;
		break;
	      case ACPI20_ENTRY_PIS_INIT:
		vector = ia64_alloc_irq();
		iosapic_irq[vector].dmode = IOSAPIC_INIT;
		break;
	      default:
		printk("iosapic_register_platform_irq(): invalid int type\n");
		return -1;
	}

	/* fill in information from this vector's IOSAPIC */
	iosapic_irq[vector].addr = iosapic_address;
	iosapic_irq[vector].base_irq = base_irq;
	iosapic_irq[vector].pin	= global_vector - iosapic_irq[vector].base_irq;
	iosapic_irq[vector].polarity = polarity ? IOSAPIC_POL_HIGH : IOSAPIC_POL_LOW;

	if (edge_triggered) {
		iosapic_irq[vector].trigger = IOSAPIC_EDGE;
		irq_type = &irq_type_iosapic_edge;
	} else {
		iosapic_irq[vector].trigger = IOSAPIC_LEVEL;
		irq_type = &irq_type_iosapic_level;
	}

	idesc = irq_desc(vector);
	if (idesc->handler != irq_type) {
		if (idesc->handler != &no_irq_type)
			printk("iosapic_register_platform_irq(): changing vector 0x%02x from"
			       "%s to %s\n", vector, idesc->handler->typename, irq_type->typename);
		idesc->handler = irq_type;
	}

	printk("PLATFORM int %x: IOSAPIC %x(%s,%s) -> Vector %x CPU %.02u:%.02u\n",
	       int_type, global_vector, (polarity ? "high" : "low"),
	       (edge_triggered ? "edge" : "level"), vector, eid, id);

	/* program the IOSAPIC routing table */
	set_rte(vector, ((id << 8) | eid) & 0xffff);
	return vector;
}


/*
 * ACPI calls this when it finds an entry for a legacy ISA interrupt.  Note that the
 * irq_base and IOSAPIC address must be set in iosapic_init().
 */
void
iosapic_register_legacy_irq (unsigned long irq,
			     unsigned long pin, unsigned long polarity,
			     unsigned long edge_triggered)
{
	unsigned int vector = isa_irq_to_vector(irq);

#ifdef DEBUG_IRQ_ROUTING
	printk("ISA: IRQ %u -> IOSAPIC irq 0x%02x (%s, %s) -> vector %02x\n",
	       (unsigned) irq, (unsigned) pin,
	       polarity ? "high" : "low", edge_triggered ? "edge" : "level",
	       vector);
#endif

	iosapic_irq[vector].pin = pin;
	iosapic_irq[vector].dmode = IOSAPIC_LOWEST_PRIORITY;
	iosapic_irq[vector].polarity = polarity ? IOSAPIC_POL_HIGH : IOSAPIC_POL_LOW;
	iosapic_irq[vector].trigger = edge_triggered ? IOSAPIC_EDGE : IOSAPIC_LEVEL;
}

void __init
iosapic_init (unsigned long phys_addr, unsigned int base_irq, int pcat_compat)
{
	struct hw_interrupt_type *irq_type;
	int i, irq, max_pin, vector;
	irq_desc_t *idesc;
	unsigned int ver;
	char *addr;
	static int first_time = 1;

	if (first_time) {
		first_time = 0;

		for (vector = 0; vector < IA64_NUM_VECTORS; ++vector)
			iosapic_irq[vector].pin = -1;	/* mark as unused */

		/*
		 * Fetch the PCI interrupt routing table:
		 */
		acpi_cf_get_pci_vectors(&pci_irq.route, &pci_irq.num_routes);
	}

	addr = ioremap(phys_addr, 0);

	ver = iosapic_version(addr);
	max_pin = (ver >> 16) & 0xff;

	printk("IOSAPIC: version %x.%x, address 0x%lx, IRQs 0x%02x-0x%02x\n",
	       (ver & 0xf0) >> 4, (ver & 0x0f), phys_addr, base_irq, base_irq + max_pin);

	if ((base_irq == 0) && pcat_compat)
		/*
		 * Map the legacy ISA devices into the IOSAPIC data.  Some of these may
		 * get reprogrammed later on with data from the ACPI Interrupt Source
		 * Override table.
		 */
		for (irq = 0; irq < 16; ++irq) {
			vector = isa_irq_to_vector(irq);
			iosapic_irq[vector].addr = addr;
			iosapic_irq[vector].base_irq = 0;
			if (iosapic_irq[vector].pin == -1)
				iosapic_irq[vector].pin = irq;
			iosapic_irq[vector].dmode = IOSAPIC_LOWEST_PRIORITY;
			iosapic_irq[vector].trigger  = IOSAPIC_EDGE;
			iosapic_irq[vector].polarity = IOSAPIC_POL_HIGH;
#ifdef DEBUG_IRQ_ROUTING
			printk("ISA: IRQ %u -> IOSAPIC irq 0x%02x (high, edge) -> vector 0x%02x\n",
			       irq, iosapic_irq[vector].base_irq + iosapic_irq[vector].pin,
			       vector);
#endif
			irq_type = &irq_type_iosapic_edge;
			idesc = irq_desc(vector);
			if (idesc->handler != irq_type) {
				if (idesc->handler != &no_irq_type)
					printk("iosapic_init: changing vector 0x%02x from %s to "
					       "%s\n", irq, idesc->handler->typename,
					       irq_type->typename);
				idesc->handler = irq_type;
			}

			/* program the IOSAPIC routing table: */
			set_rte(vector, (ia64_get_lid() >> 16) & 0xffff);
		}

	for (i = 0; i < pci_irq.num_routes; i++) {
		irq = pci_irq.route[i].irq;

		if ((unsigned) (irq - base_irq) > max_pin)
			/* the interrupt route is for another controller... */
			continue;

		if (pcat_compat && (irq < 16))
			vector = isa_irq_to_vector(irq);
		else {
			vector = iosapic_irq_to_vector(irq);
			if (vector < 0)
				/* new iosapic irq: allocate a vector for it */
				vector = ia64_alloc_irq();
		}

		iosapic_irq[vector].addr     = addr;
		iosapic_irq[vector].base_irq = base_irq;
		iosapic_irq[vector].pin	     = (irq - base_irq);
		iosapic_irq[vector].dmode    = IOSAPIC_LOWEST_PRIORITY;
		iosapic_irq[vector].trigger  = IOSAPIC_LEVEL;
		iosapic_irq[vector].polarity = IOSAPIC_POL_LOW;

# ifdef DEBUG_IRQ_ROUTING
		printk("PCI: (B%d,I%d,P%d) -> IOSAPIC irq 0x%02x -> vector 0x%02x\n",
		       pci_irq.route[i].bus, pci_irq.route[i].pci_id>>16, pci_irq.route[i].pin,
		       iosapic_irq[vector].base_irq + iosapic_irq[vector].pin, vector);
# endif
		irq_type = &irq_type_iosapic_level;
		idesc = irq_desc(vector);
		if (idesc->handler != irq_type){
			if (idesc->handler != &no_irq_type)
				printk("iosapic_init: changing vector 0x%02x from %s to %s\n",
				       vector, idesc->handler->typename, irq_type->typename);
			idesc->handler = irq_type;
		}

		/* program the IOSAPIC routing table: */
		set_rte(vector, (ia64_get_lid() >> 16) & 0xffff);
	}
}

void
iosapic_pci_fixup (int phase)
{
	struct	pci_dev	*dev;
	unsigned char pin;
	int vector;

	if (phase != 1)
		return;

	pci_for_each_dev(dev) {
		pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &pin);
		if (pin) {
			pin--;          /* interrupt pins are numbered starting from 1 */
			vector = pci_pin_to_vector(dev->bus->number, PCI_SLOT(dev->devfn), pin);
			if (vector < 0 && dev->bus->parent) {
				/* go back to the bridge */
				struct pci_dev *bridge = dev->bus->self;

				if (bridge) {
					/* allow for multiple bridges on an adapter */
					do {
						/* do the bridge swizzle... */
						pin = (pin + PCI_SLOT(dev->devfn)) % 4;
						vector = pci_pin_to_vector(bridge->bus->number,
									   PCI_SLOT(bridge->devfn),
									   pin);
					} while (vector < 0 && (bridge = bridge->bus->self));
				}
				if (vector >= 0)
					printk(KERN_WARNING
					       "PCI: using PPB(B%d,I%d,P%d) to get vector %02x\n",
					       bridge->bus->number, PCI_SLOT(bridge->devfn),
					       pin, vector);
				else
					printk(KERN_WARNING
					       "PCI: Couldn't map irq for (B%d,I%d,P%d)o\n",
					       bridge->bus->number, PCI_SLOT(bridge->devfn),
					       pin);
			}
			if (vector >= 0) {
				printk("PCI->APIC IRQ transform: (B%d,I%d,P%d) -> 0x%02x\n",
				       dev->bus->number, PCI_SLOT(dev->devfn), pin, vector);
				dev->irq = vector;

#ifdef CONFIG_SMP
				/*
				 * For platforms that do not support interrupt redirect
				 * via the XTP interface, we can round-robin the PCI
				 * device interrupts to the processors
				 */
				if (!(smp_int_redirect & SMP_IRQ_REDIRECTION)) {
					static int cpu_index = 0;

					set_rte(vector, cpu_physical_id(cpu_index) & 0xffff);

					cpu_index++;
					if (cpu_index >= smp_num_cpus)
						cpu_index = 0;
				}
#endif
			}
		}
		/*
		 * Nothing to fixup
		 * Fix out-of-range IRQ numbers
		 */
		if (dev->irq >= IA64_NUM_VECTORS)
			dev->irq = 15;	/* Spurious interrupts */
	}
}
