/**
 * Author: Matt Cruse, Haleigh Vierra
 *
 * Project: DeathTanks
 *
 * Description: This is the driver that was designed to use an SNES controller
 * with the STK600. The SNES controller has a shift register that latches
 * the button status and transmits a button sequence off of a clock cycle. This
 * code replicates the interface that the SNES console uses with the controllers.
 **/
#ifndef _SNES_H_
#define _SNES_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "shares.h"

#define SNES_1P_MODE 1
#define SNES_2P_MODE 2

//uses PORTA for Player1
#define SNES_DDR_P1     DDRA
#define SNES_PORT_P1    PORTA
#define SNES_PIN_P1     PINA
#define SNES_P1         1

//uses PORTB for Player2
#define SNES_DDR_P2     DDRB
#define SNES_PORT_P2    PORTB
#define SNES_PIN_P2     PINB
#define SNES_P2         2


#define LATCH        0        //PB0
#define CLK          1        //PB1
#define DATA         2        //PB2

// Trial and error vals b/c us delays were not accurate after the scheduler ran
#define LATCH_TIME   191//12       //time [us] for the latch signal
#define CLK_TIME     84//5.6      //time [us] for 1/2 the clock (50% duty)
#define NUM_BTNS     12       //the total number of buttons on the controller

#define SNES_R_BTN		(1<<0)
#define SNES_L_BTN		(1<<1)
#define SNES_X_BTN		(1<<2)
#define SNES_A_BTN		(1<<3)
#define SNES_RIGHT_BTN	(1<<4)
#define SNES_LEFT_BTN	(1<<5)
#define SNES_DOWN_BTN	(1<<6)
#define SNES_UP_BTN		(1<<7)
#define SNES_STRT_BTN	(1<<8)
#define SNES_SEL_BTN	   (1<<9)
#define SNES_Y_BTN		(1<<10)
#define SNES_B_BTN		(1<<11)


//----------------------------Function Prototypes----------------------------//
void snesInit(uint8_t);
uint16_t snesData(uint8_t);
#endif // _SNES_H_
