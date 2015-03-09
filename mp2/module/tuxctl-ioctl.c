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



static unsigned char array[2];//global array to read stuff in for buttons



static int flag_for_spamming;

static long  buttons;


#define FIVEMASK 0x20
#define SIXMASK 0x40
#define FIVESIXMASK 0x9F

#define BITMASK_byte 0x0F//define bitmask 00001111
#define BITMASK_fourbit 0xF //define bitmask 1111
#define BITMASK_twobyte 0x000F//define bitmask 0000 0000 0000 1111
#define BITMASK_full_byte 0xFF //11111111
#define ADD_DEC 0x10//for seven segment rep.. decimal is on fourth most significant bit of seven seg representation




#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)



//local function declarations:
/*
*	INPUTs: struct******
*	initializes vaiables associated with the driver 
*
*/
int tuxctl_ioctl_tux_init (struct tty_struct* tty);


/*	INPUT: 32 bit integer
*	
*
*	low 16 bits (15:0) specify a number whos hex value is displayed on the 7 segment displays
*	low 4 bits of the third byte (19:16) specify which LEDs should be on
*	lwo 4 bits of the highest byte (27:24) specify whether the corresponding decimal points should be turned on

*/
int tuxctl_ioctl_tux_set_led(struct tty_struct* tty, unsigned long arg);



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
int tuxctl_ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg);



/*
 *
 *	Reset Handler
 */
 void tux_reset_helper(struct tty_struct* tty);





/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet(struct tty_struct* tty, unsigned char* packet)
{
    unsigned  a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    switch(a)
    {
    	case MTCP_ACK:
    		//printk("check initialized");
    		flag_for_spamming = 0;
    		break;

    	case MTCP_BIOC_EVENT:
			
			array[0] = b;
			array[1] = c;

			

    		break;


    	case MTCP_RESET: 
    		tux_reset_helper(tty);
    		break;



    	default:
    		return;

    }


    printk("packet : %x %x %x\n", a, b, c); 
    printk("b which is array[0] is : %x \n", array[0]);
    printk("c which is array[1] is : %x \n", array[1]);

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

			flag_for_spamming = 0;
			tuxctl_ioctl_tux_init(tty);
			return 0;
			


	case TUX_BUTTONS:

			if(access_ok(%VERIFY_WRITE, (int*)arg, 4 ))/* (int*)arg == NULL OLD VERSION OF NULL CHECK */
			{
				return -EINVAL;
			}
	

			tuxctl_ioctl_tux_buttons(tty, arg);
			return 0;
			

	case TUX_SET_LED: 

			if(flag_for_spamming == 0)
			{
				flag_for_spamming = 1;
				 
				tuxctl_ioctl_tux_set_led(tty, arg);
				
			}

			return 0;
			

	case TUX_LED_ACK:


			//don't need to implement
			//return 0;

	case TUX_LED_REQUEST:

			//dont need to implement
			//return 0;

	case TUX_READ_LED:

			//don't need to implement
			//return 0;

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
	
	buttons = 0;
	flag_for_spamming = 0;

	
	
	
	buf[0] = MTCP_BIOC_ON;
	buf[1] = MTCP_LED_USR;
	
	
	tuxctl_ldisc_put(tty, buf, 2);//send this up to tux
	//printk("check initialized\n");
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


	
	unsigned char buf[6];// 6 bytes; what we'll be sending to the TUX
	int seven_seg[16] = {0xE7, 0x06,  0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAE, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};
	//holds opcode, which LEDS, and 4 hex values to be shown by LEDS

	int LED_values[4]; // 4 bytes to hold the LED hex values we want
	
	int decimals_on;
	int i;//index
	int my_arg;//set so that we can hold arg and shift it around

	//need to make a shiftable mask for getting LED values
	int shiftingMask; // 0000 0000 0000 1111
	int oneMask; // 0001

	

	oneMask = 0x1; 
	shiftingMask = 0x000F;


	//printk("%x\n", shiftingMask);


	my_arg = arg;

	decimals_on = ((my_arg >> 24) & BITMASK_byte);//first get a binary representation of which decimals are on using bitmask
	printk("decimals_on is %x \n", decimals_on);

	//shift over 16 to check which LEDS on using bitmask  0000 1111
	buf[1] = (BITMASK_byte & my_arg >> 16);//send these 8 bits to the second 'argument' of MTCP_LED_SET

	//printk(" buf[1] is %x \n" , buf[1]);
	//populate array of 8 bit LED values
	//hold their binary representation in 4 bits
	for(i = 0; i < 4 ; i++)
	{
		LED_values[i] = (shiftingMask & my_arg);
		my_arg = my_arg >> 4;//shift the argument over 4 bits
	}

	//argument should be fully parsed by now
	//now we need to convert hex to seven segment and account for decimal point
	for(i  = 0; i < 4 ; i++)
	{
		printk(" LED%d value is : %x \n" , i, LED_values[i]);
	}

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
		decimals_on = decimals_on >> 1; //shift the decimal_on 4 over 1 bit!
	}
	printk(" buf[1] is %x \n" , buf[1]);
	printk(" buf[2] is in seven_seg is %x \n" , buf[2]);
	printk(" buf[3] is in seven_seg is %x \n" , buf[3]);
	printk(" buf[4] is in seven_seg is %x \n" , buf[4]);
	printk(" buf[5] is in seven_seg is %x \n" , buf[5]);




	//we need to set up buf and figure out what the int n arg will be here
	buf[0] = MTCP_LED_SET;
	
	tuxctl_ldisc_put(tty, buf, i+2); //final put call

		

	return 0;
}


