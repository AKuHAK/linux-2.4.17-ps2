/*
 * linux/drivers/char/rtc_ds1742.c
 *
 * driver for Dallas DS1742 RTC
 *
 * Author: MontaVista Software, Inc.
 *         source@mvista.com
 *
 * Copyright 2001-2002 MontaVista Software Inc.
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

#include <linux/rtc_ds1742.h>

unsigned long rtc_ds1742_base;

#define RTC_DS1742_DEBUG
#undef  RTC_DS1742_DEBUG2
#undef  RTC_DS1742_FORCE_INIT
#undef  RTC_DS1742_TEST        

#undef BCD_TO_BIN
#define BCD_TO_BIN(val) (((val)&15) + ((val)>>4)*10)

#undef BIN_TO_BCD
#define BIN_TO_BCD(val) ((((val)/10)<<4) + (val)%10)

#ifdef RTC_DS1742_DEBUG
static void
rtc_ds1742_dump_reg( u32 reg, u32 mask, char* key, char* name )
{
  u8 data_raw;
  u8 data_bcd;
  u8 data_bin;
  
  data_raw = RTC_DS1742_RD( reg );
  data_bcd = data_raw & mask;
  data_bin = BCD_TO_BIN( data_bcd );

  printk( "rtc_ds1742_dump_reg() %s:%s raw=0x%02x/%03d bcd=0x%02x/%03d bin=0x%02x/%03d\n",
          key, name, data_raw, data_raw, data_bcd, data_bcd, data_bin, data_bin );

  return;
}
#endif


#ifdef RTC_DS1742_DEBUG
static void
rtc_ds1742_dump_regs( char* key )
{
  printk( "rtc_ds1742_dump_reg() %s-\n", key );
  
  rtc_ds1742_dump_reg( RTC_DS1742_SECOND,  RTC_DS1742_MASK_SECOND_OSC, key, "EN"  );
  rtc_ds1742_dump_reg( RTC_DS1742_DAY,     RTC_DS1742_MASK_DAY_BF,     key, "BF"  );
  rtc_ds1742_dump_reg( RTC_DS1742_DAY,     RTC_DS1742_MASK_DAY_FT,     key, "FT"  );
  rtc_ds1742_dump_reg( RTC_DS1742_CENTURY, RTC_DS1742_MASK_CENTURY_WR, key, "WR"  );
  rtc_ds1742_dump_reg( RTC_DS1742_CENTURY, RTC_DS1742_MASK_CENTURY_RD, key, "RD"  );

  rtc_ds1742_dump_reg( RTC_DS1742_CENTURY, RTC_DS1742_MASK_CENTURY,    key, "CN"  );
  rtc_ds1742_dump_reg( RTC_DS1742_YEAR,    RTC_DS1742_MASK_YEAR,       key, "YR"  );
  rtc_ds1742_dump_reg( RTC_DS1742_MONTH,   RTC_DS1742_MASK_MONTH,      key, "MO"  );
  rtc_ds1742_dump_reg( RTC_DS1742_DATE,    RTC_DS1742_MASK_DATE,       key, "DT"  );
  rtc_ds1742_dump_reg( RTC_DS1742_DAY,     RTC_DS1742_MASK_DAY,        key, "DY"  );
  rtc_ds1742_dump_reg( RTC_DS1742_HOUR,    RTC_DS1742_MASK_HOUR,       key, "HR"  );
  rtc_ds1742_dump_reg( RTC_DS1742_MINUTE,  RTC_DS1742_MASK_MINUTE,     key, "MN"  );
  rtc_ds1742_dump_reg( RTC_DS1742_SECOND,  RTC_DS1742_MASK_SECOND,     key, "SC"  );

  printk( "rtc_ds1742_dump_reg() %s+\n", key );

  return;
}
#endif


unsigned long
rtc_ds1742_get_time( void )
{
  u8 orig;
  u32 century, year, month, day, hour, minute, second;
  unsigned long t;

  /* freeze user register updates */
  orig = rtc_ds1742_atomic_rd_lock_beg();

  /* read time data */ 
  century = (u32)rtc_ds1742_rd_century();
  year    = (u32)rtc_ds1742_rd_year();
  month   = (u32)rtc_ds1742_rd_month();
  day     = (u32)rtc_ds1742_rd_date();
  hour    = (u32)rtc_ds1742_rd_hour();
  minute  = (u32)rtc_ds1742_rd_minute();
  second  = (u32)rtc_ds1742_rd_second();

  /* unfreeze user register updates */
  rtc_ds1742_atomic_rd_lock_end( orig );

  #ifdef RTC_DS1742_DEBUG2
  {
    rtc_ds1742_dump_regs( "ds1742_get" );
  }
  #endif

  /* mktime( YYYY, MM=1-12, DAY=1-31, HOUR=0-23, MIN=0-59, SEC=0-59 ); */
  t = mktime( (century*100)+year, month, day, hour, minute, second );
  return( t );
}


