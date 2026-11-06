// UARTTestMain.c
// Runs on LM4F120/TM4C123
// Used to test the UART.c driver
// Daniel Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// Modified by Dr. Min He on 9/23/2023

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "PLL.h"
#include "UART.h"
#include "UART5.h"
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>

// ================================================================
// CONFIGURATION
// ================================================================

#define MCU1_BOARD      0
#define PWM_LOAD        781
#define MODE3_MAX_LEN   30
#define MODE3_EXIT_CHAR 0x04

// ================================================================
// GLOBAL STATE
// ================================================================

// Shared LED state
static volatile uint8_t currentColor = 'd';
static uint32_t brightness = 100;

// Mode 2 state
static volatile bool mode2Active = false;
static volatile bool mode2MyTurn = false;
static volatile bool colorDisplayPending = false;
static volatile uint8_t mcu2ModeRequest = 0;

// Mode 3 state
static volatile bool mode3Active = false;
static volatile bool mode3ExitRequested = false;
static volatile uint8_t mode3RxBuffer[MODE3_MAX_LEN + 1];
static volatile uint8_t mode3RxIndex = 0;
static volatile bool mode3MessageReady = false;
static uint8_t mode3Buffer[MODE3_MAX_LEN + 1];

// ================================================================
// FUNCTION PROTOTYPES
// ================================================================

// Main / menu
void DisplayMenu(void);

// Mode 1
void Mode1(void);
void PWM_LED_Init(void);
void ApplyLED(void);
void SelectColor(void);
void SetBrightness(void);
uint8_t ReadMenuChoice(void);

// Mode 2
void Mode2(void);
void NextColor(void);
void Mode2_Switch_Init(void);
void PrintCurrentColor(void);

// Shared interrupt handlers
void GPIOPortF_Handler(void);
void UART5_Handler(void);
void UART0_Handler(void);

// Mode 3
void Mode3(void);
uint8_t Mode3_ReadMessage(uint8_t *buffer);


// ================================================================
// MAIN MENU + STARTUP
// ================================================================

void DisplayMenu(void) {

    OutCRLF();

    UART_OutString((uint8_t *)"MCU1 Main Menu");
    OutCRLF();

    UART_OutString((uint8_t *)"1. PC <-> MCU1 LED Control");
    OutCRLF();

    UART_OutString((uint8_t *)"2. MCU1 <-> MCU2 Color Wheel");
    OutCRLF();

    UART_OutString((uint8_t *)"3. PC1 <-> MCU1 <-> MCU2 <-> PC2 Chat Room");
    OutCRLF();

    UART_OutString((uint8_t *)"Please choose a communication mode (1, 2, or 3): ");

}

int main(void){

    uint8_t choice;

    PLL_Init();
#if MCU1_BOARD
    UART5_Init(false, false);
#else
    // MCU2 always listens for mode commands from MCU1
    UART5_Init(true, false);
#endif
    UART_Init(false, false, MCU1_BOARD);
    PWM_LED_Init();

    UART_OutString((uint8_t *)"Welcome to CECS 447 Project 2 - UART");

#if MCU1_BOARD

    while(1){

        DisplayMenu();

        choice = UART_InChar();
        OutCRLF();

        if(choice == '1'){
            Mode1();
        }

        else if(choice == '2'){

            UART5_OutChar('2');   // tell MCU2 to enter Mode 2
            Mode2();
        }

        else if(choice == '3'){

            UART5_OutChar('3');   // tell MCU2 to enter Mode 3
            Mode3();
        }

        else{
            UART_OutString((uint8_t *)"Invalid Selection");
        }

        OutCRLF();
    }

#else

    UART_OutString((uint8_t *)
    "\r\nWaiting for command from MCU1...");
    OutCRLF();

    while(1){

        if(mcu2ModeRequest == 2){

            mcu2ModeRequest = 0;
            Mode2();

            DisplayMenu();
            UART_OutString((uint8_t *)"\r\nWaiting for command from MCU1...");
            OutCRLF();
        }

        else if(mcu2ModeRequest == 3){

            mcu2ModeRequest = 0;
            Mode3();

            DisplayMenu();
            UART_OutString((uint8_t *)"\r\nWaiting for command from MCU1...");
            OutCRLF();
        }
    }

#endif
}

