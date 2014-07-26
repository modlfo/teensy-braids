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

#if defined(__MK20DX128__) || defined(__MK20DX256__)

#include "mk20dx128.h"
#include "core_pins.h"
#include "i2c_t3.h"

#ifdef I2C_DEBUG
    rbuf i2cDebug; // ring buffer for passing diagnostics back to main loop
    uint8_t busVal;
    uint8_t i2cDebugBuf[32]; // small buffer for moving data to Serial
    size_t i2cDebugBufLen;
    IntervalTimer i2cDebugTimer;

    // print I2C debug - call from main loop to dump diagnostics
    void printI2CDebug(void)
    {
        i2cDebugBufLen = i2cDebug.len();
        if(i2cDebugBufLen)
        {
            i2cDebugBufLen = (i2cDebugBufLen > 32) ? 32 : i2cDebugBufLen; // limit to prevent Serial overflow
            i2cDebug.getBlock(i2cDebugBuf,i2cDebugBufLen);
            Serial.write(i2cDebugBuf,i2cDebugBufLen);
        }
    }
#endif // I2C_DEBUG


// ------------------------------------------------------------------------------------------------------
// Static inits
//
struct i2cStruct i2c_t3::i2cData[] =
{
    {&I2C0_A1, &I2C0_F, &I2C0_C1, &I2C0_S, &I2C0_D, &I2C0_C2,
     &I2C0_FLT, &I2C0_RA, &I2C0_SMB, &I2C0_A2, &I2C0_SLTH, &I2C0_SLTL,
     {}, 0, 0, {}, 0, 0, I2C_MASTER, I2C_PINS_18_19, I2C_STOP, I2C_WAITING,
     0, 0, 0, 0, NULL, NULL}
#if I2C_BUS_NUM >= 2
   ,{&I2C1_A1, &I2C1_F, &I2C1_C1, &I2C1_S, &I2C1_D, &I2C1_C2,
     &I2C1_FLT, &I2C1_RA, &I2C1_SMB, &I2C1_A2, &I2C1_SLTH, &I2C1_SLTL,
     {}, 0, 0, {}, 0, 0, I2C_MASTER, I2C_PINS_29_30, I2C_STOP, I2C_WAITING,
     0, 0, 0, 0, NULL, NULL}
#endif
};


// ------------------------------------------------------------------------------------------------------
// Constructor/Destructor
//
i2c_t3::i2c_t3(uint8_t i2c_bus)
{
    bus = i2c_bus;
    i2c = &i2cData[bus];
}
i2c_t3::~i2c_t3()
{
    #ifdef I2C_DEBUG
        i2cDebugTimer.end();
    #endif
}


