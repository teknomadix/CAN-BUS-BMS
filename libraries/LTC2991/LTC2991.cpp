/*!
LTC2991: 14-bit Octal I2C Voltage, Current, and Temperature Monitor

@verbatim

The LTC2991 is used to monitor system temperatures, voltages and currents.
Through the I2C serial interface, the eight monitors can individually measure
supply voltages and can be paired for differential measurements of current sense
resistors or temperature sensing transistors. Additional measurements include
internal temperature and internal VCC. The internal 10ppm reference minimizes
the number of supporting components and area required. Selectable address and
configurable functionality give the LTC2991 flexibility to be incorporated in
various systems needing temperature, voltage or current data. The LTC2991 fits
well in systems needing submillivolt voltage resolution, 1% current measurement
and 1 degree Celsius temperature accuracy or any combination of the three.

@endverbatim

http://www.linear.com/product/LTC2991

http://www.linear.com/product/LTC2991#demoboards

REVISION HISTORY
$Revision: 6237 $
$Date: 2016-12-20 15:09:16 -0800 (Tue, 20 Dec 2016) $

Copyright (c) 2013, Linear Technology Corp.(LTC)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of Linear Technology Corp.

The Linear Technology Linduino is not affiliated with the official Arduino team.
However, the Linduino is only possible because of the Arduino team's commitment
to the open-source community.  Please, visit http://www.arduino.cc and
http://store.arduino.cc , and consider a purchase that will help fund their
ongoing work.
*/

//! @ingroup Power_Monitors
//! @{
//! @defgroup LTC2991 LTC2991: 14-bit Octal I2C Voltage, Current, and Temperature Monitor
//! @}

/*! @file
    @ingroup LTC2991
    Library for LTC2991: 14-bit Octal I2C Voltage, Current, and Temperature Monitor
*/

#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_I2C.h"
#include "LTC2991.h"
#include <Wire.h>

// Reads a 14-bit adc_code from LTC2991.
int8_t LTC2991_adc_read(uint8_t i2c_address, uint8_t msb_register_address, int16_t *adc_code, int8_t *data_valid)
{
  int8_t ack = 0;
  uint16_t code;
  ack = i2c_read_word_data(i2c_address, msb_register_address, &code);

  *data_valid = (code >> 15) & 0x01;   // Place Data Valid Bit in *data_valid

  *adc_code = code & 0x7FFF;  // Removes data valid bit to return proper adc_code value

  return(ack);
}

// Reads a 14-bit adc_code from the LTC2991 but enforces a maximum timeout.
// Similar to LTC2991_adc_read except it repeats until the data_valid bit is set, it fails to receive an I2C acknowledge, or the timeout (in milliseconds)
// expires. It keeps trying to read from the LTC2991 every millisecond until the data_valid bit is set (indicating new data since the previous
// time this register was read) or until it fails to receive an I2C acknowledge (indicating an error on the I2C bus).
int8_t LTC2991_adc_read_timeout(uint8_t i2c_address, uint8_t msb_register_address, int16_t *adc_code, int8_t *data_valid, uint16_t timeout, uint8_t status_bit)
{
  int8_t ack = 0;
  uint8_t reg_data;
  uint16_t timer_count;  // Timer count for data_valid

  for (timer_count = 0; timer_count < timeout; timer_count++)
  {

    if (status_bit<8)
    {
      ack |=  LTC2991_register_read(i2c_address, LTC2991_STATUS_LOW_REG, &reg_data); //! 1)Read status register until correct data valid bit is set
    }
    else
    {
      ack |=  LTC2991_register_read(i2c_address, LTC2991_STATUS_HIGH_REG, &reg_data); //! 1)Read status register until correct data valid bit is set
      if (status_bit==8)
      {
        status_bit =1;
      }
      else
      {
        status_bit = 0;
      }
    }

    if ((ack) || (((reg_data>>status_bit)&0x1)==1))
    {
      break;
    }

    delay(1);
  }

  ack |= LTC2991_adc_read(i2c_address, msb_register_address, &(*adc_code), &(*data_valid));   //! 2) It's either valid or it's timed out, we read anyways
  if (*data_valid  !=1)
  {
    Serial.println("Data not valid");
    Serial.println(*data_valid);
    return (1);
  }
  return(ack);
}

// Reads new data (even after a mode change) by flushing old data and waiting for the data_valid bit to be set.
// This function simplifies adc reads when modes are changing.  For example, if V1-V2 changes from temperature mode
// to differential voltage mode, the data in the register may still correspond to the temperature reading immediately
// after the mode change.  Flushing one reading and waiting for a new reading guarantees fresh data is received.
// If the timeout is reached without valid data (*data_valid=1) the function exits.
int8_t LTC2991_adc_read_new_data(uint8_t i2c_address, uint8_t msb_register_address, int16_t *adc_code, int8_t *data_valid, uint16_t timeout)
{
  int8_t ack = 0;

  ack |= LTC2991_adc_read_timeout(i2c_address, msb_register_address, &(*adc_code), &(*data_valid), timeout, ((msb_register_address/2) - 0x05)); //! 1)  Throw away old data
  ack |= LTC2991_adc_read_timeout(i2c_address, msb_register_address, &(*adc_code), &(*data_valid), timeout, ((msb_register_address/2) - 0x05)); //! 2) Read new data

  return(ack);
}