// ================================================================
// MODE 1 - PC <-> MCU1 LED CONTROL
// ================================================================

void Mode1(void){

    uint8_t choice;

    while(1){

        OutCRLF();
        UART_OutString((uint8_t *)
        "===== MODE 1: LED CONTROL =====");
        OutCRLF();

        UART_OutString((uint8_t *)
        "1. Select LED Color");
        OutCRLF();

        UART_OutString((uint8_t *)
        "2. Set Brightness");
        OutCRLF();

        UART_OutString((uint8_t *)
        "3. Exit to Main Menu");
        OutCRLF();

        UART_OutString((uint8_t *)
        "Enter choice (1, 2, or 3): ");

        choice = ReadMenuChoice();

        if(choice == '1'){

            SelectColor();
        }

        else if(choice == '2'){

            SetBrightness();
        }

        else if(choice == '3'){

            // Assignment requires LEDs off on exit
            currentColor = 'd';
            ApplyLED();

            UART_OutString((uint8_t *)
            "Returning to Main Menu...");
            OutCRLF();

            return;
        }

        else{

            UART_OutString((uint8_t *)
            "Invalid selection");
            OutCRLF();
        }
    }
}

void SelectColor(void){

    uint8_t input[2];

    UART_OutString((uint8_t *)"\r\nSelect LED Color:");
    OutCRLF();

    UART_OutString((uint8_t *)
    "d(dark), r(red), g(green), b(blue), "
    "y(yellow), c(cyan), p(purple), w(white)");
    OutCRLF();

    UART_OutString((uint8_t *)"Enter color: ");

    // Reads one letter and waits for Enter
    UART_InString(input, 1);
    OutCRLF();

    switch(input[0]){

        case 'd':
        currentColor = 'd';
        UART_OutString((uint8_t *)"LED is off");
        break;

        case 'r':
        currentColor = 'r';
        UART_OutString((uint8_t *)"Red LED is on");
        break;

        case 'g':
        currentColor = 'g';
        UART_OutString((uint8_t *)"Green LED is on");
        break;

        case 'b':
        currentColor = 'b';
        UART_OutString((uint8_t *)"Blue LED is on");
        break;

        case 'y':
        currentColor = 'y';
        UART_OutString((uint8_t *)"Yellow LED is on");
        break;

        case 'c':
        currentColor = 'c';
        UART_OutString((uint8_t *)"Cyan LED is on");
        break;

        case 'p':
        currentColor = 'p';
        UART_OutString((uint8_t *)"Purple LED is on");
        break;

        case 'w':
        currentColor = 'w';
        UART_OutString((uint8_t *)"White LED is on");
        break;

        default:
        UART_OutString((uint8_t *)"Invalid color");
        OutCRLF();
        return;
    }

    ApplyLED();
    OutCRLF();
}

void SetBrightness(void){

    uint32_t value;

    UART_OutString((uint8_t *)
    "\r\nEnter brightness (0-100): ");

    value = UART_InUDec();
    OutCRLF();

    if(value <= 100){

        brightness = value;

        ApplyLED();

        UART_OutString((uint8_t *)"Brightness set to ");
        UART_OutUDec(brightness);
        UART_OutString((uint8_t *)"%");
        OutCRLF();
    }
    else{

        UART_OutString((uint8_t *)
        "Invalid brightness. Enter 0-100.");
        OutCRLF();
    }

}

