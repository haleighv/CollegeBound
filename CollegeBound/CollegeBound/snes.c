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
 #include "snes.h"
 
/**
 * This function initializes the use of PORTB to interface
 * with the SNES controller. See the #define section for which
 * pins are tied to which signal.
 **/
void snesInit(uint8_t num_players)
{
	// Create Queue for snes controller data
	//xSnesDataQueue = xQueueCreate( 1, sizeof(uint16_t));
	
	// Configure Port for player1
	if(num_players == 1)
	{
		//data = input; clock = output; latch = output
		SNES_DDR_P1 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
		SNES_PORT_P1 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
	}
	
	// Configure Port for player2 and player 1
	else if(num_players == 2)
	{
		//data = input; clock = output; latch = output
		SNES_DDR_P1 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
		SNES_PORT_P1 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
		
		//data = input; clock = output; latch = output
		SNES_DDR_P2 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
		SNES_PORT_P2 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
	}



	//timer_mode = TIMER_16_67MS_MODE;
	//num_bits_received = 0;

	//// Initialize OCR4A to clear every 16.67ms
	//OCR4A = TIMER_16_67MS_MODE;
	//OCR4B = 0;
//
	//// Enable CTC on timer 4 with prescaler of 8
	//TCCR4B |= (1<<WGM42)|(1<<CS41);
//
	//// Enable interrupt vector on OC4A
	//TIMSK4 |= (1<<OCIE4A);
}

uint16_t snesData(uint8_t player_num)
{	
	uint8_t i;
	uint16_t data = 0;
	//volatile uint8_t* SNES_PORT;
	//volatile uint8_t* SNES_PIN;
	//
	//if(player_num == 1)
	//{
		//SNES_PORT = &SNES_PORT_P1;
		//SNES_PIN = &SNES_PORT_P1;
	//}
	//else if(player_num == 2)
	//{
		//SNES_PORT = &SNES_PORT_P2;
		//SNES_PIN = &SNES_PORT_P2;
	//}
	//else
		//return; 

   switch(player_num)
   {
      case SNES_P1:
         //latch signal (idle low) needs to be high for 12us
         SNES_PORT_P1 |= (1<<LATCH);
         _delay_us(LATCH_TIME);
         SNES_PORT_P1 &= ~(1<<LATCH); //latch idle low
         
         //following the latch signal is 12 clock signals (idle high)
         for (i=0; i<NUM_BTNS; i++)
         {
            _delay_us(CLK_TIME);
            SNES_PORT_P1 &= ~(1<<CLK); //clock low

            //reads data from the controller and bitmask to the LSB
            data |= (SNES_PIN_P1 & (1<<DATA))>>DATA;
            data <<= 1; //shifts data to the right to allow for the next reading

            _delay_us(CLK_TIME);
            SNES_PORT_P1 |= (1<<CLK); //clock high, idles high when loop exits
         }
         break;
      case SNES_P2:
          //latch signal (idle low) needs to be high for 12us
          SNES_PORT_P2 |= (1<<LATCH);
          _delay_us(LATCH_TIME);
          SNES_PORT_P2 &= ~(1<<LATCH); //latch idle low
          
          //following the latch signal is 12 clock signals (idle high)
          for (i=0; i<NUM_BTNS; i++)
          {
             _delay_us(CLK_TIME);
             SNES_PORT_P2 &= ~(1<<CLK); //clock low

             //reads data from the controller and bitmask to the LSB
             data |= (SNES_PIN_P2 & (1<<DATA))>>DATA;
             data <<= 1; //shifts data to the right to allow for the next reading

             _delay_us(CLK_TIME);
             SNES_PORT_P2 |= (1<<CLK); //clock high, idles high when loop exits
          }
          break;
      default:
         break;
   }
	//the returned data is inverted for active high
	return (~(data>>1) & 0xFFF); //0xFFF is used to mask away unwanted bits
}