int 
rtc_ds1742_set_time( unsigned long t ) 
{
  u8 orig;
  struct rtc_time tm;
  unsigned long the_time; 

  /* to_tm( YYYY, MM=0-11, DAY=1-31, HOUR=0-23, MIN=0-59, SEC=0-59 ); */
  to_tm(t, &tm);

  /* freeze user register updates */
  orig = rtc_ds1742_atomic_wr_lock_beg();

  /* write time data */ 
  rtc_ds1742_wr_second(  tm.tm_sec      );
  rtc_ds1742_wr_minute(  tm.tm_min      );
  rtc_ds1742_wr_hour(    tm.tm_hour     );
  rtc_ds1742_wr_day(     tm.tm_wday+1   );
  rtc_ds1742_wr_date(    tm.tm_mday     );
  rtc_ds1742_wr_month(   tm.tm_mon+1    );
  rtc_ds1742_wr_year(    tm.tm_year%100 );
  rtc_ds1742_wr_century( tm.tm_year/100 );

  /* unfreeze user register updates */
  rtc_ds1742_atomic_wr_lock_end( orig );

  #ifdef RTC_DS1742_DEBUG2
  {
    rtc_ds1742_dump_regs( "ds1742_set" );
  }
  #endif

  the_time = rtc_ds1742_get_time();

  return( the_time );
}


void __init
rtc_ds1742_init( unsigned long base )
{
  u8 data;

  /* remember the base */
  rtc_ds1742_base = base;


  /* some f/w don't enable the chip properly */
  #ifdef RTC_DS1742_FORCE_INIT
  {
    printk( "rtc_ds1742_init() -- Aborting stale write (time may be bogus)\n" );
    data = rtc_ds1742_atomic_wr_lock_beg();
    rtc_ds1742_atomic_wr_lock_end(data);

    printk( "rtc_ds1742_init() -- Aborting stale read\n" );
    data = rtc_ds1742_atomic_rd_lock_beg();
    rtc_ds1742_atomic_rd_lock_end(data);

    printk( "rtc_ds1742_init() -- Forcing osscilator on (time may be bogus)\n" );
    data = rtc_ds1742_atomic_wr_lock_beg();
    RTC_DS1742_CLR( RTC_DS1742_SECOND, RTC_DS1742_MASK_SECOND_OSC );
    rtc_ds1742_atomic_wr_lock_end(data);

    printk( "rtc_ds1742_init() -- Aborting frequency test (time may be bogus)\n" );
    data = rtc_ds1742_atomic_wr_lock_beg();
    RTC_DS1742_CLR( RTC_DS1742_DAY, RTC_DS1742_MASK_DAY_FT );
    rtc_ds1742_atomic_wr_lock_end(data);
  }
  #endif


  /* require fw to leave OSC=0 */
  #ifdef RTC_DS1742_DEBUG
  {
    data  = RTC_DS1742_RD( RTC_DS1742_SECOND );
    data &= RTC_DS1742_MASK_SECOND_OSC;
    if ( data )
    {
      printk( "rtc_ds1742_init() -- osscilator disabled (rtc will not update)\n" );
    }
    else
    {
      printk( "rtc_ds1742_init() -- osscilator is running\n" );
    }
  }
  #endif


  /* require fw to leave R=0, W=0 */
  #ifdef RTC_DS1742_DEBUG
  {
    data  = RTC_DS1742_RD( RTC_DS1742_CENTURY );
    data &= (RTC_DS1742_MASK_CENTURY_WR|RTC_DS1742_MASK_CENTURY_RD);
    if ( data )
    {
      if ( data & RTC_DS1742_MASK_CENTURY_WR )
      {
        printk( "rtc_ds1742_init() -- unfinished write (rtc will not update)\n" );
      }
      if ( data & RTC_DS1742_MASK_CENTURY_RD )
      {
        printk( "rtc_ds1742_init() -- unfinished read (rtc will not update)\n" );
      }
    }
    else
    {
      printk( "rtc_ds1742_init() -- user registers are updating from osscilator\n" );
    }
  }
  #endif


  #ifdef RTC_DS1742_DEBUG
  {
    data = RTC_DS1742_RD( RTC_DS1742_DAY );
    data &= RTC_DS1742_MASK_DAY_BF;
    if ( data )
    {
      printk( "rtc_ds1742_init() -- battery flag indicates normal\n" );
    }
    else
    {
      printk( "rtc_ds1742_init() -- battery flag indicates low voltage\n" );
    }
  }
  #endif


  #ifdef RTC_DS1742_DEBUG
  {
    data = RTC_DS1742_RD( RTC_DS1742_DAY );
    data &= RTC_DS1742_MASK_DAY_FT;
    if ( data )
    {
      printk( "rtc_ds1742_init() -- frequency test enabled (bad for normal ops)\n" );
    }
    else
    {
      printk( "rtc_ds1742_init() -- frequency test disabled (good for normal ops)\n" );
    }
  }
  #endif


  #ifdef RTC_DS1742_DEBUG
  {
    printk( "rtc_ds1742_init() time is %lu\n", rtc_ds1742_get_time() );
  }
  #endif


  #ifdef RTC_DS1742_TEST  
  {
    int i;
    unsigned long t0;
    unsigned long t1;
    unsigned long t2;
    unsigned long t3;

    for ( i = 0; i < 10; i++ )
    {
      printk( "rtc_ds1742_init() t[%d] = %lu\n", i, rtc_ds1742_get_time() );
      rtc_ds1742_wait();
    }

    t0 = rtc_ds1742_get_time();

    t1 = rtc_ds1742_set_time( 0 );

    t2 = rtc_ds1742_set_time( ( t0 / 1000 ) * 1000 );

    t3 = rtc_ds1742_set_time( t0 + 3 );

    printk( "rtc_ds1742_init() test t0=%lu\n", t0 );
    printk( "rtc_ds1742_init() test t1=%lu\n", t1 );
    printk( "rtc_ds1742_init() test t2=%lu\n", t2 );
    printk( "rtc_ds1742_init() test t3=%lu\n", t3 );
  }
  #endif

  return;
}