void PWM_LED_Init(void){

    // Enable PWM Module 1 and Port F
    SYSCTL_RCGCPWM_R |= 0x02;
    SYSCTL_RCGCGPIO_R |= 0x20;

    while((SYSCTL_PRPWM_R & 0x02) == 0){}
    while((SYSCTL_PRGPIO_R & 0x20) == 0){}

    // PWM clock = system clock / 64
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV;
    SYSCTL_RCC_R =
    (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M)
    | SYSCTL_RCC_PWMDIV_64;

    // PF1, PF2, PF3 use alternate functions
    GPIO_PORTF_AFSEL_R |= 0x0E;

    // PF1 = M1PWM5
    // PF2 = M1PWM6
    // PF3 = M1PWM7
    GPIO_PORTF_PCTL_R =
    (GPIO_PORTF_PCTL_R & ~0x0000FFF0)
    | 0x00005550;

    GPIO_PORTF_AMSEL_R &= ~0x0E;
    GPIO_PORTF_DEN_R |= 0x0E;
    GPIO_PORTF_DIR_R |= 0x0E;

    // Disable generators during setup
    PWM1_2_CTL_R = 0;
    PWM1_3_CTL_R = 0;

    // PWM5 = Generator 2 B
    PWM1_2_GENB_R = 0x0000080C;

    // PWM6 = Generator 3 A
    PWM1_3_GENA_R = 0x0000008C;

    // PWM7 = Generator 3 B
    PWM1_3_GENB_R = 0x0000080C;

    PWM1_2_LOAD_R = PWM_LOAD;
    PWM1_3_LOAD_R = PWM_LOAD;

    // Initially off
    PWM1_2_CMPB_R = PWM_LOAD;
    PWM1_3_CMPA_R = PWM_LOAD;
    PWM1_3_CMPB_R = PWM_LOAD;

    // Enable generators
    PWM1_2_CTL_R = 0x01;
    PWM1_3_CTL_R = 0x01;

    // Keep RGB outputs off initially
    PWM1_ENABLE_R &= ~0xE0;
}

void ApplyLED(void){

    uint32_t compare;
    uint32_t enableMask = 0;

    // Dark or 0% brightness = everything off
    if(currentColor == 'd' || brightness == 0){
        PWM1_ENABLE_R &= ~0xE0;
        return;
    }

    // Convert 0-100% brightness to PWM compare value
    compare = ((PWM_LOAD + 1) * (100 - brightness)) / 100;

    // Apply same brightness to R, B, and G channels
    PWM1_2_CMPB_R = compare;  // PF1 Red
    PWM1_3_CMPA_R = compare;  // PF2 Blue
    PWM1_3_CMPB_R = compare;  // PF3 Green

    switch(currentColor){

        case 'r':
        enableMask = 0x20;  // Red
        break;

        case 'b':
        enableMask = 0x40;  // Blue
        break;

        case 'g':
        enableMask = 0x80;  // Green
        break;

        case 'y':
        enableMask = 0xA0;  // Red + Green
        break;

        case 'c':
        enableMask = 0xC0;  // Blue + Green
        break;

        case 'p':
        enableMask = 0x60;  // Red + Blue
        break;

        case 'w':
        enableMask = 0xE0;  // Red + Blue + Green
        break;

        default:
        enableMask = 0;
        break;
    }

    PWM1_ENABLE_R =
    (PWM1_ENABLE_R & ~0xE0) | enableMask;
}

uint8_t ReadMenuChoice(void){

    uint8_t choice;

    do{
        choice = UART_InChar();
    }
    while(choice == CR || choice == LF);

    UART_OutChar(choice);
    OutCRLF();

    return choice;
}



// ================================================================
// MODE 2 - MCU1 <-> MCU2 COLOR WHEEL
// ================================================================

