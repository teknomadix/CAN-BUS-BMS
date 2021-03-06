/*!
LTC2412: 2-Channel Differential Input 24-Bit No Latency Delta Sigma ADC
LTC2413: 24-Bit No Latency Delta Sigma ADC with Simultaneous 50Hz/60Hz Rejection ADC

@verbatim

The LTC2412 is a 2-channel differential input micropower 24-bit No Latency
Delta-Sigma analog-to-digital converter with an integrated oscillator. It
provides 2ppm INL and 0.16ppm RMS noise over the entire supply range. The two
differential channels are converted alternately with channel ID included in
the conversion results.

The converter accepts any external differential reference voltage from 0.1V to
VCC for flexible ratiometric and remote sensingmeasurement configurations. The
full-scale differential input range is from –0.5VREF to 0.5VREF. The LTC2412
communicates through a flexible 3-wire digital interface which is compatible
with SPI and MICROWIRE protocols.

@endverbatim

http://www.linear.com/product/LTC2412
http://www.linear.com/product/LTC2413

http://www.linear.com/product/LTC2412#demoboards
http://www.linear.com/product/LTC2413#demoboards

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

//! @ingroup Analog_to_Digital_Converters
//! @{
//! @defgroup LTC2412 LTC2412: 2-Channel Differential Input 24-Bit No Latency Delta Sigma ADC
//! @}

/*! @file
    @ingroup LTC2412
    Library for LTC2412: 2-Channel Differential Input 24-Bit No Latency Delta Sigma ADC
*/

#include <stdint.h>
#include <Arduino.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "LTC2412.h"
#include <SPI.h>

int8_t LTC2412_EOC_timeout(uint8_t cs, uint16_t miso_timeout)
// Checks for EOC with a specified timeout
{
  uint16_t timer_count = 0;             // Timer count for MISO
  output_low(cs);                       //! 1) Pull CS low
  while (1)                             //! 2) Wait for SDO (MISO) to go low
  {
    if (input(MISO) == 0) break;        //! 3) If SDO is low, break loop
    if (timer_count++>miso_timeout)     // If timeout, return 1 (failure)
    {
      output_high(cs);                  // Pull CS high
      return(1);
    }
    else
      delay(1);
  }
  return(0);
}

void LTC2412_read(uint8_t cs, uint32_t *adc_code)
// Reads the LTC2412 result
{
  LT_union_int32_4bytes data, command;
  command.LT_byte[3] = 0;
  command.LT_byte[2] = 0;
  command.LT_byte[1] = 0;
  command.LT_byte[0] = 0;

  output_low(cs);                                                       //! 1) Pull CS low
  spi_transfer_block(cs, command.LT_byte, data.LT_byte, (uint8_t)4);    //! 2) Transfer 4 bytes
  output_high(cs);                                                      //! 3) Pull CS high
  *adc_code = data.LT_int32;
}

float LTC2412_code_to_voltage(int32_t adc_code, float LTC2412_lsb , int32_t LTC2412_offset_code)
// Calculates the LTC2412 input voltage
{
  float adc_voltage;
  uint8_t sign;
  sign = (uint8_t)(adc_code >> 29);
  sign = sign & 1;
  adc_code = adc_code>>5;                                           //! 1) Bit-shift ADC code to the right 5 bits
  adc_code = adc_code & 0xFFFFFF;
  if (!sign)
    adc_code -= 16777216;                                              //! 2) Convert ADC code from offset binary to binary
  adc_voltage=((float)adc_code+LTC2412_offset_code)*(LTC2412_lsb);  //! 3) Calculate voltage from ADC code, lsb, offset.
  return(adc_voltage);
}
