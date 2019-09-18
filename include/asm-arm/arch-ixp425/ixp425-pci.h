/*
 * ixp425-pci.h 
 *
 *  Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _ASM_ARCH_IXP425_PCI_H_
#define _ASM_ARCH_IXP425_PCI_H_

#define NP_CMD_IOREAD                           0x2
#define NP_CMD_IOWRITE                          0x3
#define NP_CMD_CONFIGREAD                       0xa
#define NP_CMD_CONFIGWRITE                      0xb

#define IXP425_PCI_BOTTOM_NIBBLE_OF_LONG_MASK   0x0000000f
#define IXP425_PCI_NP_CBE_BESL                  4

extern int ixp425_pci_read(u32 addr, u32 cmd, u32* data);
extern int ixp425_pci_write(u32 addr, u32 cmd, u32 data);
extern void ixp425_pci_init(void *sysdata);


#endif

