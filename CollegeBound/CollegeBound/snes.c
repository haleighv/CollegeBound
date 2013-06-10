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
 * This function initializes the use of PORT A, B, or both A & B to interface
 * with the SNES controller of choice. See the #define section for which
 * pins are tied to which signal.
 **/
void snesInit(uint8_t num_players)
{
	// Configure Port for player1
	if(num_players == SNES_P1)
	{
		//data = input; clock = output; latch = output
		SNES_DDR_P1 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
		SNES_PORT_P1 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
	}
	
	// Configure Port for player2 
	else if(num_players == SNES_P2)
	{		
		//data = input; clock = output; latch = output
		SNES_DDR_P2 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
		SNES_PORT_P2 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
	}
   // Configure Port for player2 and player 1
   else if(num_players == SNES_2P_MODE)
   {
      //data = input; clock = output; latch = output
      SNES_DDR_P1 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
      SNES_PORT_P1 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
      //data = input; clock = output; latch = output
      SNES_DDR_P2 = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
      SNES_PORT_P2 = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
   }
}

uint16_t snesData(uint8_t player_num)
{	
	uint8_t i;
	uint16_t data = 0;

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