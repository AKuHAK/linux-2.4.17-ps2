/*
* Driver interface to the Atmel microcontroller on the H3100/H3600/H3700 iPAQ
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
* Author:  Andrew Christian
*          <Andrew.Christian@compaq.com>
*          September, 2001
*
* This module is based in part on the original h3600_ts.c by Charles Flynn
*
*/

#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/proc_fs.h>

#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/h3600_hal.h>
#include <asm/arch/h3600_gpio.h>

#define H3600_MICRO_PROC_DIR     "micro"
#define H3600_MICRO_PROC_STATS   "stats"

/***********************************************************************************/

/* Valid message types to receive from the Atmel */
enum {
	MSG_VERSION         = 0,
	MSG_KEYBOARD        = 2,
	MSG_TOUCHPANEL      = 3,
	MSG_EEPROM_READ     = 4,
	MSG_EEPROM_WRITE    = 5,
	MSG_THERMAL_SENSOR  = 6,
	MSG_NOTIFY_LED      = 8,
	MSG_BATTERY         = 9,
	MSG_SPI_READ        = 0xb,
	MSG_SPI_WRITE       = 0xc,
	MSG_BACKLIGHT       = 0xd,   /* H3600 only for backlight - both for light sensor */
	MSG_CODEC_CONTROL   = 0xe,   /* H3100 only */
	MSG_DISPLAY_CONTROL = 0xf    /* H3100 only */
};

struct ts_msg_statistics {
	u32   sent;
	u32   received;
	u32   total_ack_time;
	u32   timeouts;
};

struct ts_statistics {
	/* Statistics of the serial port itself */
	u32   isr;          /* Interrupts received */
	u32   tx;           /* Bytes transmitted   */
	u32   rx;           /* Bytes received      */
	u32   frame;        /* Frame errors        */
	u32   overrun;      /* Overrun errors      */
	u32   parity;       /* Parity errors       */
	u32   pass_limit;   /* Exceeded pass limit */

	/* Statistics of number of messages sent and received */
	struct ts_msg_statistics msg[16];

	/* Statistics for message decoding */
	u32   missing_sof;  /* Did not find a SOF when expected  */
	u32   bad_checksum; /* Bad checksum                      */

	/* Stats for general response */
	u32   timeouts;  
};

static struct ts_statistics     g_statistics;

/* event independent structure */
enum rx_state {
	STATE_SOF = 0,     /* Next byte should be start of frame */
	STATE_ID,          /* Next byte is ID & message length   */
	STATE_DATA,        /* Next byte is a data byte           */
	STATE_CHKSUM       /* Next byte should be checksum       */
};

#define RX_BUF_SIZE    16
struct h3600_ts_rxdev {
        enum rx_state state;            /* context of rx state machine */
        unsigned char chksum;           /* calculated checksum */
        int           id;               /* message ID from packet */
        unsigned int  len;              /* rx buffer length */
        unsigned int  index;            /* rx buffer index */
        unsigned char buf[RX_BUF_SIZE]; /* rx buffer size  */
};

#define TX_BUF_SIZE	32
struct h3600_ts_txdev {
	unsigned char     buf[TX_BUF_SIZE];	/* transmitter buffer */
	unsigned int      head;
	unsigned int      tail;
	struct semaphore  lock;
	int               id;                    /* Message ID we're looking for */
	void *            user_data;             /* Place to store result data */
	wait_queue_head_t waitq;
};

static struct h3600_ts_rxdev g_rxdev;	/* receive ISR state */
static struct h3600_ts_txdev g_txdev;
static struct h3600_hal_ops *g_micro_ops;

enum pen_state {
	PEN_UP,
	PEN_DOWN
};

static enum pen_state g_pen_state;
//static int            g_suspended = 0;

#define INCBUF(x,mod) (((x)+1) & ((mod) - 1))
#define INC_TX_BUF(x)  x = INCBUF(x,TX_BUF_SIZE)

#define CHAR_SOF        0x02

/***********************************************************************************/
/*   Message handlers - process a message from the Atmel microcontroller           */
/***********************************************************************************/

static void h3600_micro_default_ack( int id, int len, unsigned char *data )
{
	if ( len != 0 )
		printk(__FUNCTION__ ": invalid ack length = %d for message id %d\n", len, id );
}

static void h3600_micro_version_ack( int id, int len, unsigned char *data )
{
        struct h3600_ts_version * ver = (struct h3600_ts_version *) g_txdev.user_data;

	switch(len) {
	case 4:
		if ( ver ) {
			memcpy(ver->host_version, data, 4);
			ver->host_version[4] = 0;
			ver->pack_version[0] = 0;
			ver->boot_type = 0;
		}
		break;
	case 9:
		if ( ver ) {
			memcpy(ver->host_version, data ,4);
			memcpy(ver->pack_version,&data[4],4);
			ver->host_version[4] = 0;
			ver->pack_version[4] = 0;
			ver->boot_type = data[8];
		}
		break;
	default:
		printk(__FUNCTION__ ": illegal len = %d\n",len);
		break;
	}
}