// ------------------------------------------------------------------------------------------------------
// Initialize I2C - initializes I2C as Master or address range Slave
// return: none
// parameters:
//      mode = I2C_MASTER, I2C_SLAVE
//      address1 = 1st 7bit address for specifying Slave address range (ignored for Master mode)
//      address2 = 2nd 7bit address for specifying Slave address range (ignored for Master mode)
//      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 / Wire1: I2C_PINS_29_30, I2C_PINS_26_31
//      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
//      rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000,
//             I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_2000, I2C_RATE_2400
//
void i2c_t3::begin_(struct i2cStruct* i2c, uint8_t bus, i2c_mode mode, uint8_t address1, uint8_t address2, i2c_pins pins, i2c_pullup pullup, i2c_rate rate)
{
    // Enable I2C internal clock
    if(bus == 0)
        SIM_SCGC4 |= SIM_SCGC4_I2C0;
    #if I2C_BUS_NUM >= 2
        if(bus == 1)
            SIM_SCGC4 |= SIM_SCGC4_I2C1;
    #endif

    i2c->currentMode = mode; // Set mode
    i2c->currentStatus = I2C_WAITING; // reset status

    // Set Master/Slave address (zeroed in Master to prevent accidental Rx when setup is changed dynamically)
    if(i2c->currentMode == I2C_MASTER)
    {
        *(i2c->C2) = I2C_C2_HDRS; // Set high drive select
        *(i2c->A1) = 0;
        *(i2c->RA) = 0;
    }
    else
    {
        *(i2c->C2) = (address2) ? (I2C_C2_HDRS|I2C_C2_RMEN) // Set high drive select and range-match enable
                                : I2C_C2_HDRS;              // Set high drive select
        // set Slave address, if two addresses are given, setup range and put lower address in A1, higher in RA
        *(i2c->A1) = (address2) ? ((address1 < address2) ? (address1<<1) : (address2<<1))
                                : (address1<<1);
        *(i2c->RA) = (address2) ? ((address1 < address2) ? (address2<<1) : (address1<<1))
                                : 0;
    }

    // Setup pins and options (note: does not "unset" unused pins if dynamically changed, must be done elsewhere)
    // As noted in original TwoWire.cpp, internal pullup is strong (about 190 ohms), but it can work if other
    // devices on bus have strong enough pulldown devices.
    uint32_t pinConfig0 = (pullup == I2C_PULLUP_EXT) ? (PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE)
                                                     : (PORT_PCR_MUX(2)|PORT_PCR_PE|PORT_PCR_PS);
    #if I2C_BUS_NUM >= 2
        uint32_t pinConfig1 = (pullup == I2C_PULLUP_EXT) ? (PORT_PCR_MUX(6)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE)
                                                         : (PORT_PCR_MUX(6)|PORT_PCR_PE|PORT_PCR_PS);
    #endif

    // The I2C interfaces are associated with the pins as follows:
    // I2C0 : I2C_PINS_18_19, I2C_PINS_16_17
    // I2C1 : I2C_PINS_29_30, I2C_PINS_26_31
    //
    // If pins are given an impossible value (eg. I2C0 with I2C_PINS_26_31) then the default pins will be
    // used, which for I2C0 is I2C_PINS_18_19, and for I2C1 is I2C_PINS_29_30
    //
    if(bus == 0)
    {
        if(pins == I2C_PINS_16_17)
        {
            i2c->currentPins = I2C_PINS_16_17;
            CORE_PIN16_CONFIG = pinConfig0;
            CORE_PIN17_CONFIG = pinConfig0;
        }
        else
        {
            i2c->currentPins = I2C_PINS_18_19;
            CORE_PIN18_CONFIG = pinConfig0;
            CORE_PIN19_CONFIG = pinConfig0;
        }
    }
    #if I2C_BUS_NUM >= 2
        if(bus == 1)
        {
            if(pins == I2C_PINS_26_31)
            {
                i2c->currentPins = I2C_PINS_26_31;
                CORE_PIN26_CONFIG = pinConfig1;
                CORE_PIN31_CONFIG = pinConfig1;
            }
            else
            {
                i2c->currentPins = I2C_PINS_29_30;
                CORE_PIN29_CONFIG = pinConfig0;
                CORE_PIN30_CONFIG = pinConfig0;
            }
        }
    #endif

    // Set rate and filter
    #if F_BUS == 48000000
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x27; break;   // 100k    480
        case I2C_RATE_200:  *(i2c->F) = 0x1F; break;   // 200k    240
        case I2C_RATE_300:  *(i2c->F) = 0x1D; break;   // 300k    160
        case I2C_RATE_400:  *(i2c->F) = 0x85; break;   // 400k    120
        case I2C_RATE_600:  *(i2c->F) = 0x14; break;   // 600k     80
        case I2C_RATE_800:  *(i2c->F) = 0x45; break;   // 800k     60
        case I2C_RATE_1000: *(i2c->F) = 0x0D; break;   // 1.0M     48
        case I2C_RATE_1200: *(i2c->F) = 0x0B; break;   // 1.2M     40
        case I2C_RATE_1500: *(i2c->F) = 0x09; break;   // 1.5M     32
        case I2C_RATE_2000: *(i2c->F) = 0x02; break;   // 2.0M     24
        case I2C_RATE_2400: *(i2c->F) = 0x00; break;   // 2.4M     20
        default:            *(i2c->F) = 0x27; break;   // 100k    480 (defaults to slowest)
        }
        *(i2c->FLT) = 4;
    #elif F_BUS == 24000000
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x1F; break;   // 100k    240
        case I2C_RATE_200:  *(i2c->F) = 0x85; break;   // 200k    120
        case I2C_RATE_300:  *(i2c->F) = 0x14; break;   // 300k     80
        case I2C_RATE_400:  *(i2c->F) = 0x45; break;   // 400k     60
        case I2C_RATE_600:  *(i2c->F) = 0x0B; break;   // 600k     40
        case I2C_RATE_800:  *(i2c->F) = 0x05; break;   // 800k     30
        case I2C_RATE_1000: *(i2c->F) = 0x02; break;   // 1.0M     24
        case I2C_RATE_1200: *(i2c->F) = 0x00; break;   // 1.2M     20
        default:            *(i2c->F) = 0x1F; break;   // 100k    240 (defaults to slowest)
        }
        *(i2c->FLT) = 2;
    #else
        #error "F_BUS must be 48 MHz or 24 MHz"
    #endif

    // Set config registers
    if(i2c->currentMode == I2C_MASTER)
        *(i2c->C1) = I2C_C1_IICEN; // Master - enable I2C (hold in Rx mode, intr disabled)
    else
        *(i2c->C1) = I2C_C1_IICEN|I2C_C1_IICIE; // Slave - enable I2C and interrupts

    // Nested Vec Interrupt Ctrl - enable I2C interrupt
    if(bus == 0)
    {
        NVIC_ENABLE_IRQ(IRQ_I2C0);
        I2C0_INTR_FLAG_INIT; // init I2C0 interrupt flag if used
    }
    #if I2C_BUS_NUM >= 2
        if(bus == 1)
        {
            NVIC_ENABLE_IRQ(IRQ_I2C1);
            I2C1_INTR_FLAG_INIT; // init I2C1 interrupt flag if used
        }
    #endif

    #ifdef I2C_DEBUG
        if(!Serial) Serial.begin(115200);
        i2cDebugTimer.begin(printI2CDebug, 500); // 500us period, 2kHz timer
    #endif
}


