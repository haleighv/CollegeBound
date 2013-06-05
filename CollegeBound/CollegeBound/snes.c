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
void snesInit()
{
	// Create Queue for snes controller data
	xSnesDataQueue = xQueueCreate( 1, sizeof(uint16_t));
	
	
	//data = input; clock = output; latch = output
	SNES_DDR = ~(1<<DATA) | (1<<CLK) | (1<<LATCH);
	SNES_PORT = ~(1<<LATCH) | (1<<CLK);    //latch idle low, clk idle high

	timer_mode = TIMER_16_67MS_MODE;
	num_bits_received = 0;

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

uint16_t snesData()
{
	/**
	* This is a table for which buttons correspond to the
	* bits in the returned value.
	* Returned Bit Button Reported
	* ============ ===============
	* 11 B
	* 10 Y
	* 9 Select
	* 8 Start
	* 7 Up on joypad
	* 6 Down on joypad
	* 5 Left on joypad
	* 4 Right on joypad
	* 3 A
	* 2 X
	* 1 L
	* 0 R
	**/
	
	uint8_t i;
	uint16_t data = 0;
	
	//latch signal (idle low) needs to be high for 12us
	SNES_PORT |= (1<<LATCH);
	_delay_us(LATCH_TIME);
	SNES_PORT &= ~(1<<LATCH); //latch idle low
	
	//following the latch signal is 12 clock signals (idle high)
	for (i=0; i<NUM_BTNS; i++)
	{
		_delay_us(CLK_TIME);
		SNES_PORT &= ~(1<<CLK); //clock low

		//reads data from the controller and bitmask to the LSB
		data |= (SNES_PIN & (1<<DATA))>>DATA;
		data <<= 1; //shifts data to the right to allow for the next reading

		_delay_us(CLK_TIME);
		SNES_PORT |= (1<<CLK); //clock high, idles high when loop exits
	}
	
	//the returned data is inverted for active high
	return (~(data>>1) & 0xFFF); //0xFFF is used to mask away unwanted bits
}


ISR(TIMER4_COMPA_vect)
{
	if(timer_mode == TIMER_12US_MODE)
	{
		// if(num_bits_received != 0)
			// num_errors++; 
		
		SNES_PORT &= ~(1<<LATCH);     //latch idle low
		
		
		
		
		OCR4A = 261;
		timer_mode = TIMER_DATA_CLK_STOP_MODE;
		
		
		// Enable compB interrupt
		OCR4B = TCNT4 + TIMERCOMP_5_US;
		TIMSK4 |= (1<<OCIE4B);
	}
	else if(timer_mode == TIMER_16_67MS_MODE)
	{
		//latch signal (idle low) needs to be high for 12us
		SNES_PORT |= (1<<LATCH);
		OCR4A = TIMERCOMP_12US;
		timer_mode = TIMER_12US_MODE;
		TCNT4 = 0;
	}
	else if(timer_mode == TIMER_DATA_CLK_STOP_MODE)
	{
		// Disable compB interrupt
		TIMSK4 &= ~(1<<OCIE4B);
		OCR4B = 0;
		temp_snes_data = ~(temp_snes_data>>1) & 0xFFF;
		//xQueueSendToBack( xSnesDataQueue, &temp_snes_data, 0);
		temp_snes_data = 0;
		//num_bits_received = 0;
		SNES_PORT = (1<<CLK); 
				
		OCR4A = TIMERCOMP_16_67MS;
		timer_mode = TIMER_16_67MS_MODE;		
	}		
	else
		timer_mode = TIMER_16_67MS_MODE;
}

ISR(TIMER4_COMPB_vect)
{
	//static temp_snes_data = 0;
	
	OCR4B = TCNT4 + TIMERCOMP_5_US;
	SNES_PORT ^= (1<<CLK);     // Toggle clock, idles high
	
	//reads data from the controller and bitmask to the LSB
	temp_snes_data |= (SNES_PIN & (1<<DATA))>>DATA;
	temp_snes_data <<= 1;    //shifts data to the left to allow for the next reading
	//++num_bits_received;
	

	
	//if(num_bits_received == (24))
	//{
		//// Disable compB interrupt
		//TIMSK4 &= ~(1<<OCIE4B);
		//OCR4B = 0;
		//temp_snes_data = ~(temp_snes_data>>1) & 0xFFF;
		//xQueueSendToBack( xSnesDataQueue, &temp_snes_data, 0);
		////temp_snes_data = 0;
		////num_bits_received = 0;
	//}
	
	// Set to re-enter in 5US
	
	
	
	// OCR4B = TCNT4 + TIMERCOMP_5_US;
	// SNES_PORT ^= (1<<CLK);     // Toggle clock, idles high
	// portBASE_TYPE xStatus;
	// xStatus = xQueueSendToBack( xSnesDataQueue, SNES_PIN, 0);
	// if( xStatus != pdPASS )
	// {
		// TIMSK4 &= ~(1<<OCIE4B);
		// OCR4B = 0;
	// }
}