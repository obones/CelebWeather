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

        //#define FREQUENCY 433.0
        #define FREQUENCY 466.206250

        #define CONFIG_FSK (RH_RF69_DATAMODUL_DATAMODE_PACKET | RH_RF69_DATAMODUL_MODULATIONTYPE_FSK | RH_RF69_DATAMODUL_MODULATIONSHAPING_FSK_NONE)
        #define CONFIG_NOWHITE (RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RH_RF69_PACKETCONFIG1_DCFREE_NONE | RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE)

        RH_RF69 rf69(RFM69_CS, RFM69_INT);

        bool setFrequency(float freq)
        {
            // borrowed from RadioLib
            if(!(((freq > 290.0) && (freq < 340.0)) ||
                ((freq > 431.0) && (freq < 510.0)) ||
                ((freq > 862.0) && (freq < 1020.0))))
            {
                Serial.printf("Invalid frequency for RF69: %f", freq);
                return false;
            }

            return rf69.setFrequency(freq);
        }

        void reset()
        {
            digitalWrite(RFM69_RST, HIGH);
            delay(1);
            digitalWrite(RFM69_RST, LOW);
            delay(10);
        }

        void setup()
        {
            Serial.println("===> Setting up radio");
            pinMode(RFM69_RST, OUTPUT);
            pinMode(RFM69_CS, OUTPUT);
            pinMode(RFM69_INT, INPUT);

            if (!rf69.init())
                Serial.println("init failed");

            reset();

            if (!setFrequency(FREQUENCY))
                Serial.println("setFrequency failed");

            // Set registers for 1200bps with 4.5KHz deviation.
            // See the notes folder for an explanation of the values.
            const RH_RF69::ModemConfig cfg = { 0x00, 0x68, 0x2b, 0x00, 0x4a, 0xe2, 0xe2, CONFIG_NOWHITE };
            rf69.setModemRegisters(&cfg);
            rf69.setPreambleLength(0);
            rf69.setTxPower(0, false);
            
            Serial.println("---> done");
        }

        #define WAIT_BETWEEN_TRANSMIT 5000

        bool transmit(uint8_t* bytes, unsigned int len)
        {
            static unsigned long previousTransmitMillis = 0;

            while (millis() - previousTransmitMillis < WAIT_BETWEEN_TRANSMIT)
            {
                yield();
            }

            //rf69.waitPacketSent();
            rf69.setModeIdle();

            // force FIFO clear
            rf69.spiWrite(RH_RF69_REG_28_IRQFLAGS2, RH_RF69_IRQFLAGS2_FIFOOVERRUN);
            rf69.spiWrite(RH_RF69_REG_28_IRQFLAGS2, 0);

            Serial.println("  Waiting for CAD");
            //while (!rf69.waitCAD());

            // Set FifoThreshold to 32 bytes (out of 68)
            rf69.spiWrite(RH_RF69_REG_3C_FIFOTHRESH, 0xa0);

            // Turn off sync bits
            rf69.spiWrite(RH_RF69_REG_2E_SYNCCONFIG, 0);

            // Set exit condition to fifo empty
            rf69.spiWrite(RH_RF69_REG_3B_AUTOMODES, RH_RF69_AUTOMODE_ENTER_COND_NONE | RH_RF69_AUTOMODE_EXIT_COND_FIFO_EMPTY | RH_RF69_AUTOMODE_INTERMEDIATE_MODE_SLEEP); // 0x04

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
                    Serial.println("  /!\\ FIFO overrun !");
                    return false;
                }

                if (fifo_full || fifo_level)
                {
                    //Serial.println("    FIFO is above level!");
                    continue;
                }

                uint8_t _len = 16;
                if (_len > (len - b))
                    _len = len - b;

                //Serial.printf("  Putting %d bytes in FIFO\n", _len);
                rf69.spiBurstWrite(RH_RF69_REG_00_FIFO | RH_RF69_SPI_WRITE_MASK, &(data_i[b]), _len);
                b += _len;

                if (first)
                {
                    //Serial.println("  First data sent to FIFO, starting TX");
                    rf69.setModeTx();
                }
                first = false;
            }

            Serial.println("  Waiting for FIFO to get empty");
            bool fifoNotEmpty = true;
            while (fifoNotEmpty)
            {
                uint8_t irq_flags = rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2);
                fifoNotEmpty = irq_flags & RH_RF69_IRQFLAGS2_FIFONOTEMPTY;
            }

            Serial.println("  Returning to idle mode");
            rf69.setModeIdle();

            return true;
        }
    }
}