// ------------------------------------------------------------------------------------------------------
// Configure I2C pins - reconfigures active I2C pins on-the-fly (only works when bus is idle).  Inactive pins
//                      will switch to input mode using same pullup configuration.
// return: 1=success, 0=fail (bus busy)
// parameters:
//      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 | Wire1: I2C_PINS_29_30, I2C_PINS_26_31
//      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
//
uint8_t i2c_t3::pinConfigure_(struct i2cStruct* i2c, uint8_t bus, i2c_pins pins, i2c_pullup pullup)
{
    if(*(i2c->S) & I2C_S_BUSY) return 0; // return immediately if bus busy

    // The I2C interfaces are associated with the pins as follows:
    // I2C0 : I2C_PINS_18_19, I2C_PINS_16_17
    // I2C1 : I2C_PINS_29_30, I2C_PINS_26_31
    //
    // If pins are given an impossible value (eg. I2C0 with I2C_PINS_26_31) then the default pins will be
    // used, which for I2C0 is I2C_PINS_18_19, and for I2C1 is I2C_PINS_29_30

    uint32_t i2cConfig0 = (pullup == I2C_PULLUP_EXT) ? (PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE)
                                                     : (PORT_PCR_MUX(2)|PORT_PCR_PE|PORT_PCR_PS);
    #if I2C_BUS_NUM >= 2
        uint32_t i2cConfig1 = (pullup == I2C_PULLUP_EXT) ? (PORT_PCR_MUX(6)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE)
                                                         : (PORT_PCR_MUX(6)|PORT_PCR_PE|PORT_PCR_PS);
    #endif

    if(bus == 0)
    {
        if(pins == I2C_PINS_16_17)
        {
            pinMode(18,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
            pinMode(19,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
            i2c->currentPins = I2C_PINS_16_17;
            CORE_PIN16_CONFIG = i2cConfig0;
            CORE_PIN17_CONFIG = i2cConfig0;
        }
        else
        {
            pinMode(16,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
            pinMode(17,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
            i2c->currentPins = I2C_PINS_18_19;
            CORE_PIN18_CONFIG = i2cConfig0;
            CORE_PIN19_CONFIG = i2cConfig0;
        }
    }
    #if I2C_BUS_NUM >= 2
        if(bus == 1)
        {
            if(pins == I2C_PINS_26_31)
            {
                pinMode(29,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                pinMode(30,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                i2c->currentPins = I2C_PINS_26_31;
                CORE_PIN26_CONFIG = i2cConfig1;
                CORE_PIN31_CONFIG = i2cConfig1;
            }
            else
            {
                pinMode(26,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                pinMode(31,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                i2c->currentPins = I2C_PINS_29_30;
                CORE_PIN29_CONFIG = i2cConfig0;
                CORE_PIN30_CONFIG = i2cConfig0;
            }
        }
    #endif

    return 1;
}


// ------------------------------------------------------------------------------------------------------
// Setup Master Transmit - initialize Tx buffer for transmit to slave at address
// return: none
// parameters:
//      address = target 7bit slave address
//
void i2c_t3::beginTransmission(uint8_t address)
{
    i2c->txBuffer[0] = (address << 1); // store target addr
    i2c->txBufferLength = 1;
    clearWriteError(); // clear any previous write error
    i2c->currentStatus = I2C_WAITING; // reset status
}


// ------------------------------------------------------------------------------------------------------
// Master Transmit - blocking routine with timeout, transmits Tx buffer to slave. i2c_stop parameter can be used
//                   to indicate if command should end with a STOP(I2C_STOP) or not (I2C_NOSTOP).
// return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
// parameters:
//      i2c_stop = I2C_NOSTOP, I2C_STOP
//      timeout = timeout in microseconds
//
uint8_t i2c_t3::endTransmission(i2c_stop sendStop, uint32_t timeout)
{
    sendTransmission_(i2c, sendStop);

    // wait for completion or timeout
    finish_(i2c, timeout);

    return getError();
}


// ------------------------------------------------------------------------------------------------------
// Send Master Transmit - non-blocking routine, starts transmit of Tx buffer to slave. i2c_stop parameter can be
//                        used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use
//                        done() or finish() to determine completion and status() to determine success/fail.
// return: none
// parameters:
//      i2c_stop = I2C_NOSTOP, I2C_STOP
//
void i2c_t3::sendTransmission_(struct i2cStruct* i2c, i2c_stop sendStop)
{
    if(i2c->txBufferLength)
    {
        // clear the status flags
        *(i2c->S) = I2C_S_IICIF | I2C_S_ARBL;

        // now take control of the bus...
        if(*(i2c->C1) & I2C_C1_MST)
        {
            // we are already the bus master, so send a repeated start
            I2C_DEBUG_STR("RSTART"); // Repeated START
            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_RSTA | I2C_C1_TX;
        }
        else
        {
            // we are not currently the bus master, so wait for bus ready
            while(*(i2c->S) & I2C_S_BUSY);
            // become the bus master in transmit mode (send start)
            I2C_DEBUG_STR("START"); // START
            i2c->currentMode = I2C_MASTER;
            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
        }

        // send 1st data and enable interrupts
        i2c->currentStatus = I2C_SENDING;
        i2c->currentStop = sendStop;
        i2c->txBufferIndex = 0;
        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TX; // enable intr
        I2C_DEBUG_STR("T:"); I2C_DEBUG_HEX(i2c->txBuffer[i2c->txBufferIndex]); I2C_DEBUG_STRB("\n"); // target addr
        *(i2c->D) = i2c->txBuffer[i2c->txBufferIndex];
    }
}


// ------------------------------------------------------------------------------------------------------
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
size_t i2c_t3::requestFrom_(struct i2cStruct* i2c, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout)
{
    // exit immediately if request for 0 bytes
    if(len == 0) return 0;

    sendRequest_(i2c, addr, len, sendStop);

    // wait for completion or timeout
    if(finish_(i2c, timeout))
        return i2c->rxBufferLength;
    else
        return 0; // NAK, timeout or bus error
}


// ------------------------------------------------------------------------------------------------------
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
void i2c_t3::sendRequest_(struct i2cStruct* i2c, uint8_t addr, size_t len, i2c_stop sendStop)
{
    // exit immediately if request for 0 bytes
    if(len == 0) return;

    i2c->reqCount=len; // store request length
    i2c->rxBufferIndex = 0; // reset buffer
    i2c->rxBufferLength = 0;

    // clear the status flags
    *(i2c->S) = I2C_S_IICIF | I2C_S_ARBL;

    // now take control of the bus...
    if(*(i2c->C1) & I2C_C1_MST)
    {
        // we are already the bus master, so send a repeated start
        I2C_DEBUG_STR("RSTART"); // Repeated START
        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_RSTA | I2C_C1_TX;
    }
    else
    {
        // we are not currently the bus master, so wait for bus ready
        while(*(i2c->S) & I2C_S_BUSY);
        // become the bus master in transmit mode (send start)
        I2C_DEBUG_STR("START"); // START
        i2c->currentMode = I2C_MASTER;
        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
    }

    // send 1st data and enable interrupts
    i2c->currentStatus = I2C_SEND_ADDR;
    i2c->currentStop = sendStop;
    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TX; // enable intr
    uint8_t target = (addr << 1) | 1; // address + READ
    I2C_DEBUG_STR("T:"); I2C_DEBUG_HEX(target); I2C_DEBUG_STRB("\n"); // target addr
    *(i2c->D) = target;
}


// ------------------------------------------------------------------------------------------------------
// Get Wire Error - returns "Wire" error code from a failed Tx/Rx command
// return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
// (Note: error code 1 (data too long) is only valid after Tx, if it appears after Rx then it was set by a previous Tx)
//
uint8_t i2c_t3::getError(void)
{
    // convert status to Arduino return values (give these a higher priority than buf overflow error)
    switch(i2c->currentStatus)
    {
    case I2C_ADDR_NAK: return 2;
    case I2C_DATA_NAK: return 3;
    case I2C_ARB_LOST: return 4;
    case I2C_TIMEOUT:  return 4;
    default: break;
    }
    if(getWriteError()) return 1; // if write_error was set then flag as buffer overflow
    return 0; // no errors
}


// ------------------------------------------------------------------------------------------------------
// Done Check - returns simple complete/not-complete value to indicate I2C status
// return: 1=Tx/Rx complete (with or without errors), 0=still running
//
uint8_t i2c_t3::done_(struct i2cStruct* i2c)
{
    return (i2c->currentStatus==I2C_WAITING ||
            i2c->currentStatus==I2C_ADDR_NAK ||
            i2c->currentStatus==I2C_DATA_NAK ||
            i2c->currentStatus==I2C_ARB_LOST);
}


// ------------------------------------------------------------------------------------------------------
// Finish - blocking routine with timeout, loops until Tx/Rx is complete or timeout occurs
// return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb Lost)
// parameters:
//      timeout = timeout in microseconds
//
uint8_t i2c_t3::finish_(struct i2cStruct* i2c, uint32_t timeout)
{
    elapsedMicros deltaT;

    // wait for completion or timeout
    while(!done_(i2c) && (timeout == 0 || deltaT < timeout));

    // check exit status, if still Tx/Rx then timeout occurred
    if(i2c->currentStatus == I2C_SENDING ||
       i2c->currentStatus == I2C_SEND_ADDR ||
       i2c->currentStatus == I2C_RECEIVING)
        i2c->currentStatus = I2C_TIMEOUT; // set to timeout state

    // delay to allow bus to settle (eg. allow STOP to complete and be recognized,
    //                               not just on our side, but on slave side also)
    delayMicroseconds(10);
    if(i2c->currentStatus == I2C_WAITING) return 1;
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Write - write data to Tx buffer
// return: #bytes written = success, 0=fail
// parameters:
//      data = data byte
//
size_t i2c_t3::write(uint8_t data)
{
    if(i2c->txBufferLength < I2C_TX_BUFFER_LENGTH)
    {
        i2c->txBuffer[i2c->txBufferLength++] = data;
        return 1;
    }
    setWriteError();
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Write Array - write length number of bytes from data array to Tx buffer
// return: #bytes written = success, 0=fail
// parameters:
//      data = pointer to uint8_t array of data
//      length = number of bytes to write
//
size_t i2c_t3::write(const uint8_t* data, size_t quantity)
{
    if(i2c->txBufferLength < I2C_TX_BUFFER_LENGTH)
    {
        size_t avail = I2C_TX_BUFFER_LENGTH - i2c->txBufferLength;
        uint8_t* dest = i2c->txBuffer + i2c->txBufferLength;

        if(quantity > avail)
        {
            quantity = avail; // truncate to space avail if needed
            setWriteError();
        }
        for(size_t count=quantity; count; count--)
            *dest++ = *data++;
        i2c->txBufferLength += quantity;
        return quantity;
    }
    setWriteError();
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Read - returns next data byte (signed int) from Rx buffer
// return: data, -1 if buffer empty
//
int i2c_t3::read_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return -1;
    return i2c->rxBuffer[i2c->rxBufferIndex++];
}


// ------------------------------------------------------------------------------------------------------
// Peek - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer
// return: data, -1 if buffer empty
//
int i2c_t3::peek_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return -1;
    return i2c->rxBuffer[i2c->rxBufferIndex];
}


// ------------------------------------------------------------------------------------------------------
// Read Byte - returns next data byte (uint8_t) from Rx buffer
// return: data, 0 if buffer empty
//
uint8_t i2c_t3::readByte_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return 0;
    return i2c->rxBuffer[i2c->rxBufferIndex++];
}


// ------------------------------------------------------------------------------------------------------
// Peek Byte - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer
// return: data, 0 if buffer empty
//
uint8_t i2c_t3::peekByte_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return 0;
    return i2c->rxBuffer[i2c->rxBufferIndex];
}


// ======================================================================================================
// ------------------------------------------------------------------------------------------------------
// I2C Interrupt Service Routine
// ------------------------------------------------------------------------------------------------------
// ======================================================================================================

// I2C0 ISR
void i2c0_isr(void)
{
    I2C0_INTR_FLAG_ON;
    i2c_isr_handler(&(i2c_t3::i2cData[0]),0);
    I2C0_INTR_FLAG_OFF;
}

#if I2C_BUS_NUM >= 2
    // I2C1 ISR
    void i2c1_isr(void)
    {
        I2C1_INTR_FLAG_ON;
        i2c_isr_handler(&(i2c_t3::i2cData[1]),1);
        I2C1_INTR_FLAG_OFF;
    }
#endif

//
// I2C ISR base handler
//
void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus)
{
    uint8_t status, c1, data;

    status = *(i2c->S);
    c1 = *(i2c->C1);

    I2C_DEBUG_INIT; I2C_DEBUG_STR("I"); I2C_DEBUG_REGS; // interrupt, reg dump
    if(c1 & I2C_C1_MST)
    {
        //
        // Master Mode
        //
        if(c1 & I2C_C1_TX)
        {
            // Continue Master Transmit
            I2C_DEBUG_STR("MT"); // master transmit
            // check if Master Tx or Rx
            if(i2c->currentStatus == I2C_SENDING)
            {
                // check if slave ACK'd
                if(status & I2C_S_RXAK)
                {
                    I2C_DEBUG_STR("N"); // NAK
                    if(i2c->txBufferIndex == 0)
                        i2c->currentStatus = I2C_ADDR_NAK; // NAK on Addr
                    else
                        i2c->currentStatus = I2C_DATA_NAK; // NAK on Data
                    // send STOP, change to Rx mode, intr disabled
                    I2C_DEBUG_STR("STOP");
                    *(i2c->C1) = I2C_C1_IICEN;
                }
                else
                {
                    I2C_DEBUG_STR("A"); // ACK
                    // check if last byte transmitted
                    if(++i2c->txBufferIndex >= i2c->txBufferLength)
                    {
                        // Tx complete, change to waiting state
                        i2c->currentStatus = I2C_WAITING;
                        // send STOP if configured
                        if(i2c->currentStop == I2C_STOP)
                        {
                            // send STOP, change to Rx mode, intr disabled
                            I2C_DEBUG_STR("STOP");
                            *(i2c->C1) = I2C_C1_IICEN;
                        }
                        else
                        {
                            // no STOP, stay in Tx mode, intr disabled
                            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                        }
                    }
                    else
                    {
                        // transmit next byte
                        *(i2c->D) = i2c->txBuffer[i2c->txBufferIndex];
                        I2C_DEBUG_STR("D:"); I2C_DEBUG_HEX(i2c->txBuffer[i2c->txBufferIndex]); // Tx data
                    }
                }
                I2C_DEBUG_STRB("\n");
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
            else if(i2c->currentStatus == I2C_SEND_ADDR)
            {
                // Master Receive, addr sent
                if(status & I2C_S_ARBL)
                {
                    // Arbitration Lost
                    I2C_DEBUG_STR("ARBL\n"); // arb lost
                    i2c->currentStatus = I2C_ARB_LOST;
                    *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled (does this send STOP if ARBL flagged?)
                    *(i2c->S) = I2C_S_ARBL | I2C_S_IICIF; // clear arbl flag and intr
                    return;
                }
                if(status & I2C_S_RXAK)
                {
                    // Slave addr NAK
                    I2C_DEBUG_STR("N"); // NAK
                    i2c->currentStatus = I2C_ADDR_NAK; // NAK on Addr
                    // send STOP, change to Rx mode, intr disabled
                    I2C_DEBUG_STR("STOP");
                    *(i2c->C1) = I2C_C1_IICEN;
                }
                else
                {
                    // Slave addr ACK, change to Rx mode
                    I2C_DEBUG_STR("A"); // ACK
                    i2c->currentStatus = I2C_RECEIVING;
                    if(i2c->reqCount == 1)
                        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TXAK; // no STOP, Rx, NAK on recv
                    else
                        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST; // no STOP, change to Rx
                    data = *(i2c->D); // dummy read
                }
                I2C_DEBUG_STRB("\n");
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
            else if(i2c->currentStatus == I2C_TIMEOUT)
            {
                // send STOP if configured
                if(i2c->currentStop == I2C_STOP)
                {
                    // send STOP, change to Rx mode, intr disabled
                    I2C_DEBUG_STR("STOP\n");
                    I2C_DEBUG_STR("TMOUT\n");
                    *(i2c->C1) = I2C_C1_IICEN;
                }
                else
                {
                    // no STOP, stay in Tx mode, intr disabled
                    I2C_DEBUG_STR("TMOUT\n");
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                }
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
            else
            {
                // Should not be in Tx mode if not sending
                // send STOP, change to Rx mode, intr disabled
                I2C_DEBUG_STR("WTF\n");
                *(i2c->C1) = I2C_C1_IICEN;
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
        }
        else
        {
            // Continue Master Receive
            I2C_DEBUG_STR("MR"); // master receive
            // check if 2nd to last byte or timeout
            if((i2c->rxBufferLength+2) == i2c->reqCount ||
               (i2c->currentStatus == I2C_TIMEOUT && !i2c->timeoutRxNAK))
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TXAK; // no STOP, Rx, NAK on recv
            // if last byte or timeout send STOP
            if((i2c->rxBufferLength+1) >= i2c->reqCount ||
               (i2c->currentStatus == I2C_TIMEOUT && i2c->timeoutRxNAK))
            {
                i2c->timeoutRxNAK = 0; // clear flag
                if(i2c->currentStatus != I2C_TIMEOUT)
                    i2c->currentStatus = I2C_WAITING; // Rx complete, change to waiting state
                // change to Tx mode
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                // grab last data
                data = *(i2c->D);
                I2C_DEBUG_STR("D:"); I2C_DEBUG_HEX(data); // Rx data
                if(i2c->rxBufferLength < I2C_RX_BUFFER_LENGTH)
                    i2c->rxBuffer[i2c->rxBufferLength++] = data;
                if(i2c->currentStop == I2C_STOP)
                {
                    I2C_DEBUG_STR("N"); I2C_DEBUG_STR("STOP\n"); // NAK and STOP
                    delayMicroseconds(1); // empirical patch, lets things settle before issuing STOP
                    *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
                }
                else
                    I2C_DEBUG_STR("N\n"); // NAK no STOP
                if(i2c->currentStatus == I2C_TIMEOUT)
                    I2C_DEBUG_STR("TMOUT\n"); // timeout
            }
            else
            {
                // grab next data
                data = *(i2c->D);
                I2C_DEBUG_STR("D:"); I2C_DEBUG_HEX(data); // Rx data
                if(i2c->rxBufferLength < I2C_RX_BUFFER_LENGTH)
                    i2c->rxBuffer[i2c->rxBufferLength++] = data;
                I2C_DEBUG_STR("A\n"); // not last byte, mark as ACK
            }
            if(i2c->currentStatus == I2C_TIMEOUT && !i2c->timeoutRxNAK)
                i2c->timeoutRxNAK = 1; // set flag to indicate NAK sent
            *(i2c->S) = I2C_S_IICIF; // clear intr
            return;
        }
    }
    else
    {
        //
        // Slave Mode
        //
        if(status & I2C_S_ARBL)
        {
            // Arbitration Lost
            I2C_DEBUG_STR("ARBL"); // arb lost
            *(i2c->S) = I2C_S_ARBL; // clear arbl flag
            if(!(status & I2C_S_IAAS))
            {
                I2C_DEBUG_STRB("\n");
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
        }
        if(status & I2C_S_IAAS)
        {
            // If in Slave Rx already, then RepSTART occured, run callback
            if(i2c->currentStatus == I2C_SLAVE_RX && i2c->user_onReceive != NULL)
            {
                I2C_DEBUG_STR("RSTART");
                i2c->rxBufferIndex = 0;
                i2c->user_onReceive(i2c->rxBufferLength);
            }
            // Is Addressed As Slave
            if(status & I2C_S_SRW)
            {
                // Begin Slave Transmit
                I2C_DEBUG_STR("AST"); // addressed slave transmit
                i2c->currentStatus = I2C_SLAVE_TX;
                i2c->txBufferLength = 0;
                if(i2c->user_onRequest != NULL)
                    i2c->user_onRequest(); // load Tx buffer with data
                I2C_DEBUG_STR("BL:"); I2C_DEBUG_HEX(i2c->txBufferLength >> 8); I2C_DEBUG_HEX(i2c->txBufferLength); // buf len
                if(i2c->txBufferLength == 0)
                    i2c->txBuffer[0] = 0; // send 0's if buffer empty
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_TX;
                i2c->rxAddr = (*(i2c->D) >> 1); // read to get target addr
                *(i2c->D) = i2c->txBuffer[0]; // send first data
                i2c->txBufferIndex = 1;
                I2C_DEBUG_STR("D:"); I2C_DEBUG_HEX(i2c->txBuffer[0]); // Tx data
            }
            else
            {
                // Begin Slave Receive
                I2C_DEBUG_STR("ASR"); // addressed slave receive
                i2c->irqCount = 0;
                // setup SDA-rising ISR
                if(i2c->currentPins == I2C_PINS_18_19)
                    attachInterrupt(18, i2c_t3::sda0_rising_isr, RISING);
                else if(i2c->currentPins == I2C_PINS_16_17)
                    attachInterrupt(17, i2c_t3::sda0_rising_isr, RISING);
                #if I2C_BUS_NUM >= 2
                else if(i2c->currentPins == I2C_PINS_29_30)
                    attachInterrupt(30, i2c_t3::sda1_rising_isr, RISING);
                else if(i2c->currentPins == I2C_PINS_26_31)
                    attachInterrupt(31, i2c_t3::sda1_rising_isr, RISING);
                #endif
                i2c->currentStatus = I2C_SLAVE_RX;
                i2c->rxBufferLength = 0;
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE;
                i2c->rxAddr = (*(i2c->D) >> 1); // read to get target addr
            }
            I2C_DEBUG_STRB("\n");
            *(i2c->S) = I2C_S_IICIF; // clear intr
            return;
        }
        if(c1 & I2C_C1_TX)
        {
            // Continue Slave Transmit
            I2C_DEBUG_STR("ST"); // slave transmit
            if((status & I2C_S_RXAK) == 0)
            {
                // Master ACK'd previous byte
                I2C_DEBUG_STR("A"); // ACK
                if(i2c->txBufferIndex < i2c->txBufferLength)
                    data = i2c->txBuffer[i2c->txBufferIndex++];
                else
                    data = 0; // send 0's if buffer empty
                I2C_DEBUG_STR("D:"); I2C_DEBUG_HEX(data); // Tx data
                *(i2c->D) = data;
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_TX;
            }
            else
            {
                // Master did not ACK previous byte
                I2C_DEBUG_STR("N"); // NAK
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE; // switch to Rx mode
                data = *(i2c->D); // dummy read
                i2c->currentStatus = I2C_WAITING;
            }
        }
        else
        {
            // Continue Slave Receive
            I2C_DEBUG_STR("SR"); // slave receive
            i2c->irqCount = 0;
            if(i2c->currentPins == I2C_PINS_18_19)
                attachInterrupt(18, i2c_t3::sda0_rising_isr, RISING);
            else if(i2c->currentPins == I2C_PINS_16_17)
                attachInterrupt(17, i2c_t3::sda0_rising_isr, RISING);
            #if I2C_BUS_NUM >= 2
            else if(i2c->currentPins == I2C_PINS_29_30)
                attachInterrupt(30, i2c_t3::sda1_rising_isr, RISING);
            else if(i2c->currentPins == I2C_PINS_26_31)
                attachInterrupt(31, i2c_t3::sda1_rising_isr, RISING);
            #endif
            data = *(i2c->D);
            I2C_DEBUG_STR("D:"); I2C_DEBUG_HEX(data); // Rx data
            if(i2c->rxBufferLength < I2C_RX_BUFFER_LENGTH)
                i2c->rxBuffer[i2c->rxBufferLength++] = data;
        }
        I2C_DEBUG_STR("\n");
        *(i2c->S) = I2C_S_IICIF; // clear intr
    }
}


// ------------------------------------------------------------------------------------------------------
// SDA-Rising Interrupt Service Routine
//
// Detects the stop condition that terminates a slave receive transfer.
// If anyone from Freescale ever reads this code, please email me at
// paul@pjrc.com and explain how I can respond to the I2C stop without
// inefficient polling or a horrible pin change interrupt hack?!
//

// I2C0 SDA ISR
void i2c_t3::sda0_rising_isr(void)
{
    i2c_t3::sda_rising_isr_handler(&(i2c_t3::i2cData[0]),0);
}

#if I2C_BUS_NUM >= 2
    // I2C1 SDA ISR
    void i2c_t3::sda1_rising_isr(void)
    {
        i2c_t3::sda_rising_isr_handler(&(i2c_t3::i2cData[1]),1);
    }
#endif

//
// SDA ISR base handler
//
void i2c_t3::sda_rising_isr_handler(struct i2cStruct* i2c, uint8_t bus)
{
    uint8_t status = *(i2c->S); // capture status first, can change if ISR is too slow
    I2C_DEBUG_INIT; I2C_DEBUG_STR2("^"); I2C_DEBUG_STR2("S:"); I2C_DEBUG_HEX2(status); // mark SDA rising edge, dump status
    if(!(status & I2C_S_BUSY))
    {
        I2C_DEBUG_STR("STOP\n"); // detected STOP
        i2c->currentStatus = I2C_WAITING;
        if(i2c->currentPins == I2C_PINS_18_19)
            detachInterrupt(18);
        else if(i2c->currentPins == I2C_PINS_16_17)
            detachInterrupt(17);
        #if I2C_BUS_NUM >= 2
        else if(i2c->currentPins == I2C_PINS_29_30)
            detachInterrupt(30);
        else if(i2c->currentPins == I2C_PINS_26_31)
            detachInterrupt(31);
        #endif
        if(i2c->user_onReceive != NULL)
        {
            i2c->rxBufferIndex = 0;
            i2c->user_onReceive(i2c->rxBufferLength);
        }
    }
    else
    {
        if(++(i2c->irqCount) >= 2 || !(i2c->currentMode == I2C_SLAVE))
        {
            I2C_DEBUG_STR2("x\n"); // disconnect SDA ISR
            if(i2c->currentPins == I2C_PINS_18_19)
                detachInterrupt(18);
            else if(i2c->currentPins == I2C_PINS_16_17)
                detachInterrupt(17);
            #if I2C_BUS_NUM >= 2
            else if(i2c->currentPins == I2C_PINS_29_30)
                detachInterrupt(30);
            else if(i2c->currentPins == I2C_PINS_26_31)
                detachInterrupt(31);
            #endif
        }
        else
            I2C_DEBUG_STRB2("\n");
    }
}


// ------------------------------------------------------------------------------------------------------
// Instantiate
//
i2c_t3 Wire  = i2c_t3(0);       // I2C0
#if I2C_BUS_NUM >= 2
    i2c_t3 Wire1 = i2c_t3(1);   // I2C1
#endif

#endif // __MK20DX128__ || __MK20DX256__
