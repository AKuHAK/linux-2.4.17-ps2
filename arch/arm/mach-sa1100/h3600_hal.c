/*
* Hardware abstraction layer for Compaq iPAQ H3xxx Pocket Computers
*
* Copyright 2000,2001 Compaq Computer Corporation.
*
* Use consistent with the GNU GPL is permitted,
* provided that this copyright notice is
* preserved in its entirety in all copies and derived works.
*
* COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
* AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
* FITNESS FOR ANY PARTICULAR PURPOSE.
*
* Andrew Christian
* October, 2001
*/

#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <asm/uaccess.h>        /* get_user,copy_to_user */
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/sysctl.h>
#include <linux/console.h>
#include <linux/devfs_fs_kernel.h>

#include <linux/tqueue.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/proc_fs.h>
#include <linux/apm_bios.h>
#include <linux/kmod.h>

#include <asm/hardware.h>
#include <asm/arch/h3600_hal.h>

#include "generic.h"

#define H3600_HAL_PROC_DIR     "hal"

/* Interface supplied by the low-level microcontroller driver */
struct h3600_hal_ops    *h3600_hal_ops = NULL;
EXPORT_SYMBOL(h3600_hal_ops);

static struct h3600_driver_ops g_driver_ops;

/* Global values */
static unsigned char  flite_brightness          = 25; 
static int            max_flite_brightness      = 255;
static enum flite_pwr flite_power               = FLITE_PWR_ON;
static int            lcd_active                = 1;
static unsigned char  screen_contrast           = 100;

/* Parameters */

MODULE_PARM(flite_brightness,"b");
MODULE_PARM_DESC(flite_brightness,"Initial brightness setting of the frontlight");
MODULE_PARM(max_flite_brightness,"i");
MODULE_PARM_DESC(max_flite_brightness,"Maximum allowable brightness setting of the frontlight");
MODULE_PARM(flite_power,"i");
MODULE_PARM_DESC(flite_power,"Initial power setting of the frontlight (on/off)");
MODULE_PARM(screen_contrast,"i");
MODULE_PARM_DESC(screen_contrast,"Initial screen contrast (for H3100 only)");

MODULE_AUTHOR("Andrew Christian");
MODULE_DESCRIPTION("Hardware abstraction layer for the iPAQ H3600");

/***********************************************************************************/
/*   General callbacks                                                             */
/***********************************************************************************/

int h3600_set_flite(enum flite_pwr pwr, unsigned char brightness)
{
        if (0) printk("###" __FUNCTION__ ": pwr=%d brightness=%d lcdactive=%d\n", 
		      pwr, brightness, lcd_active );

	if ( brightness > max_flite_brightness )
		brightness = max_flite_brightness;

	/* Save the current settings */
	flite_power        = pwr;
	flite_brightness   = brightness;

	return CALL_HAL_INTERFACE( backlight_control, 
				   (lcd_active ? flite_power : FLITE_PWR_OFF), 
				   (pwr == FLITE_PWR_ON && lcd_active ? flite_brightness : 0) );
}

void h3600_get_flite( struct h3600_ts_backlight *bl )
{
	bl->power      = flite_power;
	bl->brightness = flite_brightness;
}

int h3600_set_contrast( unsigned char contrast )
{
	if (0) printk(__FUNCTION__ ": contrast=%d\n", contrast );

	screen_contrast = contrast;
	return CALL_HAL_INTERFACE( contrast_control, screen_contrast );
}

void h3600_get_contrast( u_char *contrast )
{
	*contrast = screen_contrast;
}

int h3600_toggle_frontlight( void )
{
	return h3600_set_flite( 1 - flite_power, flite_brightness );
}

EXPORT_SYMBOL(h3600_get_flite);
EXPORT_SYMBOL(h3600_set_flite);
EXPORT_SYMBOL(h3600_get_contrast);
EXPORT_SYMBOL(h3600_set_contrast);
EXPORT_SYMBOL(h3600_toggle_frontlight);

/* This function is called by sa1100fb to turn the backlight on and off.
   There are several reasons it can be called:

      1. The framebuffer is being shut down from a suspend/resume.
      2. Someone like an X server has requested blanking

   On a blank request, we shut off the backlight.  On an unblank request,
   we restore the backlight to its prior setting.
*/

static void h3600_hal_backlight_helper(int on)
{
	lcd_active = on;
	h3600_set_flite( flite_power, flite_brightness );
}

