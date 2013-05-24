/**
 * Author: Matt Cruse, Haleigh Vierra
 *
 * Project: CollegeBound
 *
 * Description: This is the driver that was designed to use an SNES controller
 * with the STK600. The SNES controller has a shift register that latches
 * the button status and transmits a button sequence off of a clock cycle. This
 * code replicates the interface that the SNES console uses with the controllers.
 **/
 #ifndef _SNES_H_
#define _SNES_H_

#include <avr/io.h>
#include <util/delay.h>

//uses B port
#define SNES_DDR     DDRB
#define SNES_PORT    PORTB
#define SNES_PIN     PINB
#define LATCH        0        //PB0
#define CLK          1        //PB1
#define DATA         2        //PB2

#define LATCH_TIME   12       //time [us] for the latch signal
#define CLK_TIME     5.6      //time [us] for 1/2 the clock (50% duty)
#define NUM_BTNS     12       //the total number of buttons on the controller

//----------------------------Function Prototypes----------------------------//

void snesInit();
int snesData();

#endif // _SNES_H_
