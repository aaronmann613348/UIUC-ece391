// All necessary declarations for the Tux Controller driver must be in this file

#ifndef TUXCTL_H
#define TUXCTL_H


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/tty.h>





#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)





#endif

/*
*	INPUTs: struct******
*	initializes vaiables associated with the driver 
*
*/
extern int tuxctl_ioctl_tux_init(struct tty_struct* tty);


/*	INPUT: 32 bit integer
*	
*
*	low 16 bits (15:0) specify a number whos hex value is displayed on the 7 segment displays
*	low 4 bits of the third byte (19:16) specify which LEDs should be on
*	lwo 4 bits of the highest byte (27:24) specify whether the corresponding decimal points should be turned on

*/
extern int tuxctl_ioctl_tux_set_led(struct tty_struct* tty, unsigned long arg);



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
extern int tuxctl_ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg);



/*
 *
 *	Reset Handler
 */
extern void tux_reset_helper(struct tty_struct * tty);



