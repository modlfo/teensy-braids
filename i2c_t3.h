/*
    ------------------------------------------------------------------------------------------------------
    i2c_t3 - I2C library for Teensy3, derived from Teensy3 TwoWire library

    - Modified 16Jan14 by Brian (nox771 at gmail.com)
        - all new structure using dereferenced pointers instead of hardcoding. This allows functions
          (including ISRs) to be reused across multiple I2C buses.  Most functions moved to static,
          which in turn are called by inline user functions.  Added new struct (i2cData) for holding all
          bus information.
        - added support for Teensy 3.1 and I2C1 interface on pins 29/30 and 26/31.
        - added header define (I2C_BUS_ENABLE n) to control number of enabled buses (eg. both I2C0 & I2C1
          or just I2C0).  When using only I2C0 the code and ram usage will be lower.
        - added interrupt flag (toggles pin high during ISR) with independent defines for I2C0 and
          I2C1 (refer to header file), useful for logic analyzer trigger

    - Modified 09Jun13 by Brian (nox771 at gmail.com)
        - fixed bug in ISR timeout code in which timeout condition could fail to reset in certain cases
        - fixed bug in Slave mode in sda_rising_isr attach, whereby it was not getting attached on the addr byte
        - moved debug routines so they are entirely defined internal to the library (no end user code req'd)
        - debug routines now use IntervalTimer library
        - added support for range of Slave addresses
        - added getRxAddr() for Slave using addr range to determine its called address
        - removed virtual keyword from all functions (is not a base class)

    - Modified 26Feb13 by Brian (nox771 at gmail.com)
        - Reworked begin function:
            - added option for pins to use (SCL:SDA on 19:18 or 16:17 - note pin order difference)
            - added option for internal pullup - as mentioned in previous code pullup is very strong,
                                                 approx 190 ohms, but is possibly useful for high speed I2C
            - added option for rates - 100kHz, 200kHz, 300kHz, 400kHz, 600kHz, 800kHz, 1MHz, 1.2MHz, <-- 24/48MHz bus
                                       1.5MHz, 2.0MHz, 2.4MHz                                        <-- 48MHz bus only
        - Removed string.h dependency (memcpy)
        - Changed Master modes to interrupt driven
        - Added non-blocking Tx/Rx routines, and status/done/finish routines:
            - sendTransmission() - non-blocking transmit
            - sendRequest() - non-blocking receive
            - status() - reports current status
            - done() - indicates Tx/Rx complete (for main loop polling if I2C is running in background)
            - finish() - loops until Tx/Rx complete or bus error
        - Added readByte()/peekByte() for uint8_t return values (note: returns 0 instead of -1 if buf empty)
        - Added fixes for Slave Rx mode - in short Slave Rx on this part is fubar
          (as proof, notice the difference in the I2Cx_FLT register in the KL25 Sub-Family parts)
            - the SDA-rising ISR hack can work but only detects STOP conditons.
              A slave Rx followed by RepSTART won't be detected since bus remains busy.
              To fix this if IAAS occurs while already in Slave Rx mode then it will
              assume RepSTART occurred and trigger onReceive callback.
        - Separated Tx/Rx buffer sizes for asymmetric devices (adjustable in i2c_t3.h)
        - Changed Tx/Rx buffer indicies to size_t to allow for large (>256 byte) buffers
        - Left debug routines in place (controlled via header defines - default is OFF).  If debug is
            enabled, note that it can easily overrun the Debug queue on large I2C transfers, yielding
            garbage output.  Adjust ringbuf size (in rbuf.h) and possibly PIT interrupt rate to adjust
            data flow to Serial (note also the buffer in Serial can overflow if written too quickly).
        - Added getError() function to return Wire error code
        - Added pinConfigure() function for changing pins on the fly (only when bus not busy)
        - Added timeouts to endTransmission(), requestFrom(), and finish()
    ------------------------------------------------------------------------------------------------------
    TwoWire.cpp - TWI/I2C library for Wiring & Arduino
    Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
    Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
    ------------------------------------------------------------------------------------------------------
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(I2C_T3_H) && (defined(__MK20DX128__) || defined(__MK20DX256__))
#define I2C_T3_H

#include <inttypes.h>
#include <stdio.h> // for size_t
#include "Arduino.h"

// ======================================================================================================
// == Start User Define Section =========================================================================
// ======================================================================================================

// ------------------------------------------------------------------------------------------------------
// I2C Bus Enable control - change to enable buses as follows.  Currently this only affects Teensy3.1.
//                          Teensy3.0 always uses I2C0 only regardless of this setting.
//
// I2C_BUS_ENABLE 1   (enable I2C0 only)
// I2C_BUS_ENABLE 2   (enable I2C0 & I2C1)
//
#define I2C_BUS_ENABLE 2

// ------------------------------------------------------------------------------------------------------
// Tx/Rx buffer sizes - modify these as needed.  Buffers should be large enough to hold:
//                      Target Addr + Target Command (varies with protocol) + Data payload
//                      Default is: 1byte Addr + 2byte Command + 256byte Data
//
#define I2C_TX_BUFFER_LENGTH 259
#define I2C_RX_BUFFER_LENGTH 259

// ------------------------------------------------------------------------------------------------------
// Interrupt flag - uncomment and set below to make the specified pin high whenever the
//                  I2C interrupt occurs.  This is useful as a trigger signal when using a logic analyzer.
//
//#define I2C0_INTR_FLAG_PIN 8
//#define I2C1_INTR_FLAG_PIN 9

// ======================================================================================================
// == End User Define Section ===========================================================================
// ======================================================================================================


// ------------------------------------------------------------------------------------------------------
// Set number of enabled buses
//
#if defined(__MK20DX256__) && (I2C_BUS_ENABLE >= 2)
    #define I2C_BUS_NUM 2
#else
    #define I2C_BUS_NUM 1
#endif


// ------------------------------------------------------------------------------------------------------
// Teensy 3.1 defines for 2nd I2C bus (this is temporary until Teensyduino is updated to incorporate them)
//
#if I2C_BUS_NUM >= 2
    #if !defined(I2C1_A1)
        #define I2C1_A1    *(volatile uint8_t  *)0x40067000 // I2C1 Address Register 1
        #define I2C1_F     *(volatile uint8_t  *)0x40067001 // I2C1 Frequency Divider register
        #define I2C1_C1    *(volatile uint8_t  *)0x40067002 // I2C1 Control Register 1
        #define I2C1_S     *(volatile uint8_t  *)0x40067003 // I2C1 Status register
        #define I2C1_D     *(volatile uint8_t  *)0x40067004 // I2C1 Data I/O register
        #define I2C1_C2    *(volatile uint8_t  *)0x40067005 // I2C1 Control Register 2
        #define I2C1_FLT   *(volatile uint8_t  *)0x40067006 // I2C1 Programmable Input Glitch Filter register
        #define I2C1_RA    *(volatile uint8_t  *)0x40067007 // I2C1 Range Address register
        #define I2C1_SMB   *(volatile uint8_t  *)0x40067008 // I2C1 SMBus Control and Status register
        #define I2C1_A2    *(volatile uint8_t  *)0x40067009 // I2C1 Address Register 2
        #define I2C1_SLTH  *(volatile uint8_t  *)0x4006700A // I2C1 SCL Low Timeout Register High
        #define I2C1_SLTL  *(volatile uint8_t  *)0x4006700B // I2C1 SCL Low Timeout Register Low
    #endif
#endif


// ------------------------------------------------------------------------------------------------------
// Interrupt flag setup
//
#if defined(I2C0_INTR_FLAG_PIN)
    #define I2C0_INTR_FLAG_INIT do             \
    {                                          \
        pinMode(I2C0_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C0_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C0_INTR_FLAG_ON   do {digitalWrite(I2C0_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C0_INTR_FLAG_OFF  do {digitalWrite(I2C0_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C0_INTR_FLAG_INIT do{}while(0)
    #define I2C0_INTR_FLAG_ON   do{}while(0)
    #define I2C0_INTR_FLAG_OFF  do{}while(0)
#endif

#if defined(I2C1_INTR_FLAG_PIN)
    #define I2C1_INTR_FLAG_INIT do             \
    {                                          \
        pinMode(I2C1_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C1_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C1_INTR_FLAG_ON   do {digitalWrite(I2C1_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C1_INTR_FLAG_OFF  do {digitalWrite(I2C1_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C1_INTR_FLAG_INIT do{}while(0)
    #define I2C1_INTR_FLAG_ON   do{}while(0)
    #define I2C1_INTR_FLAG_OFF  do{}while(0)
#endif


// ------------------------------------------------------------------------------------------------------
// Debug controls - these defines control ISR diagnostics.  This is only really necessary for
//                  debugging bus problems, or for devices with no other means of data visibility,
//                  or for ISR development.  When enabled data will output on Serial, so if Serial
//                  already has traffic the result will be a mashup of both streams.
//
//                  Enabling debug on a dual I2C device will dump data for both buses. Bus number
//                  is prefixed to output register names, for example:
//                  0_A1 == I2C0_A1 register
//                  1_F  == I2C1_F register
//
//                  Verbose modes are NOT recommended for anything except ISR devel (they will
//                  easily overrun ring and/or Serial buffers).  If messages are garbled, try
//                  increasing ring buffer size in rbuf.h.
//

// I2C_DEBUG - uncomment below to show data transfer, START, STOP, etc.
// I2C_DEBUG_VERBOSE1 - uncomment below for more verbose diagnostics (not recommended)
// I2C_DEBUG_VERBOSE2 - uncomment below for even more verbose diagnostics (not recommended)
//#define I2C_DEBUG
//#define I2C_DEBUG_VERBOSE1
//#define I2C_DEBUG_VERBOSE2

//
// Debug Coding - note: for register information refer to
// Teensy 3.0: K20P64M50SF0RM.pdf, Chapter 44: Inter-Integrated Circuit (I2C) - Page 1012
// Teensy 3.1: K20P64M72SF1RM.pdf, Chapter 46: Inter-Integrated Circuit (I2C) - Page 1169
//
// Register dump (n will be the bus number):
//  n_A1:   I2C Address Register 1
//  n_F:    I2C Frequency Divider register
//  n_C1:   I2C Control Register 1
//  n_S:    I2C Status register
//  n_D:    I2C Data I/O register
//  n_C2:   I2C Control Register 2
//  n_FLT:  I2C Programmable Input Glitch Filter register
//
// Debug Messages (n will be the bus number):
//  n_MT       Master Transmit
//  n_MR       Master Receive
//  n_AST      Addressed Slave Transmit
//  n_ST       Slave Transmit
//  n_ASR      Addressed Slave Receive
//  n_SR       Slave Receive
//  n_T        Target Address
//  n_N        NAK
//  n_A        ACK
//  n_START    START
//  n_RSTART   Repeated START
//  n_STOP     STOP
//  n_ARBL     Arbitration Lost
//  n_TMOUT    Timeout occurred
//  n_BL       Buffer Length
//  n_^        SDA Rising-ISR
//  n_x        SDA Rising-ISR disconnected
//

#if defined(I2C_DEBUG) || defined(I2C_DEBUG_VERBOSE1) || defined(I2C_DEBUG_VERBOSE2)
    #include "../rbuf/rbuf.h"
    extern rbuf i2cDebug;
    extern uint8_t busVal;
    extern uint8_t i2cDebugBuf[];
    extern size_t i2cDebugBufLen;
    extern IntervalTimer i2cDebugTimer;
    void printI2CDebug(void);

    #if defined(I2C_DEBUG_VERBOSE2)
        // verbose regs level2
        #define I2C_DEBUG_REGS do          \
        {                                  \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_A1:", 4);  \
            i2cDebug.putAHex(*(i2c->A1));  \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_F:", 3);   \
            i2cDebug.putAHex(*(i2c->F));   \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_C1:", 4);  \
            i2cDebug.putAHex(*(i2c->C1));  \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_S:", 3);   \
            i2cDebug.putAHex(*(i2c->S));   \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_C2:", 4);  \
            i2cDebug.putAHex(*(i2c->C2));  \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_FLT:", 5); \
            i2cDebug.putAHex(*(i2c->FLT)); \
        } while(0)
        // normal debug strings
        #define I2C_DEBUG_STR(x) do                         \
        {                                                   \
            i2cDebug.putI(' ');                             \
            i2cDebug.putI(busVal);                          \
            i2cDebug.putI('_');                             \
            i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1);\
        } while(0)
        #define I2C_DEBUG_HEX(x) i2cDebug.putAHex(x)
        // verbose debug strings
        #define I2C_DEBUG_STR2(x) do                        \
        {                                                   \
            i2cDebug.putI(' ');                             \
            i2cDebug.putI(busVal);                          \
            i2cDebug.putI('_');                             \
            i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1);\
        } while(0)
        #define I2C_DEBUG_STRB2(x) i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1)
        #define I2C_DEBUG_HEX2(x) i2cDebug.putAHex(x)

    #elif defined(I2C_DEBUG_VERBOSE1)
        // verbose regs level1
        #define I2C_DEBUG_REGS do          \
        {                                  \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_C1:", 4);  \
            i2cDebug.putAHex(*(i2c->C1));  \
            i2cDebug.putI(' ');            \
            i2cDebug.putI(busVal);         \
            i2cDebug.putBlock("_S:", 3);   \
            i2cDebug.putAHex(*(i2c->S));   \
        } while(0)
        // normal debug strings
        #define I2C_DEBUG_STR(x) do                         \
        {                                                   \
            i2cDebug.putI(' ');                             \
            i2cDebug.putI(busVal);                          \
            i2cDebug.putI('_');                             \
            i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1);\
        } while(0)
        #define I2C_DEBUG_HEX(x) i2cDebug.putAHex(x)
        // verbose debug strings
        #define I2C_DEBUG_STR2(x) do                        \
        {                                                   \
            i2cDebug.putI(' ');                             \
            i2cDebug.putI(busVal);                          \
            i2cDebug.putI('_');                             \
            i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1);\
        } while(0)
        #define I2C_DEBUG_STRB2(x) i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1)
        #define I2C_DEBUG_HEX2(x) i2cDebug.putAHex(x)

    #elif defined(I2C_DEBUG)
        // no verbose regs
        #define I2C_DEBUG_REGS do{}while(0)
        // normal debug strings
        #define I2C_DEBUG_STR(x) do                         \
        {                                                   \
            i2cDebug.putI(' ');                             \
            i2cDebug.putI(busVal);                          \
            i2cDebug.putI('_');                             \
            i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1);\
        } while(0)
        #define I2C_DEBUG_HEX(x) i2cDebug.putAHex(x)
        // no verbose debug strings
        #define I2C_DEBUG_STR2(x) do{}while(0)
        #define I2C_DEBUG_STRB2(x) do{}while(0)
        #define I2C_DEBUG_HEX2(x) do{}while(0)
    #endif

    // init macro setup debug
    #define I2C_DEBUG_INIT busVal=bus+0x30
    // basic string (no bus prefix)
    #define I2C_DEBUG_STRB(x) i2cDebug.putBlock(x,(sizeof(x)/sizeof(char))-1)
    // wait macro for debug msgs to clear, use this if interleaving with other output on Serial
    #define I2C_DEBUG_WAIT while(i2cDebug.len())

#else
    // no debug
    #define I2C_DEBUG_INIT     do{}while(0)
    #define I2C_DEBUG_REGS     do{}while(0)
    #define I2C_DEBUG_STR(x)   do{}while(0)
    #define I2C_DEBUG_STRB(x)  do{}while(0)
    #define I2C_DEBUG_HEX(x)   do{}while(0)
    #define I2C_DEBUG_STR2(x)  do{}while(0)
    #define I2C_DEBUG_STRB2(x) do{}while(0)
    #define I2C_DEBUG_HEX2(x)  do{}while(0)
    #define I2C_DEBUG_WAIT     do{}while(0)
#endif


// ------------------------------------------------------------------------------------------------------
// Function argument enums
//
enum i2c_mode   {I2C_MASTER, I2C_SLAVE};
enum i2c_pins   {I2C_PINS_18_19,
                 I2C_PINS_16_17,
                 I2C_PINS_29_30,
                 I2C_PINS_26_31};
enum i2c_pullup {I2C_PULLUP_EXT, I2C_PULLUP_INT};
enum i2c_rate   {I2C_RATE_100,
                 I2C_RATE_200,
                 I2C_RATE_300,
                 I2C_RATE_400,
                 I2C_RATE_600,
                 I2C_RATE_800,
                 I2C_RATE_1000,
                 I2C_RATE_1200,
                 I2C_RATE_1500,
                 I2C_RATE_2000,
                 I2C_RATE_2400};
enum i2c_stop   {I2C_NOSTOP, I2C_STOP};
enum i2c_status {I2C_WAITING,
                 I2C_SENDING,
                 I2C_SEND_ADDR,
                 I2C_RECEIVING,
                 I2C_TIMEOUT,
                 I2C_ADDR_NAK,
                 I2C_DATA_NAK,
                 I2C_ARB_LOST,
                 I2C_SLAVE_TX,
                 I2C_SLAVE_RX};


// ------------------------------------------------------------------------------------------------------
// Main I2C data structure
//
struct i2cStruct
{
    volatile uint8_t* A1;                    // Address Register 1                (User&ISR)
    volatile uint8_t* F;                     // Frequency Divider Register        (User&ISR)
    volatile uint8_t* C1;                    // Control Register 1                (User&ISR)
    volatile uint8_t* S;                     // Status Register                   (User&ISR)
    volatile uint8_t* D;                     // Data I/O Register                 (User&ISR)
    volatile uint8_t* C2;                    // Control Register 2                (User&ISR)
    volatile uint8_t* FLT;                   // Programmable Input Glitch Filter  (User&ISR)
    volatile uint8_t* RA;                    // Range Address Register            (User&ISR)
    volatile uint8_t* SMB;                   // SMBus Control and Status Register (User&ISR)
    volatile uint8_t* A2;                    // Address Register 2                (User&ISR)
    volatile uint8_t* SLTH;                  // SCL Low Timeout Register High     (User&ISR)
    volatile uint8_t* SLTL;                  // SCL Low Timeout Register Low      (User&ISR)
    uint8_t  rxBuffer[I2C_RX_BUFFER_LENGTH]; // Rx Buffer                         (ISR)
    volatile size_t   rxBufferIndex;         // Rx Index                          (User&ISR)
    volatile size_t   rxBufferLength;        // Rx Length                         (ISR)
    uint8_t  txBuffer[I2C_TX_BUFFER_LENGTH]; // Tx Buffer                         (User)
    volatile size_t   txBufferIndex;         // Tx Index                          (User&ISR)
    volatile size_t   txBufferLength;        // Tx Length                         (User&ISR)
    i2c_mode currentMode;                    // Current Mode                      (User)
    i2c_pins currentPins;                    // Current Pins                      (User)
    i2c_stop currentStop;                    // Current Stop                      (User)
    volatile i2c_status currentStatus;       // Current Status                    (User&ISR)
    uint8_t  rxAddr;                         // Rx Address                        (ISR)
    size_t   reqCount;                       // Byte Request Count                (User)
    uint8_t  irqCount;                       // IRQ Count, used by SDA-rising ISR (ISR)
    uint8_t  timeoutRxNAK;                   // Rx Timeout NAK flag               (ISR)
    void (*user_onReceive)(size_t len);      // Slave Rx Callback Function        (User)
    void (*user_onRequest)(void);            // Slave Tx Callback Function        (User)
};


// ------------------------------------------------------------------------------------------------------
// I2C Class
//
extern "C" void i2c0_isr(void);
extern "C" void i2c1_isr(void);
extern "C" void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus);

class i2c_t3 : public Stream
{
private:
    // I2C data structures - these need to be static so "C" ISRs can use them
    static struct i2cStruct i2cData[I2C_BUS_NUM];
    // I2C bus number - this is a local, passed as an argument to base functions
    //                  since static functions cannot see it.
    uint8_t bus;
    // I2C structure pointer - this is a local, passed as an argument to base functions
    //                         since static functions cannot see it.
    struct i2cStruct* i2c;

    friend void i2c0_isr(void);
    friend void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus);
    static void sda0_rising_isr(void);
    static void sda_rising_isr_handler(struct i2cStruct* i2c, uint8_t bus);

    #if I2C_BUS_NUM >= 2
        friend void i2c1_isr(void);
        static void sda1_rising_isr(void);
    #endif

public:
    i2c_t3(uint8_t i2c_bus);
    ~i2c_t3();

    // ------------------------------------------------------------------------------------------------------
    // Initialize I2C (base routine)
    //
    static void begin_(struct i2cStruct* i2c, uint8_t bus, i2c_mode mode, uint8_t address1, uint8_t address2, i2c_pins pins, i2c_pullup pullup, i2c_rate rate);
    //
    // Initialize I2C (Master) - initializes I2C as Master mode, pins 18/19 (Wire) or 29/30 (Wire1), external pullups, 100kHz rate
    // return: none
    //
    inline void begin()
        { begin_(i2c, bus, I2C_MASTER, 0, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_29_30), I2C_PULLUP_EXT, I2C_RATE_2000); }
    //
    // Initialize I2C (Slave) - initializes I2C as Slave mode using address, pins 18/19 (Wire) or 29/30 (Wire1), external pullups, 100kHz rate
    // return: none
    // parameters:
    //      address = 7bit slave address of device
    //
    inline void begin(int address)
        { begin_(i2c, bus, I2C_SLAVE, (uint8_t)address, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_29_30), I2C_PULLUP_EXT, I2C_RATE_2000); }
    inline void begin(uint8_t address)
        { begin_(i2c, bus, I2C_SLAVE, address, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_29_30), I2C_PULLUP_EXT, I2C_RATE_2000); }
    //
    // Initialize I2C - initializes I2C as Master or single address Slave
    // return: none
    // parameters:
    //      mode = I2C_MASTER, I2C_SLAVE
    //      address = 7bit slave address when configured as Slave (ignored for Master mode)
    //      pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    //      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    //      rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000,
    //             I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_2000, I2C_RATE_2400
    //
    inline void begin(i2c_mode mode, uint8_t address, i2c_pins pins, i2c_pullup pullup, i2c_rate rate)
        { begin_(i2c, bus, mode, address, 0, pins, pullup, rate); }
    //
    // Initialize I2C - initializes I2C as Master or address range Slave
    // return: none
    // parameters:
    //      mode = I2C_MASTER, I2C_SLAVE
    //      address1 = 1st 7bit address for specifying Slave address range (ignored for Master mode)
    //      address2 = 2nd 7bit address for specifying Slave address range (ignored for Master mode)
    //      pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    //      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    //      rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000,
    //             I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_2000, I2C_RATE_2400
    //
    inline void begin(i2c_mode mode, uint8_t address1, uint8_t address2, i2c_pins pins, i2c_pullup pullup, i2c_rate rate)
        { begin_(i2c, bus, mode, address1, address2, pins, pullup, rate); }

    // ------------------------------------------------------------------------------------------------------
    // Configure I2C pins (base routine)
    //
    static uint8_t pinConfigure_(struct i2cStruct* i2c, uint8_t bus, i2c_pins pins, i2c_pullup pullup);
    //
    // Configure I2C pins - reconfigures active I2C pins on-the-fly (only works when bus is idle). Inactive pins
    //                      will switch to input mode using same pullup configuration.
    // return: 1=success, 0=fail (bus busy)
    // parameters:
    //      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 | Wire1: I2C_PINS_29_30, I2C_PINS_26_31
    //      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    //
    inline uint8_t pinConfigure(i2c_pins pins, i2c_pullup pullup) { return pinConfigure_(i2c, bus, pins, pullup); }

    // ------------------------------------------------------------------------------------------------------
    // Setup Master Transmit - initialize Tx buffer for transmit to slave at address
    // return: none
    // parameters:
    //      address = target 7bit slave address
    //
    void beginTransmission(uint8_t address);
    inline void beginTransmission(int address) { beginTransmission((uint8_t)address); }

    // ------------------------------------------------------------------------------------------------------
    // Master Transmit - blocking routine with timeout, transmits Tx buffer to slave. i2c_stop parameter can be used
    //                   to indicate if command should end with a STOP(I2C_STOP) or not (I2C_NOSTOP).
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    // parameters:
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //      timeout = timeout in microseconds
    //
    uint8_t endTransmission(i2c_stop sendStop, uint32_t timeout);
    //
    // Master Transmit - blocking routine, transmits Tx buffer to slave
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    //
    inline uint8_t endTransmission(void) { return endTransmission(I2C_STOP, 0); }
    //
    // Master Transmit - blocking routine, transmits Tx buffer to slave. i2c_stop parameter can be used to indicate
    //                   if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP).
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    // parameters:
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline uint8_t endTransmission(i2c_stop sendStop) { return endTransmission(sendStop, 0); }

    // ------------------------------------------------------------------------------------------------------
    // Send Master Transmit (base routine)
    //
    static void sendTransmission_(struct i2cStruct* i2c, i2c_stop sendStop);
    //
    // Send Master Transmit - non-blocking routine, starts transmit of Tx buffer to slave. i2c_stop parameter can be
    //                        used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use
    //                        done() or finish() to determine completion and status() to determine success/fail.
    // return: none
    // parameters:
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline void sendTransmission(i2c_stop sendStop) { sendTransmission_(i2c, sendStop); }

    // ------------------------------------------------------------------------------------------------------
    // Master Receive (base routine)
    //
    static size_t requestFrom_(struct i2cStruct* i2c, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout);
    //
    // Master Receive - blocking routine, requests length bytes from slave at address. Receive data will be placed
    //                  in the Rx buffer.
    // return: #bytes received = success, 0=fail
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //
    inline size_t requestFrom(uint8_t addr, size_t len)
        { return requestFrom_(i2c, addr, len, I2C_STOP, 0); }
    inline size_t requestFrom(int addr, int len)
        { return requestFrom_(i2c, (uint8_t)addr, (size_t)len, I2C_STOP, 0); }
    //
    // Master Receive - blocking routine, requests length bytes from slave at address. Receive data will be placed
    //                  in the Rx buffer. i2c_stop parameter can be used to indicate if command should end with a
    //                  STOP (I2C_STOP) or not (I2C_NOSTOP).
    // return: #bytes received = success, 0=fail
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline size_t requestFrom(uint8_t addr, size_t len, i2c_stop sendStop)
        { return requestFrom_(i2c, addr, len, sendStop, 0); }
    //
    // Master Receive - blocking routine with timeout, requests length bytes from slave at address. Receive data will
    //                  be placed in the Rx buffer. i2c_stop parameter can be used to indicate if command should end
    //                  with a STOP (I2C_STOP) or not (I2C_NOSTOP).
    // return: #bytes received = success, 0=fail (0 length request, NAK, timeout, or bus error)
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //      timeout = timeout in microseconds
    //
    inline size_t requestFrom(uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout)
        { return requestFrom_(i2c, addr, len, sendStop, timeout); }

    // ------------------------------------------------------------------------------------------------------
    // Start Master Receive (base routine)
    //
    static void sendRequest_(struct i2cStruct* i2c, uint8_t addr, size_t len, i2c_stop sendStop);
    //
    // Start Master Receive - non-blocking routine, starts request for length bytes from slave at address. Receive
    //                        data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if
    //                        command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use done() or finish()
    //                        to determine completion and status() to determine success/fail.
    // return: none
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline void sendRequest(uint8_t addr, size_t len, i2c_stop sendStop) { sendRequest_(i2c, addr, len, sendStop); }

    // ------------------------------------------------------------------------------------------------------
    // Get Wire Error - returns "Wire" error code from a failed Tx/Rx command
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    // (Note: error code 1 (data too long) is only valid after Tx, if it appears after Rx then it was set by a previous Tx)
    //
    uint8_t getError(void);

    // ------------------------------------------------------------------------------------------------------
    // Return Status - returns current status of I2C (enum return value)
    // return: I2C_WAITING, I2C_SENDING, I2C_SEND_ADDR, I2C_RECEIVING, I2C_TIMEOUT, I2C_ADDR_NAK, I2C_DATA_NAK,
    //         I2C_ARB_LOST, I2C_SLAVE_TX, I2C_SLAVE_RX
    //
    inline i2c_status status(void) { return i2c->currentStatus; }

    // ------------------------------------------------------------------------------------------------------
    // Return Status (base routine)
    //
    static uint8_t done_(struct i2cStruct* i2c);
    //
    // Done Check - returns simple complete/not-complete value to indicate I2C status
    // return: 1=Tx/Rx complete (with or without errors), 0=still running
    //
    inline uint8_t done(void) { return done_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Return Status (base routine)
    //
    static uint8_t finish_(struct i2cStruct* i2c, uint32_t timeout);
    //
    // Finish - blocking routine, loops until Tx/Rx is complete
    // return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb Lost)
    //
    inline uint8_t finish(void) { return finish_(i2c, 0); }
    //
    // Finish - blocking routine with timeout, loops until Tx/Rx is complete or timeout occurs
    // return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb Lost)
    // parameters:
    //      timeout = timeout in microseconds
    //
    inline uint8_t finish(uint32_t timeout) { return finish_(i2c, timeout); }

    // ------------------------------------------------------------------------------------------------------
    // Write - write data byte to Tx buffer
    // return: #bytes written = success, 0=fail
    // parameters:
    //      data = data byte
    //
    size_t write(uint8_t data);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n)          { return write((uint8_t)n); }
    inline size_t write(unsigned int n)  { return write((uint8_t)n); }
    inline size_t write(int n)           { return write((uint8_t)n); }

    // ------------------------------------------------------------------------------------------------------
    // Write Array - write length number of bytes from data array to Tx buffer
    // return: #bytes written = success, 0=fail
    // parameters:
    //      data = pointer to uint8_t (or char) array of data
    //      length = number of bytes to write
    //
    size_t write(const uint8_t* data, size_t quantity);
    inline size_t write(const char* str) { return write((const uint8_t*)str, strlen(str)); }

    // ------------------------------------------------------------------------------------------------------
    // Available - returns number of remaining available bytes in Rx buffer
    // return: #bytes available
    //
    inline int available(void) { return i2c->rxBufferLength - i2c->rxBufferIndex; }

    // ------------------------------------------------------------------------------------------------------
    // Read (base routine)
    //
    static int read_(struct i2cStruct* i2c);
    //
    // Read - returns next data byte (signed int) from Rx buffer
    // return: data, -1 if buffer empty
    //
    inline int read(void) { return read_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Peek (base routine)
    //
    static int peek_(struct i2cStruct* i2c);
    //
    // Peek - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer
    // return: data, -1 if buffer empty
    //
    inline int peek(void) { return peek_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Read Byte (base routine)
    //
    static uint8_t readByte_(struct i2cStruct* i2c);
    //
    // Read Byte - returns next data byte (uint8_t) from Rx buffer
    // return: data, 0 if buffer empty
    //
    inline uint8_t readByte(void) { return readByte_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Peek Byte (base routine)
    //
    static uint8_t peekByte_(struct i2cStruct* i2c);
    //
    // Peek Byte - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer
    // return: data, 0 if buffer empty
    //
    inline uint8_t peekByte(void) { return peekByte_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Flush (not implemented)
    //
    inline void flush(void) {}

    // ------------------------------------------------------------------------------------------------------
    // Get Rx Address - returns target address of incoming I2C command. Used for Slaves operating over an address range.
    // return: rxAddr of last received command
    //
    inline uint8_t getRxAddr(void) { return i2c->rxAddr; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Rx
    //
    inline void onReceive(void (*function)(size_t len)) { i2c->user_onReceive = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Tx
    //
    inline void onRequest(void (*function)(void)) { i2c->user_onRequest = function; }

    // ------------------------------------------------------------------------------------------------------
    // for compatibility with pre-1.0 sketches and libraries
    inline void send(uint8_t b)             { write(b); }
    inline void send(uint8_t* s, uint8_t n) { write(s, n); }
    inline void send(int n)                 { write((uint8_t)n); }
    inline void send(char* s)               { write(s); }
    inline uint8_t receive(void)            { int c = read(); return (c<0) ? 0 : c; }
};

extern i2c_t3 Wire;
#if I2C_BUS_NUM >= 2
    extern i2c_t3 Wire1;
#endif

#endif // I2C_T3_H