void h3600_hal_keypress( unsigned char key )
{
	if ( g_driver_ops.keypress )
		g_driver_ops.keypress(key);
}

void h3600_hal_touchpanel( unsigned short x, unsigned short y, int down )
{
	if ( g_driver_ops.touchpanel )
		g_driver_ops.touchpanel(x,y,down);
}

void h3600_hal_option_detect( int present )
{
	if ( g_driver_ops.option_detect )
		g_driver_ops.option_detect(present);
}

EXPORT_SYMBOL(h3600_hal_keypress);
EXPORT_SYMBOL(h3600_hal_touchpanel);
EXPORT_SYMBOL(h3600_hal_option_detect);

/***********************************************************************************/
/*      Functions exported for use by the kernel and kernel modules                */
/***********************************************************************************/

int h3600_apm_get_power_status(u_char *ac_line_status,
			       u_char *battery_status, 
			       u_char *battery_flag, 
			       u_char *battery_percentage, 
			       u_short *battery_life)
{
	struct h3600_battery bstat;
	int result;

	result = h3600_get_battery(&bstat);
	if (result) {
		printk(__FUNCTION__ ": unable to access battery information\n");
		return 0;
	}

	/* now translate result into APM terms */
	if (ac_line_status != NULL) {
		u_char stat = APM_AC_UNKNOWN;
		switch (bstat.ac_status) {
		case H3600_AC_STATUS_AC_OFFLINE:
			stat = APM_AC_OFFLINE;
			break;
		case H3600_AC_STATUS_AC_ONLINE:
			stat = APM_AC_ONLINE;
			break;
		case H3600_AC_STATUS_AC_BACKUP:
			stat = APM_AC_BACKUP;
			break;
		}
		*ac_line_status = stat;
	}
	if (battery_status != NULL) {
		u_char stat = APM_BATTERY_STATUS_UNKNOWN;
		switch (bstat.battery[0].status) {
		case H3600_BATT_STATUS_HIGH:
			stat = APM_BATTERY_STATUS_HIGH;
			break;
		case H3600_BATT_STATUS_LOW:
			stat = APM_BATTERY_STATUS_LOW;
			break;
		case H3600_BATT_STATUS_CRITICAL:
			stat = APM_BATTERY_STATUS_CRITICAL;
			break;
		case H3600_BATT_STATUS_CHARGING:
			stat = APM_BATTERY_STATUS_CHARGING;
			break;
		case H3600_BATT_STATUS_NOBATT:
			stat = APM_BATTERY_STATUS_UNKNOWN;
			break;
		}
		*battery_status = stat;
	}
	if (battery_percentage != NULL) {
		/* 100% time left at 935 raw, 5% at 830, 0% at 701
		 * However, when charging, stops at 960 (goes into
		 * slow-charging mode, I guess), but that's > 935.
		 * Finally, when online, but not charging, must be full.
		 * See also
		 * http://handhelds.org/z/wiki/Kero%20van%20Gelder
		 */
		int raw = bstat.battery[0].voltage;
		int percentage;

		if (bstat.battery[0].status & H3600_BATT_STATUS_CHARGING)
			raw -= 30;
		if (raw > 830)
			percentage = (raw - 830) * 95 / 105 + 5;
		else
			percentage = (raw - 711) * 5 / 129;
		if ((bstat.ac_status & H3600_AC_STATUS_AC_ONLINE) 
		    && !(bstat.battery[0].status & H3600_BATT_STATUS_CHARGING))
			percentage = 100;
		if (percentage > 100) percentage = 100;
		*battery_percentage = percentage;

		/* assuming C/5 discharge rate */
		if (battery_life != NULL) {
			*battery_life = 300 * percentage / 100;
			*battery_life |= 0x8000;   /* Flag for minutes */
		}
	}
                        
	return 1;
}

EXPORT_SYMBOL(h3600_apm_get_power_status);

/***********************************************************************************/
/*   Proc filesystem interface                                                     */
/***********************************************************************************/

static struct ctl_table h3600_hal_table[] = 
{
	{3, "max_flite_brightness", &max_flite_brightness, sizeof(max_flite_brightness), 
	 0666, NULL, &proc_dointvec},
	{0}
};

static struct ctl_table h3600_hal_dir_table[] =
{
	{11, "hal", NULL, 0, 0555, h3600_hal_table},
	{0}
};

