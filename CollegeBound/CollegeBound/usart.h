/***************************
* Filename: usart.h
*
* Description: Provides print methods for various
*              datatypes using USART.
*
* Authors: Doug Gallatin and Jason Schray
*
* Revisions:
* 5/10/12 HAV Added USART_Write_Task
*
***************************/
#ifndef USART_H_
#define USART_H_

#define QUEUE_SIZE 300

uint8_t USART_Read(void);
void USART_Write(uint8_t data);
void USART_Write_Unprotected(uint8_t data);
void USART_Init(uint16_t baudin, uint32_t clk_speedin);
void USART_Queue_Reset(void);
void USART_Let_Queue_Empty(void);
void USART_Write_Task(void *vParam);


#endif /* USART_H_ */
