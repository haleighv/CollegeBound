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
 
 
/**
 * This function initializes the use of PORTB to interface
 * with the SNES controller. See the #define section for which
 * pins are tied to which signal.
 **/
void snesInit()
{
   //data = input; clock = output; latch = output
   SNES_DDR = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
   SNES_PORT = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high
}

/**
 * This function will produce the latch and clock signals
 * needed to operate the SNES controller's bit shift register.
 * The buttons are active low so before returning, the data is 
 * inverted to be active high.
 *
 * return: an interger with the 12 LSB representing active high
 *    status of each button on the controller.
 **/
int snesData()
{
   /**
   * This is a table for which buttons correspond to the 
   * bits in the returned value.
   *     Returned Bit     Button Reported
   *     ============     ===============
   *     11               B
   *     10               Y
   *     9                Select
   *     8                Start
   *     7                Up on joypad
   *     6                Down on joypad
   *     5                Left on joypad
   *     4                Right on joypad
   *     3                A
   *     2                X
   *     1                L
   *     0                R
   **/
   
   int i;
   int data = 0;
   
   //latch signal (idle low) needs to be high for 12us
   SNES_PORT |= (1<<LATCH);
   _delay_us(LATCH_TIME);
   SNES_PORT &= ~(1<<LATCH);     //latch idle low
   
   //following the latch signal is 12 clock signals (idle high)
   for (i=0; i<NUM_BTNS; i++)
	{
      _delay_us(CLK_TIME);
		SNES_PORT &= ~(1<<CLK);    //clock low
		
      //reads data from the controller and bitmask to the LSB
      data |= (SNES_PIN & (1<<DATA))>>DATA;
      data <<= 1;    //shifts data to the right to allow for the next reading

      _delay_us(CLK_TIME);
		SNES_PORT |= (1<<CLK);     //clock high, idles high when loop exits
	}
   
   //the returned data is inverted for active high
   return (~(data>>1) & 0xFFF);     //0xFFF is used to mask away unwanted bits
}