void Mode2(void){

    UART_OutString((uint8_t *)"\r\n===== MODE 2: COLOR WHEEL =====");
    OutCRLF();

    Mode2_Switch_Init();

    currentColor = 'd';
    brightness = 100;
    ApplyLED();

    mode2Active = true;

#if MCU1_BOARD
    mode2MyTurn = true;      // MCU1 starts active
#else
    mode2MyTurn = false;     // MCU2 waits for MCU1
#endif

#if MCU1_BOARD
    // Enable UART0 RX interrupt for '^'
    UART0_ICR_R = UART_ICR_RXIC;
    UART0_IM_R |= UART_IM_RXIM;

    NVIC_PRI1_R =
    (NVIC_PRI1_R & ~0x0000E000) | 0x0000A000;

    // IMPORTANT: |= preserves the GPIO Port F interrupt enable
    NVIC_EN0_R |= 0x20;
#endif

    // Enable UART5 RX interrupt during Mode 2
    UART5_Init(true, false);

    while(mode2Active){

        if(colorDisplayPending){
            colorDisplayPending = false;
            PrintCurrentColor();
        }
    }

#if MCU1_BOARD
    // MCU1 only needs UART5 RX while inside Mode 2
    UART5_IM_R &= ~UART_IM_RXIM;

    // Disable UART0 RX interrupt after leaving Mode 2
    UART0_IM_R &= ~UART_IM_RXIM;
#else
    // MCU2 must continue listening for commands from MCU1
    UART5_IM_R |= UART_IM_RXIM;
#endif

    currentColor = 'd';
    ApplyLED();
}

void NextColor(void){

    switch(currentColor){

        case 'd':
        currentColor = 'r';
        break;

        case 'r':
        currentColor = 'g';
        break;

        case 'g':
        currentColor = 'b';
        break;

        case 'b':
        currentColor = 'y';
        break;

        case 'y':
        currentColor = 'c';
        break;

        case 'c':
        currentColor = 'p';
        break;

        case 'p':
        currentColor = 'w';
        break;

        case 'w':
        default:
        currentColor = 'd';
        break;
    }

    ApplyLED();
    colorDisplayPending = true;
}

void PrintCurrentColor(void){

    UART_OutString((uint8_t *)"Current Color: ");

    switch(currentColor){

        case 'd':
        UART_OutString((uint8_t *)"Dark");
        break;

        case 'r':
        UART_OutString((uint8_t *)"Red");
        break;

        case 'g':
        UART_OutString((uint8_t *)"Green");
        break;

        case 'b':
        UART_OutString((uint8_t *)"Blue");
        break;

        case 'y':
        UART_OutString((uint8_t *)"Yellow");
        break;

        case 'c':
        UART_OutString((uint8_t *)"Cyan");
        break;

        case 'p':
        UART_OutString((uint8_t *)"Purple");
        break;

        case 'w':
        UART_OutString((uint8_t *)"White");
        break;
    }

    OutCRLF();
}