static void h3600_micro_key_message( int id, int len, unsigned char *data  )
{
	if ( len != 1 ) {
		printk(__FUNCTION__ ": invalid message length %d\n", len );
		return;
	}
	
	h3600_hal_keypress( data[0] );
}

static void h3600_micro_touchpanel_message( int id, int len, unsigned char *data )
{
	unsigned short x,y;

	switch (len) {
	case 0:		/* Pen up */
		g_pen_state = PEN_UP;
		h3600_hal_touchpanel( 0, 0, 0 );
		break;

	case 4:         /* Pen down */
		switch ( g_pen_state ) {
		case PEN_DOWN:
			x = (((unsigned short)data[0]) << 8) | data[1];
			y = (((unsigned short)data[2]) << 8) | data[3];
			h3600_hal_touchpanel( x, y, 1 );
			break;

		case PEN_UP: /* Pen was previously UP - ignore this sample */
			g_pen_state = PEN_DOWN;
			break;
		}
		break;
	default:
		printk(__FUNCTION__ ": unexpected data length %d\n", len );
		break;
	}

}

static void h3600_micro_eeprom_read_ack( int id, int len, unsigned char *data )
{
	struct h3600_eeprom_read_request *p = (struct h3600_eeprom_read_request *) g_txdev.user_data;

	if ( len / 2 > EEPROM_RD_BUFSIZ ) {
		printk(__FUNCTION__ " bad length = %d\n",len);
		return;
	}

	if ( p ) {
		int i;
		unsigned short *buf = p->buff;
		for ( i = 0 ; i < len ; i += 2 )
			*buf++ = ( ((unsigned short)data[i]) << 8) | data[i+1];
		p->len = len / 2;
	}
}

static void h3600_micro_thermal_sensor_ack( int id, int len, unsigned char *data )
{
	unsigned short *p = (unsigned short *) g_txdev.user_data;

	if ( len != 2 ) {
		printk(__FUNCTION__ " bad event length %d\n", len);
		return;
	}

	if ( p ) 
		*p = ((unsigned short) data[1]) << 8 | data[0];
}

static void h3600_micro_battery_ack( int id, int len, unsigned char * data )
{
        struct h3600_battery *pbattinfo = (struct h3600_battery *) g_txdev.user_data;

	if ( len != 5 && len != 9 ) {
		printk(__FUNCTION__ ": illegal length = %d\n", len );
		return;
	}

	if ( pbattinfo ) {
		pbattinfo->ac_status            = data[0];
		pbattinfo->battery_count        = ( len == 5 ? 1 : 2 );

		pbattinfo->battery[0].chemistry = data[1];
		pbattinfo->battery[0].voltage   = (((unsigned short)data[3]) << 8) | data[2];
		pbattinfo->battery[0].status    = data[4];

		if ( pbattinfo->battery_count == 2 ) {
			pbattinfo->battery[1].chemistry = data[5];
			pbattinfo->battery[1].voltage   = (((unsigned short)data[6]) << 8) | data[7];
			pbattinfo->battery[1].status    = data[8];
		}
	}
}

/* This can be a normal read or a "special" read of the PCMCIA protection bits */
static void h3600_micro_spi_read_ack( int id, int len, unsigned char *data )
{
	struct h3600_spi_read_request *p = (struct h3600_spi_read_request *) g_txdev.user_data;
	
	if ( p ) {
		memcpy( p->buff, data, len );
		p->len = len;
	}
}

static void h3600_micro_backlight_ack( int id, int len, unsigned char *data )
{
	unsigned char *result = (unsigned char *) g_txdev.user_data;

	if ( len > 0 && result )
		*result = data[0];
}

/***********************************************************************************/
/*          Core interrupt code                                                    */
/***********************************************************************************/

enum MessageHandleType {
	HANDLE_NORMAL,
	HANDLE_ACK,
	HANDLE_ERROR
};

struct h3600_message_dispatch {
	const char *           name;
	void                 (*handle)( int id, int len, unsigned char *data );
	enum MessageHandleType type;
};