static struct ctl_table_header *h3600_hal_sysctl_header = NULL;
static struct proc_dir_entry   *hal_proc_dir = NULL;

enum hal_proc_index {
	HAL_DONE,
	HAL_VERSION,
	HAL_THERMAL_SENSOR,
	HAL_BATTERY,
	HAL_LIGHT_SENSOR,
	HAL_ASSETS,
	HAL_MODEL
};

struct h3600_proc_item {
	const char *name;
	enum hal_proc_index id;
};

static struct h3600_proc_item g_procitems[] = {
	{"version",      HAL_VERSION        },
	{"thermal",      HAL_THERMAL_SENSOR },
	{"battery",      HAL_BATTERY        },
	{"light_sensor", HAL_LIGHT_SENSOR   },
	{"assets",       HAL_ASSETS         },
	{"model",        HAL_MODEL          },
	{"done",         HAL_DONE           },
};

struct battery_flag_name {
	unsigned char bits;
	char         *name;
};

static struct battery_flag_name battery_chemistry[] = {
	{ H3600_BATT_CHEM_ALKALINE, "alkaline" },
	{ H3600_BATT_CHEM_NICD,     "NiCd" },
	{ H3600_BATT_CHEM_NIMH,     "NiMH" },
	{ H3600_BATT_CHEM_LION,     "Li-ion" },
	{ H3600_BATT_CHEM_LIPOLY,   "Li-Polymer" },
	{ H3600_BATT_CHEM_UNKNOWN,  "unknown" },
	{ 0, "?" }
};

static struct battery_flag_name ac_status[] = {
	{ H3600_AC_STATUS_AC_OFFLINE, "offline" },
	{ H3600_AC_STATUS_AC_ONLINE,  "online" },
	{ H3600_AC_STATUS_AC_BACKUP,  "backup" },
	{ H3600_AC_STATUS_AC_UNKNOWN, "unknown" },
	{ 0, "?" }
};

/* TODO : I believe we're using the "or'd" version of these
   flags */

static struct battery_flag_name battery_status[] = {
	{ H3600_BATT_STATUS_HIGH,     "high" },
	{ H3600_BATT_STATUS_LOW,      "low" },
	{ H3600_BATT_STATUS_CRITICAL, "critical" },
	{ H3600_BATT_STATUS_CHARGING, "charging" },
	{ H3600_BATT_STATUS_NOBATT,   "no battery" },
	{ H3600_BATT_STATUS_UNKNOWN,  "unknown" },
	{ 0, "?" }
};

static char * extract_flag_name( struct battery_flag_name *list, unsigned char value )
{
	while ( list->bits && list->bits != value )
		list++;
	return list->name;
}

enum params_data {
	PARAMS_DONE = 0,
	PARAMS_TCHAR,
	PARAMS_WORD,
	PARAMS_LONG,
};

struct params_list {
	int   value;
	char *name;
};

struct params_table {
	int                 offset;
	enum params_data    type;
	int                 length;
	char *              name;
	struct params_list *list;
};

static struct params_list product_id[] = {
	{ 2, "Palm" },
	{ -1, "" },
};

static struct params_list page_mode[] = {
	{ 0, "Flash" },
	{ 1, "ROM" },
	{ -1, "" },
};

/* These are guesses */
static struct params_list country_id[] = {
	{ 0, "USA" },
	{ 1, "English" },
	{ 2, "German" },
	{ 3, "French" },
	{ 4, "Italian" },
	{ 5, "Spanish" },
	{ 0x8001, "Traditional Chinese" },
	{ 0x8002, "Simplified Chinese" },
	{ 0x8003, "Japanese" },
	{ -1, "" },
};

static struct params_table params_table[] = {
	{ 0, PARAMS_TCHAR, 5,  "HM Version" },
	{ 10, PARAMS_TCHAR, 20, "Serial #" },
	{ 50, PARAMS_TCHAR, 10, "Module ID" },
	{ 70, PARAMS_TCHAR, 5,  "Product Revision" },
	{ 80, PARAMS_WORD,  0,  "Product ID", product_id },
	{ 82, PARAMS_WORD,  0,  "Frame Rate" },
	{ 84, PARAMS_WORD,  0,  "Page Mode", page_mode },
	{ 86, PARAMS_WORD,  0,  "Country ID", country_id },
	{ 88, PARAMS_WORD,  0,  "Is Color Display" },
	{ 90, PARAMS_WORD,  0,  "ROM Size" },
	{ 92, PARAMS_WORD,  0,  "RAM Size" },
	{ 94, PARAMS_WORD,  0,  "Horizontal pixels" },
	{ 96, PARAMS_WORD,  0,  "Vertical pixels" },
	{ 0,  PARAMS_DONE, 0, NULL  }
};

