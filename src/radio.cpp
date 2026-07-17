/*
 CelebWeather project

 The Original Code is radio.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
// Inspired by https://github.com/AaronJackson/rfm69-pocsag/blob/main/rfm69-pocsag/rfm69-pocsag.ino
#include <RH_RF69.h>

#include "radio.h"

namespace CelebWeather
{
    namespace Radio
    {
        #define RFM69_CS 5
        #define RFM69_RST 4
        #define RFM69_INT 13

        #define FREQUENCY 433.0

        #define CONFIG_FSK (RH_RF69_DATAMODUL_DATAMODE_PACKET | RH_RF69_DATAMODUL_MODULATIONTYPE_FSK | RH_RF69_DATAMODUL_MODULATIONSHAPING_FSK_NONE)
        #define CONFIG_NOWHITE (RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RH_RF69_PACKETCONFIG1_DCFREE_NONE | RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE)

        RH_RF69 rf69(RFM69_CS, RFM69_INT);

        void setup()
        {
            pinMode(RFM69_RST, OUTPUT);
            pinMode(RFM69_CS, OUTPUT);
            pinMode(RFM69_INT, INPUT);

            if (!rf69.init())
                Serial.println("init failed");

            if (!rf69.setFrequency(FREQUENCY))
                Serial.println("setFrequency failed");

            // Set registers for 1200bps with 4.5KHz deviation.
            // See the notes folder for an explanation of the values.
            const RH_RF69::ModemConfig cfg = { 0x00, 0x68, 0x2b, 0x00, 0x4a, 0xe2, 0xe2, CONFIG_NOWHITE };
            rf69.setModemRegisters(&cfg);
            rf69.setPreambleLength(0);
            rf69.setTxPower(0, false);
        }

        bool transmit(uint8_t* bytes, unsigned int len)
        {
            //rf69.waitPacketSent();
            rf69.setModeIdle();

            Serial.println("  Waiting for CAD");
            //while (!rf69.waitCAD());

            // Set FifoThreshold to 32 bytes (out of 68)
            rf69.spiWrite(0x3c, 0xa0);

            // Turn off sync bits
            rf69.spiWrite(0x2e, 0);

            // Set exit condition to fifo empty
            rf69.spiWrite(0x3b, 0x04);

            // invert bits
            uint8_t* data_i = bytes;
            /*uint8_t data_i[len];
            for (int i = 0; i < len; i++)
                data_i[i] = ~bytes[i];*/

            Serial.println("  Starting send loop");
            int b = 0;
            bool first = true;
            while (b < len)
            {
                uint8_t irq_flags = rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2);
                uint8_t fifo_full = irq_flags % RH_RF69_IRQFLAGS2_FIFOFULL;
                uint8_t fifo_level = irq_flags % RH_RF69_IRQFLAGS2_FIFOLEVEL;
                uint8_t fifo_overrun = irq_flags % RH_RF69_IRQFLAGS2_FIFOOVERRUN;
                if (fifo_overrun)
                {
                    Serial.println("FIFO overrun !");
                    return false;
                }

                if (fifo_full || fifo_level)
                {
                    Serial.println("FIFO is above level!");
                    continue;
                }

                uint8_t _len = 16;
                if (_len > (len - b))
                    _len = len - b;

                Serial.printf("  Putting %d bytes in FIFO", _len);
                rf69.spiBurstWrite(RH_RF69_REG_00_FIFO | RH_RF69_SPI_WRITE_MASK, &(data_i[b]), _len);
                b += _len;

                if (first)
                {
                    Serial.println("  First data sent to FIFO, starting TX");
                    rf69.setModeTx();
                }
                first = false;
            }

            Serial.println("  Waiting for FIFO to get empty");
            bool fifoNotEmpty = true;
            while (!fifoNotEmpty)
            {
                uint8_t irq_flags = rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2);
                fifoNotEmpty = irq_flags & RH_RF69_IRQFLAGS2_FIFONOTEMPTY;
            }

            Serial.println("  Returning to idle mode");
            rf69.setModeIdle();

            Serial.println("  Done.");
            return true;
        }
    }
}