static struct h3600_message_dispatch g_handlers[] = {
	{ "Version",         h3600_micro_version_ack,        HANDLE_ACK    },  /* 0 */
	{ "ID=1",            0,                              HANDLE_ERROR  },  /* 1 */
	{ "Keyboard",        h3600_micro_key_message,        HANDLE_NORMAL },  /* 2 */
	{ "Touchpanel",      h3600_micro_touchpanel_message, HANDLE_NORMAL },  /* 3 */
	{ "EEPROM Read",     h3600_micro_eeprom_read_ack,    HANDLE_ACK    },  /* 4 */
	{ "EEPROM Write",    h3600_micro_default_ack,        HANDLE_ACK    },  /* 5 */
	{ "Thermal Sensor",  h3600_micro_thermal_sensor_ack, HANDLE_ACK    },  /* 6 */
	{ "ID=7",            0,                              HANDLE_ERROR  },  /* 7 */
	{ "Notify LED",      h3600_micro_default_ack,        HANDLE_ACK    },  /* 8 */
	{ "Battery",         h3600_micro_battery_ack,        HANDLE_ACK    },  /* 9 */
	{ "ID=10",           0,                              HANDLE_ERROR  },  /* 10 */
	{ "SPI Read",        h3600_micro_spi_read_ack,       HANDLE_ACK    },  /* 11 */
	{ "SPI Write",       h3600_micro_default_ack,        HANDLE_ACK    },  /* 12 */
	{ "Backlight",       h3600_micro_backlight_ack,      HANDLE_ACK    },  /* 13 */
	{ "Codec Control",   h3600_micro_default_ack,        HANDLE_ACK    },  /* 14 */
	{ "Display Control", h3600_micro_default_ack,        HANDLE_ACK    }   /* 15 */
};

static void h3600_micro_rx_msg( int id, int len, unsigned char *data )
{
	struct h3600_message_dispatch *d = &g_handlers[id];

	if (0) {
		int i;
		printk(KERN_INFO __FUNCTION__ ": id=%d len=%d message=",id,len);
		for ( i = 0 ; i < len ; i++ ) printk("%02x ",data[i]);
		printk("\n");
	}

	switch (d->type) {
	case HANDLE_NORMAL:
		if ( d->handle ) 
			(*(d->handle))( id, len, data );
		break;
	case HANDLE_ACK:
		if ( g_txdev.id != id ) {
			printk(__FUNCTION__ ": unexpected message id=%d while waiting for id=%d\n", 
			       id, g_txdev.id);
		}
		else {
			if ( d->handle )
				(*(d->handle))( id, len, data );
			g_txdev.id = -1;
			wake_up_interruptible( &g_txdev.waitq );
		}
		break;
	case HANDLE_ERROR:
		printk(__FUNCTION__ ": unrecognized message id=%d\n", id );
		break;
	}
	g_statistics.msg[id].received++;
}

static void h3600_micro_process_char( unsigned char rxchar )
{
        switch ( g_rxdev.state ) {
        case STATE_SOF:         /* Looking for SOF */
                if ( rxchar == CHAR_SOF )
                        g_rxdev.state=STATE_ID; /* Next byte is the id and len */
		else 
			g_statistics.missing_sof++;
                break;

        case STATE_ID:          /* Looking for id and len byte */
                g_rxdev.id  = ( rxchar & 0xf0 ) >> 4 ;
                g_rxdev.len    = ( rxchar & 0x0f );
                g_rxdev.index  = 0;
                g_rxdev.chksum = rxchar;
                g_rxdev.state  = ( g_rxdev.len > 0 ) ? STATE_DATA : STATE_CHKSUM;
                break;

        case STATE_DATA:        /* Looking for 'len' data bytes */
                g_rxdev.chksum += rxchar;
                g_rxdev.buf[g_rxdev.index]= rxchar;
                if ( ++g_rxdev.index == g_rxdev.len )
                        g_rxdev.state = STATE_CHKSUM;
                break;

        case STATE_CHKSUM:      /* Looking for the checksum */
                if ( rxchar == g_rxdev.chksum )
			h3600_micro_rx_msg( g_rxdev.id, g_rxdev.len, g_rxdev.buf );
		else
                        g_statistics.bad_checksum++;

                g_rxdev.state = STATE_SOF;
                break;
	}
}

static void h3600_micro_rx_chars( void )
{
	unsigned int status, ch;

	while ( (status = Ser1UTSR1) & UTSR1_RNE ) {
		ch = Ser1UTDR;
		g_statistics.rx++;

		if ( status & UTSR1_PRE ) { /* Parity error */
			g_statistics.parity++;
		} else 	if ( status & UTSR1_FRE ) { /* Framing error */
			g_statistics.frame++;
		} else {
			if ( status & UTSR1_ROR )   /* Overrun error */
				g_statistics.overrun++;
			
			h3600_micro_process_char( ch );
		}
	}
}

static void h3600_micro_tx_chars( void )
{
	if (0) printk(KERN_CRIT __FUNCTION__ " head = %d tail = %d\n", g_txdev.head, g_txdev.tail);

	while ( g_txdev.tail != g_txdev.head && (Ser1UTSR1 & UTSR1_TNF) ) {
		Ser1UTDR = g_txdev.buf[g_txdev.tail];
		INC_TX_BUF(g_txdev.tail);
		g_statistics.tx++;
	}

	if ( g_txdev.tail == g_txdev.head )      /* Stop interrupts */
		Ser1UTCR3 &= ~UTCR3_TIE;
}

#define H3600_TS_PASS_LIMIT 10

