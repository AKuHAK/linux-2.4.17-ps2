/*
 * include/asm-arm/arch-ixp425/preem-latency.h
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 *
 * Copyright 2003 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <asm/hardware.h>

/*
 * IXP425 has a Time Stamp counter which is always incrementing 
 * every core clock cycle and is not user resetable.  Perfect 
 * for this purpose. Clock runs at 66Mhz, so we can measure about
 * 65s.
 */
#define readclock_init() 

#define readclock(x) do { (x) = *IXP425_OSTS }; while (0)
#define	clock_to_usecs(x) ((x) / 66)

