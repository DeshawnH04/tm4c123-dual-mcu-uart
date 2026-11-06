# Dual-MCU UART Communication System

An embedded C project that connects **two Texas Instruments TM4C123 LaunchPad microcontrollers** and **two PC serial terminals** using three independent UART links. The system demonstrates register-level peripheral configuration, interrupt-driven communication, PWM-based RGB LED control, and message exchange between microcontrollers.

Developed for **CECS 447 — Project 2: UART Communication System**.

## Features

The application provides three menu-driven communication modes:

| Mode | Function | Description |
| --- | --- | --- |
| **1 — LED Control** | PC ↔ MCU1 | Select the onboard RGB LED color and adjust brightness from 0–100% using hardware PWM. |
| **2 — Color Wheel** | MCU1 ↔ MCU2 | Cycle through LED colors with **SW2** and transfer control to the other board with **SW1**. UART interrupts handle messages and mode termination. |
| **3 — Serial Chat Room** | PC1 ↔ MCU1 ↔ MCU2 ↔ PC2 | Exchange messages of up to **30 characters** through both microcontrollers, with coordinated exit handling. |

Additional capabilities include a **50 MHz system clock configured through the PLL**, dedicated UART drivers for UART0 and UART5, GPIO switch interrupts, and interactive menus over serial terminals.

## System Architecture

```text
  PC Terminal 1         TM4C123 MCU1           TM4C123 MCU2         PC Terminal 2
  +-----------+         +------------+         +------------+        +-----------+
  |           | <--->   |   UART0    |         |   UART0    |  <---> |           |
  |  Serial   |         |            |         |            |        |  Serial   |
  |  Terminal |         |   UART5    | <-----> |   UART5    |        |  Terminal |
  +-----------+         +------------+         +------------+        +-----------+
                         Master board          Secondary board
```

**MCU1** displays the main menu and initiates Modes 2 and 3. **MCU2** waits for mode commands from MCU1.

### UART Configuration

The source code currently configures the following baud rates, using a 50 MHz system clock:

| Connection | Interface | Baud rate |
| --- | --- | ---: |
| PC Terminal 1 ↔ MCU1 | UART0 (PA0/PA1) | 230400 |
| MCU1 ↔ MCU2 | UART5 (PE4/PE5) | 19200 |
| MCU2 ↔ PC Terminal 2 | UART0 (PA0/PA1) | 460800 |

The UART drivers use **8 data bits, no parity, and 1 stop bit (8N1)**. Configure each PC terminal to match its board's baud rate.

## Hardware and Software

**Hardware**
- 2 × TI TM4C123 LaunchPad development boards
- 2 × USB connections to PCs/serial terminals
- Jumper wires for the board-to-board UART5 connection
- Onboard RGB LEDs and pushbuttons (SW1/SW2)

**Software**
- Keil µVision / MDK-ARM (or a compatible TM4C123 embedded C toolchain)
- Serial terminal software, such as PuTTY or Tera Term
- Appropriate TM4C123 device support and startup files for your toolchain

### Board-to-Board Wiring

Connect **UART5 TX to the other board's UART5 RX** in both directions, and connect the boards' grounds:

| MCU1 pin | MCU2 pin |
| --- | --- |
| PE5 (UART5 TX) | PE4 (UART5 RX) |
| PE4 (UART5 RX) | PE5 (UART5 TX) |
| GND | GND |

UART0 uses the LaunchPad's virtual COM connection (PA0/PA1) to communicate with each PC.

## Build and Run

1. Create a Keil project targeting the **TM4C123GH6PM**, or open your existing project for this MCU.
2. Add the C sources and headers from this repository, along with the device startup/support files required by your toolchain.
3. **Rename the uploaded `PLL(2).c` and `PLL(2).h` files to `PLL.c` and `PLL.h`** before building, since the code includes `PLL.h`.
4. In `UARTTestMain.c`, set the board selection macro for the first build:

   ```c
   #define MCU1_BOARD 1  // Build firmware for MCU1
   ```

5. Compile and flash that build to **MCU1**. Then change the macro and rebuild:

   ```c
   #define MCU1_BOARD 0  // Build firmware for MCU2
   ```

6. Flash the second build to **MCU2**. Connect the UART5 pins and common ground as shown above.
7. Open two serial terminals, selecting the appropriate virtual COM ports and baud rates (**230400** for MCU1, **460800** for MCU2). Use **8N1** serial settings.
8. Reset or run both boards. MCU1 presents the main menu; MCU2 waits for a mode command.

> **Note:** The repository source files are provided without a complete Keil project configuration or startup file. You will need to supply the appropriate toolchain/device setup. The firmware is intended for the physical TM4C123 LaunchPads.

## Using the Application

### Mode 1 — PWM RGB LED Control

On PC Terminal 1, select **Mode 1**. You can choose one of eight states: dark, red, green, blue, yellow, cyan, purple, or white. The brightness menu accepts a value from **0 to 100%** and updates the PWM output. Exiting the mode turns the LEDs off.

### Mode 2 — Dual-MCU Color Wheel

Select **Mode 2** on MCU1's terminal. The active board uses **SW2 (PF0)** to cycle through colors and **SW1 (PF4)** to send the current color to the other board. The receiving board becomes active and continues from the transferred color. Enter **`^`** on PC Terminal 1 to end this mode and turn off the LEDs.

### Mode 3 — Two-Way Serial Chat

Select **Mode 3** on MCU1's terminal. Enter a message of up to **30 characters** on PC Terminal 1; it is forwarded over UART5 and displayed on PC Terminal 2. PC Terminal 2 can then send a reply. Press **SW1** on either board to exit. The source also provides a `^` exit shortcut while typing a message.

## Repository Files

```text
.
├── UARTTestMain.c       # Main application, menus, modes, and interrupt handlers
├── UART.c               # UART0 serial-terminal driver
├── UART.h
├── UART5.c              # UART5 board-to-board communication driver
├── UART5.h
├── PLL.c                # System clock / PLL setup (rename from PLL(2).c)
├── PLL.h                # PLL configuration (rename from PLL(2).h)
├── tm4c123gh6pm.h      # TI TM4C123 register definitions
└── README.md
```

## Concepts Demonstrated

- Bare-metal embedded C and memory-mapped register programming
- UART serial communication across multiple devices
- Interrupt service routines (UART and GPIO)
- PWM-based RGB LED brightness control
- GPIO button input and control handoff between microcontrollers
- Multi-mode embedded application organization

## Acknowledgments

This project was developed for **CECS 447**, based on assignment requirements provided by **Dr. Min He**. Some UART and PLL support code is based on educational material by **Jonathan W. Valvano** and course-provided drivers; the TM4C123 register header is from **Texas Instruments**. Please preserve the original attribution, copyright notices, and license terms in these files.
