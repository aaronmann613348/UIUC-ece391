/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"



//gonna need these guys
static int buttons;
static int led;
static int reset;

#define BITMASK_byte 0x0F//define bitmask 00001111
#define BITMASK_fourbit 0xF //define bitmask 1111
#define BITMASK_twobyte 0x000F//define bitmask 0000 0000 0000 1111

#define ADD_DEC 0x10//for seven segment rep.. decimal is on fourth most significant bit of seven seg representation




#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)




/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    switch(a)
    {
    	case MTCP_ACK:
    		if(reset ==1)
    		{
    			tuxctl_ioctl_tux_set_led(tty, led);
    			reset = 0;
    		}
    		break;
/*
    	case MTCP_BIOC_EVENT:
			//need the buttons!!!  ___ 	___
								  |	|    | |
								  |	  _\   |
								  |	______ |
    		break;
*/
    	case MTCP_RESET: 
    		tux_reset_helper(tty);
    		break;



    	default:
    		return;

    }


    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {


	case TUX_INIT:

			tuxctl_ioctl_tux_init(tty);
			return 0;


	case TUX_BUTTONS:

			return tuxctl_ioctl_tux_buttons(tty, arg);
			break;

	case TUX_SET_LED: 

			return tuxctl_ioctl_tux_set_led(tty, arg);
			break;

	case TUX_LED_ACK:

			//don't need to implement

	case TUX_LED_REQUEST:

			//dont need to implement

	case TUX_READ_LED:

			//don't need to implement

	default:
	    return -EINVAL;
    }
}
/*
*	INPUTs: tty struct!***
*	initializes vaiables associated with the driver 
*	we'll be calling this thing first
*/
int
tuxctl_ioctl_tux_init(struct tty_struct* tty)
{
	
	char buf[2];
	reset = 0;
	buttons = 0;
	led = 0;
	
	buf[0] = MTCP_BIOC_ON;
	buf[1] = MTCP_LED_USR;
	

	tuxctl_ldisc_put(tty, buf, 2);//send this up to tux
	printk("check initialized");
	return 0;

}
/*	INPUT: 32 bit integer
*	low 16 bits (15:0) specify a number whos hex value is displayed on the 7 segment displays
*	low 4 bits of the third byte (19:16) specify which LEDs should be on
*	lwo 4 bits of the highest byte (27:24) specify whether the corresponding decimal points should be turned on

	bitmasks defined above:
	#define BITMASK_byte 0x0F//define bitmask 00001111
	#define BITMASK_fourbit 0xF //define bitmask 1111
	#define BITMASK_twobyte 0x000F//define bitmask 0000 0000 0000 1111
	
	#define ADD_DEC 0x10//for seven segment rep.. decimal is on fourth most significant bit of seven seg representation

*/

int
tuxctl_ioctl_tux_set_led(struct tty_struct* tty, unsigned long arg)
{


	
	char buf[6];// 6 bytes; what we'll be sending to the TUX
	char seven_seg[16] = {0xE7, 0x06,  0xCB, 0x8F, 0x28, 0xAD, 0xED, 0x86, 0xEF, 0xAE, 0xEE, 0xEF, 0xE1, 0xE7, 0xE9, 0xEA};
	//holds opcode, which LEDS, and 4 hex values to be shown by LEDS

	int LED_values[4]; // 4 bytes to hold the LED hex values we want
	int holder1;
	int holder2;
	int holder3;
	int decimals_on;
	int i;//index
	int my_arg;//set so that we can hold arg and shift it around

	//need to make a shiftable mask for getting LED values
	int shiftingMask; // 0000 0000 0000 1111
	int oneMask; // 0001


	oneMask = 0x1; 
	shiftingMask = 0x000F;
	my_arg = arg;
	holder1 = my_arg >> 24;
	decimals_on = (holder1 & BITMASK_byte);//first get a binary representation of which decimals are on using bitmask

	holder2 = my_arg >> 16;//shift over 16 to check which LEDS on using bitmask  0000 1111
	
	buf[1] = (BITMASK_byte & holder2);//send these 8 bits to the second 'argument' of MTCP_LED_SET

	
	
	//populate array of 8 bit LED values
	//hold their binary representation in 4 bits
	for(i = 0; i < 4 ; i++)
	{
		
		holder3 = (shiftingMask & my_arg);//will this work?? not sure because of later values of my_arg
		LED_values[i] = holder3;
		shiftingMask = shiftingMask << 4;
	}

	//argument should be fully parsed by now
	//now we need to convert hex to seven segment and account for decimal point
	
	
	for(i = 0; i < 4 ; i++)
	{
		if(decimals_on & oneMask)
		{
			buf[i+2] = seven_seg[LED_values[i]] + ADD_DEC;
		}
		else
		{
			buf[i+2] = seven_seg[LED_values[i]];
		}
		oneMask = oneMask << 1;//shift one mask over!
	}


	//we need to set up buf and figure out what the int n arg will be here
	buf[0] = MTCP_LED_SET;
	
	tuxctl_ldisc_put(tty, buf, i+2); //final put call

		

	return 0;
}


/*
*	INPUT: 32 bit integer
*	returns -EINVAL error if this pointer is not valid
*	otherwise, sets the bits of the low byte corresponding to the currently pressed buttons as follows:
*	bit 7: right
*	bit 6: left
*	bit 5: down
*	bit 4: up
*	bit 3: c
*	bit 2: b
*	bit 1: a
*	bit 0: start
*
*/
int
tuxctl_ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg)
{
	return -EINVAL;
}



/* WE NEED TO HANDLE RESETS ALRIGHT YEAH!!!!!
*
*
*
*/

void tux_reset_helper(struct tty_struct * tty)
{
	char buf[2];
	buf[0] = MTCP_BIOC_ON;
	buf[1] = MTCP_LED_USR;
	reset = 1;
	tuxctl_ldisc_put(tty, buf, 2);//send it up!
	return;
	 
}