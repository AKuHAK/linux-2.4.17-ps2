#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).

USE_STANDARD_AS_RULE := true

O_TARGET		:= ixp425.o

# Object file lists.

obj-y			:= arch.o  mm.o
obj-m			:=
obj-n			:=
obj-			:=

export-objs		:= ixp425-pcibuf.o ixp425-io.o ixp425-gpio.o

obj-$(CONFIG_ARCH_IXP425)	+= ixp425-irq.o ixp425-pci.o pci.o pci-auto.o \
                                   ixp425-gpio.o ixp425-io.o ixp425-pcibuf.o
				   
obj-$(CONFIG_ARCH_IXDP425)	+= ixdp425-pci.o

obj-$(CONFIG_IXP425_OS_TIMER1)	+= ixp425-time.o 

obj-$(CONFIG_KGDB) 	+= kgdb.o

include $(TOPDIR)/Rules.make
