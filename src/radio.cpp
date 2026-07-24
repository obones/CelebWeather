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

        #define RH_RF69_PACKETCONFIG1_PACKETFORMAT_FIXED 0x00
        #define RH_RF69_PACKETCONFIG1_CRC_OFF 0x00

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

            const int16_t FSTEP = 61;
            const int16_t DevRegister = round(4500.0 / FSTEP); // See datasheet 3.3.3
            Serial.printf("DevRegister: %2x\n", DevRegister);

            const RH_RF69::ModemConfig cfg =
                {
                    // register RH_RF69_REG_02_DATAMODUL (packet mode, FSK modulation without gaussian shaping)
                    RH_RF69_DATAMODUL_DATAMODE_PACKET | RH_RF69_DATAMODUL_MODULATIONTYPE_FSK | RH_RF69_DATAMODUL_MODULATIONSHAPING_FSK_NONE,
                    // register RH_RF69_REG_03_BITRATEMSB (MSB value for 1200bps, see table 9 from datasheet)
                    0x68,
                    // register RH_RF69_REG_04_BITRATELSB (LSB value for 1200bps, see table 9 from datasheet)
                    0x2b,
                    // register RH_RF69_REG_05_FDEVMSB (deviation most significant bits)
                    DevRegister >> 8,
                    // register RH_RF69_REG_06_FDEVLSB (deviation least significant bits)
                    DevRegister & 0xFF,
                    // register RH_RF69_REG_19_RXBW (reception is not used, default value from datasheet)
                    0x55,
                    // register RH_RF69_REG_1A_AFCBW (reception is not used, recommended default value from datasheet)
                    0x8b,
                    // register RH_RF69_REG_37_PACKETCONFIG1
                    RH_RF69_PACKETCONFIG1_PACKETFORMAT_FIXED | RH_RF69_PACKETCONFIG1_DCFREE_NONE |
                    RH_RF69_PACKETCONFIG1_CRC_OFF | RH_RF69_PACKETCONFIG1_CRCAUTOCLEAROFF |
                    RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE
                };

            rf69.setModemRegisters(&cfg);
            rf69.spiWrite(RH_RF69_REG_38_PAYLOADLENGTH, 0); // length = 0 -> with fixed packet format, this means unlimited length

            rf69.setPreambleLength(0);
            rf69.setTxPower(-12, false);

            Serial.println("---> done");
        }

        #define WAIT_BETWEEN_TRANSMIT 2500

        bool doTransmit(uint8_t* bytes, unsigned int len)
        {
            static unsigned long previousTransmitMillis = 0;

            while (millis() - previousTransmitMillis < WAIT_BETWEEN_TRANSMIT)
            {
                yield();
            }

            rf69.setModeIdle();

            // force FIFO clear
            rf69.spiWrite(RH_RF69_REG_28_IRQFLAGS2, RH_RF69_IRQFLAGS2_FIFOOVERRUN);
            rf69.spiWrite(RH_RF69_REG_28_IRQFLAGS2, 0);

            // set TxStartCondition to 1 (NotEmpty) and FifoThreshold to 48 bytes
            rf69.spiWrite(RH_RF69_REG_3C_FIFOTHRESH, RH_RF69_FIFOTHRESH_TXSTARTCONDITION_NOTEMPTY | (48 & RH_RF69_FIFOTHRESH_FIFOTHRESHOLD));

            // Turn off sync bits
            rf69.spiWrite(RH_RF69_REG_2E_SYNCCONFIG, 0);

            // We would love to use automatic mode to go to TX when FIFO threshold level is reached and
            // exit when FIFO is empty but actual usage showed that the module never gets out of the idle
            // mode so we handle things ourselves
            //rf69.spiWrite(RH_RF69_REG_3B_AUTOMODES, RH_RF69_AUTOMODE_ENTER_COND_FIFO_LEVEL | RH_RF69_AUTOMODE_EXIT_COND_FIFO_EMPTY | RH_RF69_AUTOMODE_INTERMEDIATE_MODE_TX);

            uint8_t irq_flags = rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2);
            Serial.printf("  irq_flags before loop: %2x\n", irq_flags);

            bool first = true;
            int bytesSent = 0;
            uint8_t previousIrqFlags = 0;
            while (bytesSent < len)
            {
                uint8_t irq_flags = rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2);
                uint8_t fifo_full = irq_flags & RH_RF69_IRQFLAGS2_FIFOFULL;
                uint8_t fifo_level = irq_flags & RH_RF69_IRQFLAGS2_FIFOLEVEL;
                uint8_t fifo_overrun = irq_flags & RH_RF69_IRQFLAGS2_FIFOOVERRUN;

                //if (previousIrqFlags != irq_flags)
                //    Serial.printf("  irq_flags changed: %2x != %2x\n", previousIrqFlags, irq_flags);

                if (fifo_overrun)
                {
                    Serial.println("  /!\\ FIFO overrun !");
                    return false;
                }

                if (fifo_full)
                {
                    //if (previousIrqFlags != irq_flags)
                    //    Serial.println("    FIFO is full!");
                    previousIrqFlags = irq_flags;
                    continue;
                }
                if (fifo_level)
                {
                    //if (previousIrqFlags != irq_flags)
                    //    Serial.println("    FIFO is above level!");
                    previousIrqFlags = irq_flags;
                    continue;
                }

                previousIrqFlags = irq_flags;

                uint8_t lengthToSend = (first) ? 64 : 16;
                if (lengthToSend > (len - bytesSent))
                    lengthToSend = len - bytesSent;

                //Serial.printf("  Putting %d bytes in FIFO\n", lengthToSend);
                rf69.spiBurstWrite(RH_RF69_REG_00_FIFO | RH_RF69_SPI_WRITE_MASK, &(bytes[bytesSent]), lengthToSend);
                bytesSent += lengthToSend;

                if (first)
                {
                    Serial.println("  First data sent to FIFO, starting TX");
                    rf69.setModeTx();
                }
                first = false;
            }

            Serial.printf("  irq_flags after loop: %2x\n", rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2));

            Serial.println("  Waiting for FIFO to get empty");
            bool fifoNotEmpty = true;
            while (fifoNotEmpty)
            {
                uint8_t irq_flags = rf69.spiRead(RH_RF69_REG_28_IRQFLAGS2);
                fifoNotEmpty = irq_flags & RH_RF69_IRQFLAGS2_FIFONOTEMPTY;
            }

            Serial.println("  Returning to idle mode");
            rf69.setModeIdle();

            previousTransmitMillis = millis();
            return true;
        }

        bool transmit(uint8_t* bytes, unsigned int len)
        {
            bool result = true;

            // Use an array of frequencies to make it easier to debug on different possible frequencies
            // even if only one will be used in the end product.
            const float frequencies[] = { FREQUENCY }; // { 466.200, 466.202, 466.208, 466.210, 466.220 };
            constexpr int frequenciesLength = sizeof(frequencies) / sizeof(frequencies[0]);
            for (int frequencyIndex = 0; frequencyIndex < frequenciesLength; frequencyIndex++)
            {
                float frequency = frequencies[frequencyIndex];
                Serial.printf("===== Trying %f ====\n", frequency);
                setFrequency(frequency);
                delay(2000);
                result &= doTransmit(bytes, len);
            }

            return result;
        }
    }
}