static void h3600_micro_serial_isr(int irq, void *dev_id, struct pt_regs *regs)
{
        unsigned int status;	/* UTSR0 */
	unsigned int pass_counter = 0;

	if (0) printk(__FILE__ ": " __FUNCTION__ "\n");

	g_statistics.isr++;
        status = Ser1UTSR0;
	do {
		if ( status & (UTSR0_RID | UTSR0_RFS) ) {
			if ( status & UTSR0_RID )
				Ser1UTSR0 = UTSR0_RID; /* Clear the Receiver IDLE bit */
			h3600_micro_rx_chars();
		}

		/* Clear break bits */
		if (status & (UTSR0_RBB | UTSR0_REB))
			Ser1UTSR0 = status & (UTSR0_RBB | UTSR0_REB);

		if ( status & UTSR0_TFS )
			h3600_micro_tx_chars();

		status = Ser1UTSR0;
	} while ( (((g_txdev.head != g_txdev.tail) && (status & UTSR0_TFS))
		   || status & (UTSR0_RFS | UTSR0_RID ))
		  && pass_counter++ < H3600_TS_PASS_LIMIT );

	if ( pass_counter >= H3600_TS_PASS_LIMIT )
		g_statistics.pass_limit++;
}

static void h3600_micro_reset_comm( void )
{
	if (0) printk(__FUNCTION__ ": initializing serial port\n");

        /* Initialize Serial channel protocol frame */
        g_rxdev.state     = STATE_SOF;  /* Reset the state machine */

	g_txdev.id        = -1;         /* We're not looking for a message */
	g_txdev.user_data = NULL;
        g_txdev.head      = 0;          /* Reset the transmission queue */
	g_txdev.tail      = 0;
	g_pen_state       = PEN_UP;

	/* Set up interrupts */
	Ser1SDCR0 = 0x1;                               /* Select UART mode */

	Ser1UTCR3 = 0;                                 /* Clean up CR3                  */
	Ser1UTCR0 = UTCR0_8BitData | UTCR0_1StpBit;    /* 8 bits, no parity, 1 stop bit */
	Ser1UTCR1 = 0;                                 /* Baud rate to 115K bits/sec    */
        Ser1UTCR2 = 0x1;

	Ser1UTSR0 = 0xff;                              /* Clear SR0 */
        Ser1UTCR3 = UTCR3_TXE | UTCR3_RXE | UTCR3_RIE; /* Enable receive interrupt */
}


/***********************************************************************************/
/*      Special button IRQs                                                        */
/***********************************************************************************/

#define MAKEKEY(index, down)  ((down) ? (index) : ((index) | 0x80))

static void h3600_micro_action_isr(int irq, void *dev_id, struct pt_regs *regs)
{
        int down = (GPLR & GPIO_H3600_ACTION_BUTTON) ? 0 : 1;
        if (dev_id != h3600_micro_action_isr)
                return;
           
	h3600_hal_keypress( MAKEKEY( 10, down ) );
}

static void h3600_micro_power_isr(int irq, void *dev_id, struct pt_regs *regs)
{
        int down = (GPLR & GPIO_H3600_NPOWER_BUTTON) ? 0 : 1;
        if (dev_id != h3600_micro_power_isr)
                return;

	h3600_hal_keypress( MAKEKEY( 11, down ) );
}

static void h3600_micro_sleeve_isr(int irq, void *dev_id, struct pt_regs *regs)
{
        int present = (GPLR & GPIO_H3600_OPT_DET) ? 0 : 1;
        if (dev_id != h3600_micro_sleeve_isr)
                return;

	h3600_hal_option_detect( present );
        GEDR = GPIO_H3600_OPT_DET;    /* Clear the interrupt */
}

/***********************************************************************************/
/*      Standard entry point for microcontroller requests                          */
/***********************************************************************************/

/* 
   We insert a bunch of bytes in the ring buffer.
   Return an error if less than the appropriate number have been sent 
*/

#define FRAME_OVERHEAD	3	/* CHAR_SOF, ID <<8 |  LEN, CHKSUM */

static int h3600_micro_tx_msg( int id, int len, unsigned char * data )
{
        unsigned char chksum;
        unsigned int space,head,tail,i;

	if ( id < 0 || id > 15 || len < 0 || len > 15 ) {
		printk(KERN_CRIT __FUNCTION__ ": illegal value id=%d len=%d\n", id, len);
		return -EINVAL;
	}

	if (0) {
		int i;
		unsigned char sum = (id << 4) | len;
		printk(KERN_INFO __FUNCTION__ ": id=%d len=%d message=%02x:%02x", 
		       id,len,CHAR_SOF,(id<<4)|len);
		for (i = 0 ; i < len ; i++) {
			printk(":%02x",data[i]);
			sum += data[i];
		}
		printk(":%02x\n",sum);
	}

	head = g_txdev.head;
	tail = g_txdev.tail;
	space = ( head >= tail ? TX_BUF_SIZE - (head - tail) - 1 : tail - head - 1);

	if ( space < len + FRAME_OVERHEAD ) {
		printk(KERN_CRIT __FUNCTION__ ": insufficient space in the transmit buffer %d\n", space );
		return -EIO;
	}

        chksum = (id << 4) | len;

	g_txdev.buf[head]= (unsigned char)CHAR_SOF;
	INC_TX_BUF(head);
	g_txdev.buf[head]= chksum;
	INC_TX_BUF(head);

	for ( i = 0 ; i < len ; i++ ) {
		g_txdev.buf[head]= *data;	
		INC_TX_BUF(head);
		chksum += *data++;
	}
	g_txdev.buf[head]=chksum;
	INC_TX_BUF(head);

	g_txdev.head = head;
	Ser1UTCR3 |= UTCR3_TIE; /* Enable Tx IRQs */
	g_statistics.msg[id].sent++;

	return 0;
}