/*
*	INPUT: 32 bit integer pointer
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
*	INPUT: struct tty_struct * tty, unsigned long arg
*
* 	static unsigned long buttons;
* 	static unsigned char * array; //global array to read stuff in for buttons
*	void tuxctl_handle_packet(struct tty_struct* tty, unsigned char* packet)
*   unsigned  a, b, c;
*
*   a = packet[0];  Avoid printk() sign extending the 8-bit 
*
* 	 b = packet[1];  values when printing them. 
*   c = packet[2];

	#define FIVEMASK 0x20	//0010 0000
	#define SIXMASK 0x40	//0100 0000
*

*/
int
tuxctl_ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg)
{
	

	int holder; //holds the 8 bits we need to send up

	unsigned char temp1;//holder for C B A S
	unsigned char temp2;//holder for R L D U
	unsigned char bit5;
	unsigned char bit6;
	

	
	temp1 = array[0]; // X X X X C B A S
	temp2 = array[1]; // X X X X R D L U
	

	//populate holder
	holder = (temp1 & 0xF);
	//now add the bits of the second array to the high 4 bits of holder
	holder |= ((temp2 & 0xF) << 4);
	//holder now has R D L U C B A S (kernel)

	//now we want R L D U C B A S (user)

	bit5 = (holder & FIVEMASK); //and with bitmask 0x0010 0000
	bit6 = (holder & SIXMASK); //and with bitmask 0x0100 0000

	//clear 5th and 6th bits of holder
	holder &= FIVESIXMASK; //  and with bitmask 0x1001 1111

	bit5 = bit5 << 1;
	bit6 = bit6 >> 1;

	holder |=  bit5;
	holder |=  bit6;
	printk("holder is : %x \n", holder);


	//send this thing up!
	buttons = (long)holder;

	copy_to_user((long*)arg, &buttons, sizeof(long));
	
}




void tux_reset_helper(struct tty_struct * tty)
{
	//get ldisc_put ready for new input
	char buf[2];
	buf[0] = MTCP_BIOC_ON;
	buf[1] = MTCP_LED_USR;
	tuxctl_ldisc_put(tty, buf, 2);//send it up!

	return;
	 
}