#define BASIC_FORMAT "%20s : "
#define ATMEL_FLASH_LEN 256

static unsigned int read_word( char *p )
{
	unsigned int v = *p++;
	v <<= 8;
	return v | *p;
}

static char * lookup_params( struct params_list *list, int value )
{
	if ( !list )
		return NULL;

	while ( list->value != -1 && list->value != value )
		list++;
	return list->name;
}

static char * h3600_hal_parse_eeprom( char *p )
{
	unsigned char data[ATMEL_FLASH_LEN]; 
	struct params_table *t = params_table;
	int retval;

	/* Suck in the data */
	retval = h3600_eeprom_read( 0, data, ATMEL_FLASH_LEN );
	if ( retval ) {
		p += sprintf(p,"Error value %d\n", retval);
		return p;
	}

	for ( ; t->type != PARAMS_DONE ; t++ ) {
		char *data_ptr = &data[ t->offset ];

		switch ( t->type ) {
		case PARAMS_WORD: {
			unsigned short v = read_word( data_ptr );
			char *param;

			param = lookup_params(t->list,v);

			if ( param )
				p += sprintf(p, BASIC_FORMAT "%d (%s)\n", t->name, v, param );
			else
				p += sprintf(p, BASIC_FORMAT "%d\n", t->name, v);
			break;
		}
		case PARAMS_LONG: {
			unsigned int v = read_word( data_ptr );
			unsigned int w = read_word( data_ptr + 2 );

			p += sprintf(p, BASIC_FORMAT "0x%08x\n", t->name, w << 16 | v);
			break;
		}
		case PARAMS_TCHAR: {
			int i;
			char *d = data_ptr;

			p += sprintf(p, BASIC_FORMAT, t->name );
			for ( i = 0 ; i < t->length ; i++ ) {
				if ( *++d != 0 )
					p += sprintf(p, "%c", *d );
				d++;
			}
			p += sprintf(p,"\n");
			break;
		}
		case PARAMS_DONE:
			break;
		}
	}
	return p;
}

