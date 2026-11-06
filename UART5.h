// UART.h
// Runs on TM4C123, device driver for the UART.
// board to board communitation use UART5
// Ground connected ground in the USB cable
// By. Dr. Min He

#include <stdbool.h>  // for C boolean data type

// standard ASCII symbols
#define CR   0x0D
#define LF   0x0A
#define BS   0x08
#define ESC  0x1B
#define SP   0x20
#define DEL  0x7F

//------------UART_Init------------
// Initialize the UART for 115,200 baud rate (assuming 50 MHz clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Input: none
// Output: none
void UART5_Init(bool RxInt, bool TxInt);

//------------UART_InChar------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
unsigned char UART5_InChar(void);

//------------UART_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART5_OutChar(unsigned char data);