static int h3600_micro_request( int id, int len, unsigned char *data, void *user_data )
{
	wait_queue_t  wait;
	signed long   timeout;
	int           result = 0;
	unsigned long start_time = jiffies;

	if ( in_interrupt() ) {
		printk(__FUNCTION__ ": trying to execute id=%d in interrupt context\n",id);
		return -ERESTARTSYS;  /* TODO: is this a good error value? */
	}

/*	if ( g_suspended ) {
		printk(__FUNCTION__ ": trying to execute id=%d while suspended\n",id);
		switch (PM_STATE()) {
		case PM_STATE_NORMAL:
			printk(__FUNCTION__ ": confusion?\n");
			break;
		case PM_STATE_REQUESTING_SUSPEND:
			printk(__FUNCTION__ ": it's probably okay to still make this call\n");
			break;
		case PM_STATE_RESUMING:
			printk(__FUNCTION__ ": cleverly re-enabling myself early\n");
			h3600_micro_reset_comm();
                        g_suspended = 0;
			break;
		default:
			return -ERESTARTSYS;
		}
	}
*/
	if ( down_interruptible(&g_txdev.lock) )
		return -ERESTARTSYS;

	/* Send data stream */
	if (0) {
		int i;
		printk(__FUNCTION__ ": id=%d len=%d ",id,len);
		for ( i = 0 ; i < len ; i++ )
			printk("0x%02x ", data[i]);
		printk("\n");
	}

	result = h3600_micro_tx_msg( id, len, data );
	if ( result )
		return result;

	/* Store ID and result pointer (we assume that we haven't received a result yet) */
	g_txdev.id        = id;
	g_txdev.user_data = user_data;

	/* Wait for id == 0 or a timeout and sleep              */
	/* We're basically using interruptible_sleep_on_timeout */
	init_waitqueue_entry(&wait,current);
	add_wait_queue(&g_txdev.waitq, &wait);
	timeout = 100 * HZ / 1000;    /* 100 milliseconds (empirically derived) */

	while ( timeout > 0 ) {
		set_current_state( TASK_INTERRUPTIBLE );
		if ( g_txdev.id != id )
			break;
		if ( !signal_pending(current) ) {
			timeout = schedule_timeout( timeout );
			if ( timeout <= 0 ) {
				g_statistics.timeouts++;
				g_statistics.msg[id].timeouts++;
				result = -ETIMEDOUT;       /* is this right? */
			}
			continue;
		}
		result = -ERESTARTSYS;
		break;
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(&g_txdev.waitq, &wait);
	
	g_txdev.user_data = NULL;
	up(&g_txdev.lock);

	g_statistics.msg[id].total_ack_time += (jiffies - start_time);
	return result;
}


/***********************************************************************************/
/*      Standard entry points for microcontroller requests                         */
/***********************************************************************************/

int h3600_micro_version( struct h3600_ts_version *result )
{
	return h3600_micro_request( MSG_VERSION, 0, NULL, result );
}

int h3600_micro_eeprom_read( unsigned short address, unsigned char *data, unsigned short len )
{
        int status, index, read_size;
	struct h3600_eeprom_read_request request;

	if (0) printk(__FUNCTION__ ": address=%d len=%d\n", address, len);
	for ( index = 0 ; index < len ; ) {
		read_size = len - index;
		if ( read_size > EEPROM_RD_BUFSIZ * 2 )
			read_size = EEPROM_RD_BUFSIZ * 2;

		request.addr = (address + index) / 2;
		request.len  = read_size / 2;
	
		status = h3600_micro_request( MSG_EEPROM_READ, 2, 
					      (unsigned char *) &request, &request );
		if ( status )
			return status;

		read_size = request.len * 2;
		memcpy(data, &request.buff, read_size);
		data += read_size;
		index += read_size;
	}
        return 0;
}


int h3600_micro_eeprom_write( unsigned short address, unsigned char *data, unsigned short len )
{
	struct h3600_eeprom_write_request request;
        int status, index, write_size;
	
	for ( index = 0 ; index < len ; ) {
		write_size = len - index;
		if ( write_size > EEPROM_WR_BUFSIZ * 2 )
			write_size = EEPROM_WR_BUFSIZ * 2;

		request.addr = (address + index) / 2;
		request.len  = write_size / 2;
		memcpy(request.buff, data, write_size);

		status = h3600_micro_request( MSG_EEPROM_WRITE, write_size + 1, 
					      ((unsigned char *) &request) + 1, NULL );
		if ( status )
			return status;

		data += write_size;
		index += write_size;
	}
        return 0;
}

int h3600_micro_thermal_sensor( unsigned short *result )
{
	return h3600_micro_request( MSG_THERMAL_SENSOR, 0, NULL, result );
}

int h3600_micro_notify_led( unsigned char mode, unsigned char duration, 
			    unsigned char ontime, unsigned char offtime )
{
	unsigned char buf[4];
	buf[0] = mode;
	buf[1] = duration;
	buf[2] = ontime;
	buf[3] = offtime;

	return h3600_micro_request( MSG_NOTIFY_LED, 4, (unsigned char *) buf, NULL );
}

int h3600_micro_battery( struct h3600_battery *result )
{
	return h3600_micro_request( MSG_BATTERY, 0, NULL, result );
}

/* Address = 0xffff is a special request for NM25C020 status */
/* This is currently not handled correctly */

int h3600_micro_spi_read(unsigned short address, unsigned char *data, unsigned short len)
{
        int status, index, read_size;
	struct h3600_spi_read_request request;

	for ( index = 0 ; index < len ; ) {
		read_size = len - index;
		if ( read_size > SPI_RD_BUFSIZ )
			read_size = SPI_RD_BUFSIZ;

		request.addr = address + index;
		request.len  = read_size;
	
		status = h3600_micro_request( MSG_SPI_READ, 3, (unsigned char *) &request, &request );
		if ( status )
			return status;

		memcpy(data, &request.buff, read_size);
		data += read_size;
		index += read_size;
	}
        return 0;
}

int h3600_micro_spi_write(unsigned short address, unsigned char *data, unsigned short len)
{
	struct h3600_spi_write_request request;
        int status, index, write_size;
	
	for ( index = 0 ; index < len ; ) {
		write_size = len - index;
		if ( write_size > SPI_WR_BUFSIZ )
			write_size = SPI_WR_BUFSIZ;

		request.addr = address + index;
		request.len  = write_size;
		memcpy(request.buff, data, write_size);

		status = h3600_micro_request( MSG_SPI_WRITE, write_size + 2, 
					      ((unsigned char *) &request) + 2, NULL );
		if ( status )
			return status;

		data += write_size;
		index += write_size;
	}
        return 0;
}

int h3100_micro_codec_control( unsigned char command, unsigned char level )
{
	unsigned char buf[2];
	buf[0] = command;
	buf[1] = level;

	return h3600_micro_request( MSG_CODEC_CONTROL, 2, buf, NULL );
}

/******************/

int h3600_micro_read_light_sensor( unsigned char *result )
{
	unsigned char buf[3];
	buf[0] = FLITE_GET_LIGHT_SENSOR;
	buf[1] = buf[2] = 0;

	return h3600_micro_request( MSG_BACKLIGHT, 3, buf, result );
}

int h3600_micro_backlight_control( enum flite_pwr power, unsigned char level )
{
	unsigned char buf[3];
	buf[0] = FLITE_MANUAL_MODE;
	buf[1] = power;
	buf[2] = level;
	
	return h3600_micro_request( MSG_BACKLIGHT, 3, buf, NULL );
}

enum {
	TS_DISPLAY_BACKLIGHT = 1,
	TS_DISPLAY_CONTRAST  = 2
};
	
int h3100_micro_backlight_control( enum flite_pwr power, unsigned char level )
{
	unsigned char buf[2];
	buf[0] = TS_DISPLAY_BACKLIGHT;
	buf[1] = ( power == FLITE_PWR_ON && level > 0 ? 1 : 0 );

	return h3600_micro_request( MSG_DISPLAY_CONTROL, 2, buf, NULL );
}

int h3100_micro_contrast_control( unsigned char level )
{
	unsigned char buf[2];
	buf[0] = TS_DISPLAY_CONTRAST;
	buf[1] = level;

	return h3600_micro_request( MSG_DISPLAY_CONTROL, 2, buf, NULL );
}

int h3600_micro_option_pack( int *result )
{
        int opt_ndet = (GPLR & GPIO_H3600_OPT_DET);

	*result = opt_ndet ? 0 : 1;
	return 0;
}

int h3600_micro_set_audio_clock( long val )
{
	/* Set the external clock generator */
	switch (val) {
	case 24000:
	case 32000:
	case 48000:
		/* 00: 12.288 MHz */
		GPCR = GPIO_H3600_CLK_SET0 | GPIO_H3600_CLK_SET1;
		break;
	case 22050:
	case 29400:
	case 44100:
		/* 01: 11.2896 MHz */
		GPSR = GPIO_H3600_CLK_SET0;
		GPCR = GPIO_H3600_CLK_SET1;
		break;
	case 8000:
	case 10666:
	case 16000:
		/* 10: 4.096 MHz */
		GPCR = GPIO_H3600_CLK_SET0;
		GPSR = GPIO_H3600_CLK_SET1;
		break;
	case 10985:
	case 14647:
	case 21970:
		/* 11: 5.6245 MHz */
		GPSR = GPIO_H3600_CLK_SET0 | GPIO_H3600_CLK_SET1;
		break;
	default:
		return -EIO;
	}

	return 0;
}

/***********************************************************************************/
/*   Power management                                                              */
/***********************************************************************************/
/*
#ifdef CONFIG_PM
static int h3600_micro_pm_callback(struct pm_dev *pm_dev, pm_request_t req, void *data)
{
        if (0) printk(__FUNCTION__ ": req=%d suspended=%d\n", req, g_suspended);
	switch (req) {
	case PM_SUSPEND: // enter D1-D3 
                g_suspended = 1;
                break;
	case PM_RESUME:  // enter D0 - same as unblank 
                if (g_suspended) {
			h3600_micro_reset_comm();
                        g_suspended = 0;
                }
                break;
	}
	return 0;
}
#endif
*/

static int h3600_micro_pm_callback(pm_request_t req)
{
        if (1) printk(__FUNCTION__ ": req=%d\n", req);
	
	switch (req) {
	case PM_RESUME:
		h3600_micro_reset_comm();
		break;
	}
	return 0;
}

/***********************************************************************************/
/*   Proc filesystem interface                                                     */
/***********************************************************************************/

static struct proc_dir_entry   *micro_proc_dir;

#define PRINT_DATA(x,s) \
	p += sprintf (p, "%-20s : %d\n", s, g_statistics.x)

int h3600_micro_proc_stats_read(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	char *p = page;
	int len;
	int i;

	PRINT_DATA(isr,        "Interrupts");
	PRINT_DATA(tx,         "Bytes transmitted");
	PRINT_DATA(rx,         "Bytes received");
	PRINT_DATA(frame,      "Frame errors");
	PRINT_DATA(overrun,    "Overrun errors");
	PRINT_DATA(parity,     "Parity errors");
	PRINT_DATA(pass_limit, "Pass limit");
	PRINT_DATA(missing_sof,  "Missing SOF");
	PRINT_DATA(bad_checksum, "Bad checksums");
	PRINT_DATA(timeouts,     "ACK timeouts");

	p += sprintf(p,"\nMessages             Sent   Rcvd  Jiffs Timeout\n");
	for ( i = 0 ; i < 16 ; i++ )
		p += sprintf(p,"%-18s %6d %6d %6d %6d\n", 
			     g_handlers[i].name,
			     g_statistics.msg[i].sent,
			     g_statistics.msg[i].received,
			     g_statistics.msg[i].total_ack_time,
			     g_statistics.msg[i].timeouts);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

/***********************************************************************************/
/*   Initialization code                                                           */
/***********************************************************************************/

static struct h3600_hal_ops h3600_micro_ops = {
	get_version         : h3600_micro_version,
	eeprom_read         : h3600_micro_eeprom_read,
	eeprom_write        : h3600_micro_eeprom_write,
	get_thermal_sensor  : h3600_micro_thermal_sensor,
	set_notify_led      : h3600_micro_notify_led,
	read_light_sensor   : h3600_micro_read_light_sensor,
	get_battery         : h3600_micro_battery,
	spi_read            : h3600_micro_spi_read,
	spi_write           : h3600_micro_spi_write,
	get_option_detect   : h3600_micro_option_pack,
	set_audio_clock     : h3600_micro_set_audio_clock,
        owner               : THIS_MODULE,
	
	/* H3600 specific calls */
//	codec_control       : h3600_micro_codec_control,
	backlight_control   : h3600_micro_backlight_control,
/*	contrast_control    : NULL    */  /* H3600 doesn't have contrast */
};

static struct h3600_hal_ops h3100_micro_ops = {
	get_version         : h3600_micro_version,
	eeprom_read         : h3600_micro_eeprom_read,
	eeprom_write        : h3600_micro_eeprom_write,
	get_thermal_sensor  : h3600_micro_thermal_sensor,
	set_notify_led      : h3600_micro_notify_led,
	read_light_sensor   : h3600_micro_read_light_sensor,
	get_battery         : h3600_micro_battery,
	spi_read            : h3600_micro_spi_read,
	spi_write           : h3600_micro_spi_write,
	get_option_detect   : h3600_micro_option_pack,
	set_audio_clock     : h3600_micro_set_audio_clock,
        owner               : THIS_MODULE,

	/* H3100 specific calls */
	codec_control       : h3100_micro_codec_control,
	backlight_control   : h3100_micro_backlight_control,
	contrast_control    : h3100_micro_contrast_control,
};

static int h3600_micro_setup( void )
{
	int result;

        printk(__FUNCTION__ ": setting up microcontroller interface\n");

	if ( machine_is_h3100() ) {
		g_micro_ops = &h3100_micro_ops;
	} 
	else if ( machine_is_h3600() ) {
		g_micro_ops = &h3600_micro_ops;
	}
	else {
		printk(__FUNCTION__ ": illegal iPAQ model %s\n", h3600_generic_name() );
		return -ENODEV;
	}

	/* Set up our structures */
	init_waitqueue_head( &g_txdev.waitq );
	init_MUTEX( &g_txdev.lock );

	/* Start working */
	h3600_micro_reset_comm();

	/* Set up interrupts */
	result = request_irq(IRQ_Ser1UART, h3600_micro_serial_isr, SA_SHIRQ | SA_INTERRUPT,
                             "h3600_ts", h3600_micro_serial_isr);
	if ( result ) {
		printk(KERN_CRIT __FUNCTION__ ": unable to grab serial port IRQ\n");
		return result;
	}

        set_GPIO_IRQ_edge( GPIO_H3600_ACTION_BUTTON, GPIO_BOTH_EDGES );
	result = request_irq(IRQ_GPIO_H3600_ACTION_BUTTON, 
			     h3600_micro_action_isr, 
			     SA_SHIRQ | SA_INTERRUPT | SA_SAMPLE_RANDOM,
			     "h3600_action", h3600_micro_action_isr);

	if ( result ) {
		printk(KERN_CRIT __FUNCTION__ ": unable to grab action button IRQ\n");
		free_irq( IRQ_Ser1UART, h3600_micro_serial_isr );
		return result;
	}

        set_GPIO_IRQ_edge( GPIO_H3600_NPOWER_BUTTON, GPIO_BOTH_EDGES );
	result = request_irq(IRQ_GPIO_H3600_NPOWER_BUTTON, 
			     h3600_micro_power_isr, 
			     SA_SHIRQ | SA_INTERRUPT | SA_SAMPLE_RANDOM,
			     "h3600_suspend", h3600_micro_power_isr);

	if ( result ) {
		printk(KERN_CRIT __FUNCTION__ ": unable to grab power button IRQ\n");
		free_irq( IRQ_Ser1UART, h3600_micro_serial_isr );
		free_irq( IRQ_GPIO_H3600_ACTION_BUTTON, h3600_micro_action_isr );
		return result;
	}

        set_GPIO_IRQ_edge( GPIO_H3600_OPT_DET, GPIO_BOTH_EDGES );
	result = request_irq(IRQ_GPIO_H3600_OPT_DET, h3600_micro_sleeve_isr, 
			     SA_SHIRQ | SA_INTERRUPT | SA_SAMPLE_RANDOM,
                             "h3600_sleeve", h3600_micro_sleeve_isr);
	
	if ( result ){
		printk(KERN_CRIT __FUNCTION__ ": unable to grab option pack detect IRQ\n");
		free_irq( IRQ_Ser1UART, h3600_micro_serial_isr );
		free_irq( IRQ_GPIO_H3600_ACTION_BUTTON, h3600_micro_action_isr );
		free_irq( IRQ_GPIO_H3600_OPT_DET, h3600_micro_sleeve_isr );
		return result;
	}

	/* Register in /proc filesystem */
	micro_proc_dir = proc_mkdir(H3600_MICRO_PROC_DIR, NULL);
	if ( micro_proc_dir )
		create_proc_read_entry(H3600_MICRO_PROC_STATS, 0, micro_proc_dir, 
				       h3600_micro_proc_stats_read, NULL );
	else
		printk(KERN_ALERT __FUNCTION__ 
		       ": unable to create proc entry " H3600_MICRO_PROC_DIR "\n");

	/* Register with power management */
	h3600_register_pm_callback( h3600_micro_pm_callback );
//	pm_register(PM_SYS_DEV, PM_SYS_COM, h3600_micro_pm_callback);

	/* Set some GPIO direction registers */
	GPDR |= (GPIO_H3600_CLK_SET0 | GPIO_H3600_CLK_SET1);

	return 0;
}


int __init h3600_micro_init( void )
{
	int result;

	result = h3600_micro_setup();
	if ( !result )
		result = h3600_hal_register_interface( g_micro_ops );
	return result;
}

void __exit h3600_micro_cleanup( void )
{
	h3600_hal_unregister_interface( g_micro_ops );

	free_irq(IRQ_Ser1UART, h3600_micro_serial_isr);
	free_irq(IRQ_GPIO_H3600_ACTION_BUTTON, h3600_micro_action_isr);
	free_irq(IRQ_GPIO_H3600_NPOWER_BUTTON, h3600_micro_power_isr);
	free_irq(IRQ_GPIO_H3600_OPT_DET,       h3600_micro_sleeve_isr);

	h3600_unregister_pm_callback( h3600_micro_pm_callback );
//	pm_unregister_all( h3600_micro_pm_callback );

	if ( micro_proc_dir ) {
		remove_proc_entry(H3600_MICRO_PROC_STATS, micro_proc_dir);
		remove_proc_entry(H3600_MICRO_PROC_DIR, NULL );
		micro_proc_dir = NULL;
	}
}


module_init(h3600_micro_init)
module_exit(h3600_micro_cleanup)