static int h3600_hal_proc_item_read(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{			      
	char *p = page;
	int len;
	int retval = 0;

	switch ((int)data) {
	case HAL_VERSION:
	{
		struct h3600_ts_version v;
		retval = h3600_get_version( &v );
		if ( !retval ) {
			p += sprintf(p, "Host      : %s\n", v.host_version );
		        p += sprintf(p, "Pack      : %s\n", v.pack_version );
			p += sprintf(p, "Boot type : 0x%02x\n", v.boot_type );
		}
		break;
	}
	case HAL_THERMAL_SENSOR:
	{
		unsigned short v;
		retval = h3600_get_thermal_sensor( &v );
		if ( !retval )
			p += sprintf(p, "0x%04x\n", v );
		break;
	}
	case HAL_BATTERY:
	{
		struct h3600_battery v;
		retval = h3600_get_battery( &v );
		if ( !retval ) {
			int i;
			p += sprintf(p, "AC status   : %x (%s)\n", v.ac_status,
				     extract_flag_name(ac_status,v.ac_status));

			for ( i = 0 ; i < v.battery_count ; i++ ) {
				p += sprintf(p, "Battery #%d\n", i);
				p += sprintf(p, " Chemistry  : 0x%02x (%s)\n", 
					     v.battery[i].chemistry,
					     extract_flag_name(battery_chemistry, 
							       v.battery[i].chemistry));
				p += sprintf(p, " Status     : 0x%02x (%s)\n", 
					     v.battery[i].status,
					     extract_flag_name(battery_status, v.battery[i].status));
				p += sprintf(p, " Voltage    : 0x%04x\n", v.battery[i].voltage);
			}				
		}
		break;
	}
	case HAL_LIGHT_SENSOR:
	{
		unsigned char level;
		retval = h3600_get_light_sensor(&level);
		if (!retval)
			p += sprintf(p,"0x%02x\n", level );
		break;
	}
	case HAL_ASSETS:
		p = h3600_hal_parse_eeprom( p );
		break;
	case HAL_MODEL:
		p += sprintf(p,"%s\n", h3600_generic_name() );
		break;
	default:
		p += sprintf(p,"Unsupported item %d\n", (int)data);
		break;
	}

	if ( retval ) {
		p += sprintf(p,"Error value %d\n", retval);
	}

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}


/***********************************************************************************/
/*       Registration functions from low level and from device drivers             */
/***********************************************************************************/

int h3600_hal_register_interface( struct h3600_hal_ops *ops )
{
	int retval = -ENODEV;
//        if (!try_inc_mod_count(ops->owner))
//                goto fail;

	h3600_hal_ops     = ops;

	/* Initialize our screen and frontlight */
	h3600_set_flite( flite_power, flite_brightness );
	h3600_set_contrast( screen_contrast );

	return 0;
 fail:
        return retval;
}

void h3600_hal_unregister_interface( struct h3600_hal_ops *ops )
{
	h3600_hal_ops     = NULL;
#ifdef CONFIG_MODULES
//        if (ops->owner) {
//                __MOD_DEC_USE_COUNT(ops->owner);
//        }
#endif
}

EXPORT_SYMBOL(h3600_hal_register_interface);
EXPORT_SYMBOL(h3600_hal_unregister_interface);

#define HAL_REG(x) \
	if ( ops->x ) g_driver_ops.x = ops->x
        
#define HAL_UNREG(x) \
	if ( ops->x ) g_driver_ops.x = NULL
        
int h3600_hal_register_driver( struct h3600_driver_ops *ops )
{
	HAL_REG(keypress);
	HAL_REG(touchpanel);
	HAL_REG(option_detect);
	return 0;
}

void h3600_hal_unregister_driver( struct h3600_driver_ops *ops )
{
	HAL_UNREG(keypress);
	HAL_UNREG(touchpanel);
	HAL_UNREG(option_detect);
}

EXPORT_SYMBOL(h3600_hal_register_driver);
EXPORT_SYMBOL(h3600_hal_unregister_driver);


/***********************************************************************************/
/*       Initialization                                                            */
/***********************************************************************************/

int __init h3600_hal_init_module(void)
{
	int i;
        printk(KERN_INFO "H3600 Registering HAL abstraction layer\n");

	/* Request the appropriate underlying module to provide services */
	/* TODO: We've put this here to avoid having to add another /etc/module
	   line for people upgrading their kernel.  In the future, we'd like to 
	   automatically request and load the appropriate subsystem based on the
	   type of iPAQ owned */
	if ( machine_is_h3100() || machine_is_h3600() ) {
		request_module("h3600_micro");
	} 
	else if ( machine_is_h3800() ) {
		request_module("h3600_asic");
	}
	else {
		printk(__FUNCTION__ ": unhandled ipaq model %s\n", h3600_generic_name() );
		return -ENODEV;
	}

	h3600_hal_sysctl_header = register_sysctl_table(h3600_hal_dir_table, 0);
	sa1100fb_backlight_power = h3600_hal_backlight_helper;

	/* Register in /proc filesystem */
	hal_proc_dir = proc_mkdir(H3600_HAL_PROC_DIR, NULL);
	if ( hal_proc_dir )
		for ( i = 0 ; g_procitems[i].id != HAL_DONE ; i++ )
			create_proc_read_entry(g_procitems[i].name, 0, hal_proc_dir, 
					       h3600_hal_proc_item_read, (void *) g_procitems[i].id );
	else
		printk(KERN_ALERT __FUNCTION__ 
		       ": unable to create proc entry " H3600_HAL_PROC_DIR "\n");

	return 0;
}

void h3600_hal_cleanup_module(void)
{
	int i;
	printk(KERN_INFO "H3600 shutting down HAL abstraction layer\n");

	sa1100fb_backlight_power = NULL;
        unregister_sysctl_table(h3600_hal_sysctl_header);

	if ( hal_proc_dir ) {
		for ( i = 0 ; g_procitems[i].id != HAL_DONE ; i++ )
			remove_proc_entry(g_procitems[i].name, hal_proc_dir);
		remove_proc_entry(H3600_HAL_PROC_DIR, NULL );
		hal_proc_dir = NULL;
	}
}

module_init(h3600_hal_init_module);
module_exit(h3600_hal_cleanup_module);
