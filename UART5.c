// UART.c
// Runs on TM4C123
// Simple device driver for the UART. This is an example code for UART board to board communication.
// board to board communitation uses UART1
// By Dr.Min He
#include "UART.h"
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>  // for C boolean data type

//#define NVIC_EN0_UART1 0x40

//------------UART_Init------------
// Initialize the UART for 115,200 baud rate (assuming 50 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Input: none
// Output: none
void UART5_Init(bool RxInt, bool TxInt){
	SYSCTL_RCGCUART_R |= 0x20;  // activate UART5
	SYSCTL_RCGCGPIO_R |= 0x10;  // activate Port E
	while((SYSCTL_PRGPIO_R & 0x10) == 0){}; // wait for Port E
		
  UART5_CTL_R = 0;        // disable UART5 during setup

	UART5_IBRD_R = 162;      // 19200 baud rate  BRD = 50,000,000 / (16 * 19,200) = 162.7604167 = 162
	UART5_FBRD_R = 49;			// FBRD = (0.7604167 * 64) = 49 round

	UART5_LCRH_R = UART_LCRH_WLEN_8;

	if (RxInt) {
			UART5_IM_R |= UART_IM_RXIM;
	}

	if (TxInt) {
			UART5_IM_R |= UART_IM_TXIM;
	}

	UART5_CTL_R |= UART_CTL_UARTEN |
               UART_CTL_RXE |
               UART_CTL_TXE;
	
	if (RxInt | TxInt) {
    NVIC_PRI15_R = (NVIC_PRI15_R & 0xFFFF1FFF) | 0x0000A000; // UART5 priority 5
    NVIC_EN1_R |= 0x20000000;                               // enable UART5 interrupt 61
	}
		
	GPIO_PORTE_AFSEL_R |= 0x30;  // enable alternate function on PE4, PE5
	GPIO_PORTE_DEN_R   |= 0x30;  // enable digital I/O on PE4, PE5

	// PE4 = U5Rx, PE5 = U5Tx
	GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R & 0xFF00FFFF) | 0x00110000;

	GPIO_PORTE_AMSEL_R &= ~0x30; // disable analog on PE4, PE5
	}
//------------UART_InChar------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
	uint8_t UART5_InChar(void){
		while((UART5_FR_R & UART_FR_RXFE) != 0); // wait until receive FIFO is not empty
		return((uint8_t)(UART5_DR_R & 0xFF));
	}

	//------------UART_OutChar------------
	// Output 8-bit to serial port
	// Input: data is an 8-bit character to be transferred
	// Output: none
	void UART5_OutChar(uint8_t data){
		while((UART5_FR_R & UART_FR_TXFF) != 0); // wait if transmit FIFO is full
		UART5_DR_R = data;
	}