void Mode2_Switch_Init(void){

    SYSCTL_RCGCGPIO_R |= 0x20;          // Enable Port F clock
    while((SYSCTL_PRGPIO_R & 0x20) == 0){}

    // Unlock PF0 so SW2 can be used
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= 0x11;            // Allow changes to PF0 and PF4

    // PF0 = SW2, PF4 = SW1
    GPIO_PORTF_DIR_R &= ~0x11;          // Set as inputs
    GPIO_PORTF_AFSEL_R &= ~0x11;        // Use GPIO
    GPIO_PORTF_AMSEL_R &= ~0x11;        // Disable analog
    GPIO_PORTF_PCTL_R &= ~0x000F000F;   // Clear alternate functions
    GPIO_PORTF_DEN_R |= 0x11;           // Digital enable
    GPIO_PORTF_PUR_R |= 0x11;           // Enable pull-up resistors

    // Configure interrupts for falling edge
    GPIO_PORTF_IS_R &= ~0x11;           // Edge-sensitive
    GPIO_PORTF_IBE_R &= ~0x11;          // Not both edges
    GPIO_PORTF_IEV_R &= ~0x11;          // Falling edge

    GPIO_PORTF_ICR_R = 0x11;            // Clear previous interrupt flags
    GPIO_PORTF_IM_R |= 0x11;            // Arm PF0 and PF4 interrupts

    // Set GPIO Port F interrupt priority
    NVIC_PRI7_R =
    (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000;

    // Enable GPIO Port F interrupt
    NVIC_EN0_R |= 0x40000000;
}



// ================================================================
// SHARED INTERRUPT HANDLERS
// Used by Mode 2 and/or Mode 3
// ================================================================

void GPIOPortF_Handler(void){

    // SW2 = PF0: cycle color only when it is this board's turn
    if(GPIO_PORTF_RIS_R & 0x01){

        GPIO_PORTF_ICR_R = 0x01;

        if(mode2Active && mode2MyTurn){
            NextColor();
        }
    }

    // SW1 = PF4
    if(GPIO_PORTF_RIS_R & 0x10){

        GPIO_PORTF_ICR_R = 0x10;

        // ===== MODE 3 EXIT =====
        if(mode3Active){

            // Tell the other MCU to leave the chat room
            UART5_OutChar(MODE3_EXIT_CHAR);

            mode3Active = false;
            mode3ExitRequested = true;

            return;
        }

        // ===== MODE 2 SEND COLOR =====
        if(mode2Active && mode2MyTurn){

            UART5_OutChar(currentColor);

            mode2MyTurn = false;
        }
    }
}

void UART5_Handler(void){

    uint8_t received;

    if(UART5_RIS_R & UART_RIS_RXRIS){

        received = (uint8_t)(UART5_DR_R & 0xFF);

        UART5_ICR_R = UART_ICR_RXIC;

#if !MCU1_BOARD

        // MCU2 receives command from MCU1 to enter Mode 2
        if(!mode2Active && !mode3Active && received == '2'){
            mcu2ModeRequest = 2;
            return;
        }

        // MCU2 receives command from MCU1 to enter Mode 3
        if(!mode2Active && !mode3Active && received == '3'){
            mcu2ModeRequest = 3;
            return;
        }

#endif

        // ===== MODE 3 REMOTE EXIT =====
        if(mode3Active && received == MODE3_EXIT_CHAR){

            mode3Active = false;
            mode3ExitRequested = true;

            mode3RxIndex = 0;
            mode3MessageReady = false;

            return;
        }

        // ===== MODE 3 MESSAGE RECEIVE =====
        if(mode3Active){

            if(received == CR){

                mode3RxBuffer[mode3RxIndex] = '\0';
                mode3RxIndex = 0;
                mode3MessageReady = true;

                return;
            }

            if(mode3RxIndex < MODE3_MAX_LEN){

                mode3RxBuffer[mode3RxIndex] = received;
                mode3RxIndex++;
            }

            return;
        }

        // ===== MODE 2 EXIT =====
        if(received == '^'){

            mode2Active = false;
            mode2MyTurn = false;
            return;
        }

        // ===== MODE 2 COLOR RECEIVE =====
        if(mode2Active){

            switch(received){

                case 'd':
                case 'r':
                case 'g':
                case 'b':
                case 'y':
                case 'c':
                case 'p':
                case 'w':

                currentColor = received;
                brightness = 100;
                ApplyLED();

                mode2MyTurn = true;
                colorDisplayPending = true;

                break;
            }
        }
    }
}

void UART0_Handler(void){

    uint8_t received;

    if(UART0_RIS_R & UART_RIS_RXRIS){

        received = (uint8_t)(UART0_DR_R & 0xFF);

        UART0_ICR_R = UART_ICR_RXIC;

#if MCU1_BOARD
        if(mode2Active && received == '^'){

            // Tell MCU2 to exit too
            UART5_OutChar('^');

            mode2Active = false;
            mode2MyTurn = false;
        }
#endif
    }
}



// ================================================================
// MODE 3 - PC1 <-> MCU1 <-> MCU2 <-> PC2 CHAT ROOM
// ================================================================

uint8_t Mode3_ReadMessage(uint8_t *buffer){

    uint8_t index = 0;
    uint8_t ch;

    while(mode3Active){

        // Check UART0 without blocking
        if((UART0_FR_R & UART_FR_RXFE) == 0){

            ch = (uint8_t)(UART0_DR_R & 0xFF);
				// Type ^ to exit Mode 3
				if(ch == '^'){

							UART5_OutChar(MODE3_EXIT_CHAR);

							mode3Active = false;
							mode3ExitRequested = true;

							OutCRLF();
							return 0;
						}
            // Enter finishes the message
            if(ch == CR){
                OutCRLF();
                break;
            }

            // Ignore line feed
            if(ch == LF){
                continue;
            }

            // Backspace
            if((ch == 0x08 || ch == 0x7F) && index > 0){

                index--;

                UART_OutChar(0x08);
                UART_OutChar(' ');
                UART_OutChar(0x08);

                continue;
            }

            // Only accept up to 30 characters
            if(index < MODE3_MAX_LEN){

                buffer[index] = ch;
                index++;

                // Echo character so you can see what you're typing
                UART_OutChar(ch);
            }
        }
    }

    buffer[index] = '\0';

    return index;
}

void Mode3(void){

    uint8_t i;
    uint8_t j;

    mode3Active = true;
    mode3ExitRequested = false;
    mode3MessageReady = false;
    mode3RxIndex = 0;

    // We need SW1 interrupt for exiting Mode 3
    Mode2_Switch_Init();

    // Enable board-to-board reception
    UART5_Init(true, false);

    UART_OutString((uint8_t *)
    "\r\n===== MODE 3: PC CHAT ROOM =====");
    OutCRLF();

#if MCU1_BOARD

    // MCU1 starts the conversation
    while(mode3Active){

        UART_OutString((uint8_t *)
        "\r\nPC1 - Enter message (max 30 chars): ");

        i = Mode3_ReadMessage(mode3Buffer);

        // SW1 may have been pressed while typing
        if(!mode3Active){
            break;
        }

        // Send message to MCU2
        for(j = 0; j < i; j++){
            UART5_OutChar(mode3Buffer[j]);
        }

        // End-of-message marker
        UART5_OutChar(CR);

        UART_OutString((uint8_t *)
        "\r\nWaiting for PC2...");
        OutCRLF();

        // Wait for PC2's reply
        while(mode3Active && !mode3MessageReady){
        }

        if(!mode3Active){
            break;
        }

        mode3MessageReady = false;

        UART_OutString((uint8_t *)"\r\nPC2: ");
        UART_OutString((uint8_t *)mode3RxBuffer);
        OutCRLF();
    }

#else

    // MCU2 waits for MCU1's first message
    while(mode3Active){

        UART_OutString((uint8_t *)
        "\r\nWaiting for PC1...");
        OutCRLF();

        while(mode3Active && !mode3MessageReady){
        }

        if(!mode3Active){
            break;
        }

        mode3MessageReady = false;

        UART_OutString((uint8_t *)"\r\nPC1: ");
        UART_OutString((uint8_t *)mode3RxBuffer);
        OutCRLF();

        UART_OutString((uint8_t *)
        "PC2 - Enter reply (max 30 chars): ");

        i = Mode3_ReadMessage(mode3Buffer);

        if(!mode3Active){
            break;
        }

        // Send reply back to MCU1
        for(j = 0; j < i; j++){
            UART5_OutChar(mode3Buffer[j]);
        }

        UART5_OutChar(CR);
    }

#endif

    // ===== MODE 3 CLEANUP =====

    UART_OutString((uint8_t *)
    "\r\nChat room ended. Returning to Main Menu...");
    OutCRLF();

    mode3RxIndex = 0;
    mode3MessageReady = false;

#if MCU1_BOARD

    // MCU1 doesn't need UART5 reception while sitting at main menu
    UART5_IM_R &= ~UART_IM_RXIM;

#else

    // MCU2 must keep listening for the next mode command
    UART5_IM_R |= UART_IM_RXIM;

#endif

    mode3Active = false;
    mode3ExitRequested = false;
}