// Reads an 8-bit register from the LTC2991 using the standard repeated start format.
int8_t LTC2991_register_read(uint8_t i2c_address, uint8_t register_address, uint8_t *register_data)
{
  int8_t ack = 0;

  ack = i2c_read_byte_data(i2c_address, register_address, register_data);
  return(ack);
}

// Write one byte to an LTC2991 register.
// Writes to an 8-bit register inside the LTC2991 using the standard I2C repeated start format.
int8_t LTC2991_register_write(uint8_t i2c_address, uint8_t register_address, uint8_t register_data)
{
  int8_t ack = 0;

  ack = i2c_write_byte_data(i2c_address, register_address, register_data);
  return(ack);
}

// Used to set and clear bits in a control register.  bits_to_set will be bitwise OR'd with the register.
// bits_to_clear will be inverted and bitwise AND'd with the register so that every location with a 1 will result in a 0 in the register.
int8_t LTC2991_register_set_clear_bits(uint8_t i2c_address, uint8_t register_address, uint8_t bits_to_set, uint8_t bits_to_clear)
{
  uint8_t register_data;
  int8_t ack = 0;

  ack |= LTC2991_register_read(i2c_address, register_address, &register_data);  //! 1) Read register
  register_data = register_data & (~bits_to_clear); //! 2) Clear bits that were set to be cleared
  register_data = register_data | bits_to_set;
  ack |= LTC2991_register_write(i2c_address, register_address, register_data);  //! 3) Write to register with the cleared bits
  return(ack);
}

// Calculates the LTC2991 single-ended input voltages
float LTC2991_code_to_single_ended_voltage(int16_t adc_code, float LTC2991_single_ended_lsb)
{
  float voltage;
  int16_t sign = 1;
  if (adc_code >> 14)
  {
    adc_code = (adc_code ^ 0x7FFF) + 1;                 //! 1) Converts two's complement to binary
    sign = -1;
  }
  adc_code = (adc_code & 0x3FFF);
  voltage = ((float) adc_code) * LTC2991_single_ended_lsb * sign;   //! 2) Convert code to voltage from lsb
  return (voltage);
}

// Calculates the LTC2991 Vcc voltage
float LTC2991_code_to_vcc_voltage(int16_t adc_code, float LTC2991_single_ended_lsb)
{
  float voltage;
  int16_t sign = 1;
  if (adc_code >> 14)
  {
    adc_code = (adc_code ^ 0x7FFF) + 1;                 //! 1) Converts two's complement to binary
    sign = -1;
  }

  voltage = (((float) adc_code) * LTC2991_single_ended_lsb * sign) + 2.5; //! 2) Convert code to Vcc Voltage from single-ended lsb
  return (voltage);
}

// Calculates the LTC2991 differential input voltage.
float LTC2991_code_to_differential_voltage(int16_t adc_code, float LTC2991_differential_lsb)
{
  float voltage;
  int16_t sign = 1;
  if (adc_code >> 14)
  {
    adc_code = (adc_code ^ 0x7FFF) + 1;                 //! 1)Converts two's complement to binary
    sign = -1;
  }
  voltage = ((float) adc_code) * LTC2991_differential_lsb * sign;   //! 2) Convert code to voltage form differential lsb
  return (voltage);
}

// Calculates the LTC2991 temperature
float LTC2991_temperature(int16_t adc_code, float LTC2991_temperature_lsb, boolean unit)
{
  float temperature;
  adc_code = (adc_code & 0x1FFF);                               //! 1) Removes first 3 bits
  if (!unit)                                                     //! 2)Checks to see if it's Kelvin
  {
    if (adc_code >>12)
    {
      adc_code = (adc_code | 0xE000);                         //! Sign extend if it's not Kelvin (Celsius)
    }
  }
  temperature = ((float) adc_code) * LTC2991_temperature_lsb;   //! 3) Converts code to temperature from temperature lsb

  return (temperature);
}

//  Calculates the LTC2991 diode voltage
float LTC2991_code_to_diode_voltage(int16_t adc_code, float LTC2991_diode_voltage_lsb)
{
  float voltage;
  adc_code = (adc_code & 0x1FFF);                               //! 1) Removes first 3 bits
  voltage = ((float) adc_code) * LTC2991_diode_voltage_lsb;     //! 2) Convert code to voltage from diode voltage lsb
  return (voltage);
}
