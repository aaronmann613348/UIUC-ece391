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

    		break;
*/
    	case MTCP_RESET: 
    		tux_reset_helper(tty);
    		break;



    	default:
    		return -EINVAL;

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

*/
int
tuxctl_ioctl_tux_set_led(struct tty_struct* tty, unsigned long arg)
{
	//need to use
	//MPTC_ACK
	//MPTC_BIOC_EVENT

	
	char buf[6];// 6 bytes 
	//holds opcode, which LEDS, and 4 hex values to be shown by LEDS

	unsigned int time_bytes;//holder for the low 16 bits
	unsigned int LED_on;//holder for which LEDs should be turned on
	unsigned int decimal_on;//holder for decimal points that need to be on


	int LED0_val;
	int LED1_val;
	int LED2_val;
	int LED3_val;

	int maskbit1;
	int maskbit2;
	int maskbit3;
	int maskbit4;


	

	led = arg;
	time_bytes =  parser_helper_byByte(arg, 1, 16);
	LED_on = parser_helper_byByte(arg, 3, 4);
	decimal_on = parser_helper_byByte(arg, 4, 4);


	//parse time_bytes
	

	LED0_val = parser_helper_byBit(time_bytes, 1);
	LED1_val = parser_helper_byBit(time_bytes, 2);
	LED2_val = parser_helper_byBit(time_bytes, 3);
	LED3_val = parser_helper_byBit(time_bytes, 4);


	//call hex to seven helper!
	//first, we need to determine if we want the DEC parameter to be 1 or 0
	
	maskbit1 = 0x1; //0001
	maskbit2 = 0x2; //0010
	maskbit3 = 0x8; //0100
	maskbit4 = 0xF; //1000


	buf[0] = MTCP_LED_SET;
	buf[1] = LED_on;
	if((LED_on & maskbit1) == 1 )
	{
		buf[2] = hex_to_seven(LED0_val, 1);
	}
	else
	{
		buf[2] = hex_to_seven(LED0_val, 0);
	}
	if((LED_on & maskbit2) == 1 )
	{
		buf[3] = hex_to_seven(LED1_val, 1);
	}
	else
	{
		buf[3] = hex_to_seven(LED1_val, 0);
	}
	if((LED_on & maskbit3) == 1 )
	{
		buf[4] = hex_to_seven(LED2_val, 1);
	}
	else
	{
		buf[4] = hex_to_seven(LED2_val, 0);
	}
	if((LED_on & maskbit4) == 1 )
	{
		buf[5] = hex_to_seven(LED3_val, 1);
	}
	else
	{
		buf[5] = hex_to_seven(LED3_val, 0);
	}

	



	//we need to set up buf and figure out what the int n arg will be here
	tuxctl_ldisc_put(tty, buf, 6); //final put call

		

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

/*
*	helper function to parse the 32 bit arg
*	INPUTS:
*		arg -- the actual argument
*		startByte -- the byte we want to start parsing at
*		numBit -- the number of its we want to take from that byte
*	RETUNTS:
*		the new number we want
*/

int parser_helper_byByte(unsigned long arg, int startByte, int numBit)
{
	int holder;
	int shift;
	int bitmask;
	int retVal;
	shift = 8*(startByte -1);
	holder = arg << shift;

	if(numBit == 4)
	{
		bitmask = 0x000F; // 0xF = 1111
	}
	else if(numBit == 16)
	{
		bitmask = 0xFFFF; // 0xFFFF = 1111111111111111
	}
	else
	{
		bitmask = 0x0000;//else just make the whole thing zero
	}

	//bits with value 1 from arg will only register if the second argument of the '&' is a 1!
	retVal = (arg << shift) & bitmask;
	retVal = retVal >> numBit;//shift back over to the beginning of the new number!

	return retVal;


}

int parser_helper_byBit(unsigned long arg, int startBit)
{
	int holder;
	int shift;
	int bitmask;
	int retVal;
	shift = 4*(startBit -1);
	holder = arg << shift;

	bitmask = 0xF;

	//bits with value 1 from arg will only register if the second argument of the '&' is a 1!
	retVal = (arg << shift) & bitmask;
	retVal = retVal >> 4;//shift back over to the beginning of the new number!

	return retVal;


}


/*
	DEC bit determines if we need a decimal point after..
	to add the decimal, just add 0x10
*/
int hex_to_seven(int LED_VAL, int DEC)
{

	//declare representations in 7 segment 
	//calced by hand
	//just add 0x10 if decimal is needed 
	int seven_segments_rep[16];
	seven_segments_rep[0] = 0xE7;
	seven_segments_rep[1] = 0x06;
	seven_segments_rep[2] = 0xCB;
	seven_segments_rep[3] = 0x8F;
	seven_segments_rep[4] = 0x28;
	seven_segments_rep[5] = 0xAD;
	seven_segments_rep[6] = 0xED;
	seven_segments_rep[7] = 0x86;
	seven_segments_rep[8] = 0xEF;
	seven_segments_rep[9] = 0xAE;
	seven_segments_rep[10] = 0xEE;//A
	seven_segments_rep[11] = 0xEF;//B
	seven_segments_rep[12] = 0xE1;//C
	seven_segments_rep[13] = 0xE7;//D
	seven_segments_rep[14] = 0xE9;//E
	seven_segments_rep[15] = 0xEA;//F



	if((DEC == 1))
	{
		switch(LED_VAL)
		{
				case 0x0: 
					return seven_segments_rep[0] + 0x10;
					break;
				case 0x1:
					return seven_segments_rep[1] + 0x10;
					break;
				case 0x2:
					return seven_segments_rep[2] + 0x10;
					break;
				case 0x3:
					return seven_segments_rep[3] + 0x10;
					break;
				case 0x4:
					return seven_segments_rep[4] + 0x10;
					break;
				case 0x5:
					return seven_segments_rep[5] + 0x10;
					break;
				case 0x6:
					return seven_segments_rep[6] + 0x10;
					break;
				case 0x7:
					return seven_segments_rep[7] + 0x10;
					break;
				case 0x8:
					return seven_segments_rep[8] + 0x10;
					break;
				case 0x9:
					return seven_segments_rep[9] + 0x10;
					break;
				case 0xA:
					return seven_segments_rep[10] + 0x10;
					break;
				case 0xB:
					return seven_segments_rep[11] + 0x10;
					break;
				case 0xC:
					return seven_segments_rep[12] + 0x10;
					break;
				case 0xD:
					return seven_segments_rep[13] + 0x10;
					break;
				case 0xE:
					return seven_segments_rep[14] + 0x10;
					break;
				case 0xF:
					return seven_segments_rep[15] + 0x10;
					break;

				default:
	    			return -EINVAL;
		}


	}


	else//DEC = 0
	{
		switch(LED_VAL)
		{
				case 0x0: 
					return seven_segments_rep[0];
					break;
				case 0x1:
					return seven_segments_rep[1];
					break;
				case 0x2:
					return seven_segments_rep[2];
					break;
				case 0x3:
					return seven_segments_rep[3];
					break;
				case 0x4:
					return seven_segments_rep[4];
					break;
				case 0x5:
					return seven_segments_rep[5];
					break;
				case 0x6:
					return seven_segments_rep[6];
					break;
				case 0x7:
					return seven_segments_rep[7];
					break;
				case 0x8:
					return seven_segments_rep[8];
					break;
				case 0x9:
					return seven_segments_rep[9];
					break;
				case 0xA:
					return seven_segments_rep[10];
					break;
				case 0xB:
					return seven_segments_rep[11];
					break;
				case 0xC:
					return seven_segments_rep[12];
					break;
				case 0xD:
					return seven_segments_rep[13];
					break;
				case 0xE:
					return seven_segments_rep[14];
					break;
				case 0xF:
					return seven_segments_rep[15];
					break;

				default:
	    			return -EINVAL;
		}

	}
	

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

	 
}