// Largely inspired by https://github.com/jfdelnero/rf-tools/blob/master/src/pocsag/
//
// File : pocsag.c
// Contains: a pocsag transmitter
//
// This file is part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2022-2026 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
//     Adapted by Olivier Sannier for the CelebWeather project
///////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

/*#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>*/
//#include <sys/types.h>
/*#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>*/

//#include <stdint.h>
/*
#include "cmd_param.h"

#include "wave.h"
#include "modulator.h"
#include "serial.h"
#include "utils.h"*/

namespace CelebWeather
{
    namespace Pocsag
    {
        //LITTLE ENDIAN HOST
        // big endian
        #define BIGENDIAN_WORD(wValue) ( (((wValue)<<8)&0xFF00) | (((wValue)>>8)&0xFF) )
        #define BIGENDIAN_DWORD(dwValue) ( (((dwValue&0xFF)<<24)) | (((dwValue&0xFF00)<<8)) | (((dwValue)>>8)&0xFF00) | (((dwValue)>>24)&0xFF) )

        // little endian
        #define LITTLEENDIAN_WORD(wValue) (wValue)
        #define LITTLEENDIAN_DWORD(dwValue) (dwValue)

        // from ../common/utils.c
        const unsigned char LUT_ByteBitsInverter[] =
        {
            0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
            0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
            0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
            0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
            0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
            0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
            0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
            0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
            0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
            0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
            0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
            0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
            0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
            0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
            0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
            0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
            0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
            0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
            0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
            0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
            0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
            0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
            0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
            0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
            0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
            0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
            0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
            0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
            0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
            0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
            0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
            0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
        };

        const unsigned char LUT_QuartetBitsInverter[] =
        {
            0x0, 0x8, 0x4, 0xC,
            0x2, 0xA, 0x6, 0xE,
            0x1, 0x9, 0x5, 0xD,
            0x3, 0xB, 0x7, 0xF
        };

        #pragma pack(1)

        typedef struct _poc_batch
        {
            uint32_t sync;
            uint32_t words[8*2];
        } poc_batch;

        #pragma pack()

        typedef struct _poc
        {
            int preamble_cnt;
            int frm_cnt;
            int frm_end;
        }poc;

/*        void printhelp(char* argv[])
        {
            printf("Options:\n");
            printf("  -stdout \t\t\t: IQ stream send to stdout\n");
            printf("  -generate \t\t\t: Generate the IQ stream\n");
            printf("  -baud:[BAUD]\t\t\t: Baud rate setting (Default:1200)\n");
            printf("  -ric:[RIC]\t\t\t: RIC address (Default:8)\n");
            printf("  -func:[function_code]\t\t: Function code (Default:0 Min/Max:0-3)\n");
            printf("  -freqshift:[Hz]\t\t: FSK frequency shift (Default:4500 -> +4500Hz / -4500Hz)\n");
            printf("  -smprate:[Hz]\t\t\t: IQ Sample rate (Default:2000000)\n");
            printf("  -alpha\t\t\t: Alphanumeric mode (Default: Numeric mode)\n");
            printf("  -message:message_txt\t\t: Message to send\n");
            printf("  -fmessage:message_file.txt\t: File message to send\n");
            printf("  -stdin_message\t\t: Message to send from stdin\n");
            printf("  -level:[0-127]\t\t: Output level (Default: 126)\n");
            printf("  -settle_time:[0-100]\t\t: Frequency change settle time, in percent of a bit period (Default: 20)\n");
            printf("  -bits_stream_out\t\t: Enable bits stream output\n");
            printf("  -verbose \t\t\t: Verbose mode\n");
            printf("  -help \t\t\t: This help\n\n");
            printf("Example : ./pocsag -generate -stdout -ric:154232 -message:\"Hello\" -alpha | hackrf_transfer  -f 466200500 -t -  -x 10 -a 0 -s 2000000\n");
            printf("\n");
        }
*/
        uint32_t adr_codeword(uint32_t ric_address, uint8_t func)
        {
            uint32_t val;

            // Get the 18 bits most significant bits ric address part.
            // The 3 lower bits is the frame position

            val =   ( ( (ric_address>>3) & ((0x1<<18)-1) ) << 13 ) |
                    ( ( (uint32_t)(func&3)) << 11);

            return val;
        }

        // BCH(31,21) (21 bit data - 31 bit code word, without the parity)
        // g(x) = x^10 + x^9 + x^8 + x^6 + x^5 + x^3 + 1

        uint32_t calc_pocsag_bch(uint32_t codeword)
        {
            uint32_t bit = 0;
            uint32_t parity = 0;
            uint32_t tmp_codeword;
            uint32_t shiftr;

            tmp_codeword = codeword & 0xFFFFF800;
            shiftr = tmp_codeword;

            // BCH bits
            for (bit = 1; bit <= 21; bit++)  // 21 bits of data
            {
                if (shiftr & 0x80000000)
                {
                    shiftr ^= (0x769U << (32U - (31-21) - 1U));
                }
                shiftr <<= 1;
            }

            tmp_codeword |= (shiftr >> 21);

            // Parity bit
            shiftr = tmp_codeword;
            bit = 32;
            while ((shiftr != 0) && (bit > 0))
            {
                if (shiftr & 1)
                {
                    parity++;
                }
                shiftr >>= 1;
                bit--;
            }

            return tmp_codeword | (parity&1);
        }

        unsigned char pocsag_nextbyte(poc * ctx, poc_batch * batches, int batchcnt)
        {
            if(ctx->preamble_cnt < (72 + 32) )
            {
                // > 576 bits
                ctx->preamble_cnt++;
                return 0xAA;
            }

            if(ctx->frm_cnt < ( sizeof(poc_batch) * batchcnt ) )
            {
                uint8_t * ptr;

                ptr = (uint8_t *)batches;

                unsigned char val = *(ptr + ctx->frm_cnt);

                ctx->frm_cnt++;

                return val;
            }
            else
            {
                ctx->frm_end = 1;
                return 0x00;
            }


            return 0x00;
        }

        #define POCSAG_SYNC_CW 0x7CD215D8
        #define POCSAG_IDLE_CW 0x7A89C197

        void init_batch(poc_batch * batch)
        {
            batch->sync = BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_SYNC_CW) );
            for(int i=0;i<16;i++)
            {
                batch->words[i] = BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_IDLE_CW) );
            }
        }

/*        int set_numerical_frame(poc_batch * batch, int batchcnt, unsigned int ric, unsigned int func, char * message)
        {
            int ba,b,i,w,framecnt;
            int digitcnt;
            uint8_t charcode;
            uint32_t word;

            i = 0;
            while(message[i] != 0)
            {
                i++;
            }

            digitcnt = i;

            framecnt = digitcnt / 5;
            if( digitcnt % 5 )
                framecnt++;

            if( framecnt > batchcnt)
            {
                return -1;
            }

            ba = 0;
            w = (ric&7)*2;

            batch[ba].words[w++] = BIGENDIAN_DWORD( calc_pocsag_bch(adr_codeword(ric, func)));

            b = 0;

            digitcnt = 0;

            // 4 bits per digit - bits 30 <>11 -> 5 digits per code word
            word = 0x80000000;
            i = 0;
            while(message[i] != 0)
            {
                switch(message[i])
                {
                    case 'U':
                        charcode = 0xB;
                    break;
                    case ' ':
                        charcode = 0xC;
                    break;
                    case '-':
                        charcode = 0xD;
                    break;
                    case ']':
                        charcode = 0xE;
                    break;
                    case '[':
                        charcode = 0xF;
                    break;
                    default:
                        if(message[i]>='0' && message[i] <= '9')
                        {
                            charcode = (message[i] - '0')&0xF;
                        }
                        else
                        {
                            charcode = 0xA;
                        }
                    break;
                }

                word |= (((uint32_t)(LUT_QuartetBitsInverter[charcode]))<<(27-((b%5)*4)) );

                b++;
                if(!(b%5))
                {
                    batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) ); // Spaces

                    w++;

                    if( w >= 16)
                    {
                        w = 0;
                        ba++;
                    }

                    word = 0x80000000;
                }
                i++;
            }

            if((b%5))
            {
                while((b%5))
                {
                    word |= ( ((uint32_t)LUT_QuartetBitsInverter[0xC])<<(27-((b%5)*4)) );
                    b++;
                }

                batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) );
            }

            if( ( batch[ba].words[15] != BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_IDLE_CW) ) ) )
            {
                return ba+2;
            }
            else
            {
                return ba+1;
            }

            return ba+1;
        }
*/
        int set_alphanum_frame(poc_batch * batch, int batchcnt, unsigned int ric, unsigned int func, const unsigned char * message, int message_size)
        {
            int ba,b,i,w,framecnt;
            int digitcnt;
            uint8_t charcode;
            uint32_t word;
            unsigned char stream[80+1];
            int bitin,bitout;

            digitcnt = message_size;

            framecnt = (digitcnt*7) / 20;
            if( (digitcnt*7) % 20 )
                framecnt++;

            if( framecnt > batchcnt)
            {
                Serial.printf("framecnt > batchcnt (%d > %d)\n", framecnt, batchcnt);
                return -1;
            }

            memset(&stream,0,sizeof(stream));
            i = 0;
            bitin = 0;
            bitout = 0;
            while( ((bitin/7)<message_size) && ((bitout>>3) < sizeof(stream)-1) )
            {
                if( LUT_ByteBitsInverter[(message[bitin/7]&0x7F)<<1] & ( 0x40 >> (bitin%7) ) )
                    stream[bitout>>3] |= ( 0x80 >> (bitout&7) );

                bitin++;
                bitout++;
            }

            stream[sizeof(stream)-1] = 0;

            digitcnt = bitout >> 2;
            if(bitout&3)
                digitcnt++;

            ba = 0;
            w = (ric&7)*2;

            batch[ba].words[w++] = BIGENDIAN_DWORD( calc_pocsag_bch(adr_codeword(ric, func)));

            b = 0;

            // 4 bits per digit - bits 30 <>11 -> 5 digits per code word
            word = 0x80000000;
            i = 0;
            while( i < digitcnt )
            {
                charcode = (stream[i>>1] >> (4*((i&1)^1)) & 0xF);
                if( charcode > 127 )
                    charcode = ' ';

                word |= (((uint32_t)(charcode))<<(27-((b%5)*4)) );

                b++;
                if(!(b%5))
                {
                    batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) ); // Spaces

                    w++;

                    if( w >= 16)
                    {
                        w = 0;
                        ba++;
                    }

                    word = 0x80000000;
                }
                i++;
            }

            if((b%5))
            {
                while((b%5))
                {
                    word |= ( ((uint32_t)0x0)<<(27-((b%5)*4)) );
                    b++;
                }

                batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) );
            }

            if( ( batch[ba].words[15] != BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_IDLE_CW) ) ) )
            {
                return ba+2;
            }
            else
            {
                return ba+1;
            }

            return ba+1;
        }

        #define MAXBATCHCNT 40 //128

        //  -generate -ric:25176 -func:3 -alpha -stdin_message -bits_stream_out -quiet

        int GetBits(const unsigned char* frame, size_t frameSize)
        {
            int ric = 25176;
            int func = 3;
            int alpha = 1;
            //int data_stream_mode = 1;

            poc pocctx = {};
            poc_batch* batches = new poc_batch[MAXBATCHCNT];//[MAXBATCHCNT];

            //serial_init(&ser, 8, smprate, baud);

            //memset(&pocctx,0,sizeof(pocctx));

            for(int i=0;i<MAXBATCHCNT;i++)
            {
                init_batch((poc_batch *)&batches[i]);
            }

            Serial.println("Generating batches");
            int batchesCount = set_alphanum_frame(batches, MAXBATCHCNT-1, ric, func&3, frame, frameSize);
            Serial.print("Generated "); Serial.print(batchesCount); Serial.println(" batches");

            if( batchesCount < 0 )
            {
                //free(settle_buf);
                //exit(1);
                delete batches;
                return 0;
            }

            for(int batchIndex = 0; batchIndex < batchesCount; batchIndex++)
            {
                auto batch = batches[batchIndex];

                Serial.printf("Batch %d: \n", batchIndex);
                Serial.printf("  sync: %08x\n", batch.sync);
                Serial.print("  words: ");
                for (int wordIndex = 0; wordIndex < sizeof(batch.words) / sizeof(batch.words[0]); wordIndex++)
                    Serial.printf("%08x ", batch.words[wordIndex]);
                Serial.println();
            }

            Serial.println("Reading bytes out of batches");
            while( !pocctx.frm_end )
            {
                unsigned char tmp = pocsag_nextbyte(&pocctx, batches, batchesCount);
                Serial.printf("%02x ", tmp);
            }
            Serial.println();

            delete batches;
            return batchesCount;

/*            // Blank
            i = 0;
            while(i < (smprate/20)) // ~ 50 ms
            {
                for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
                {
                    iq_wave_buf[j] = 0;
                }
                write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
                i += BUFFER_IQ_SAMPLES_SIZE;
            }

            freqidx = settle_size / 2;

            // Initial 750Hz tone
            serial_init(&ser, 8, smprate, 750*2); // 750 Hz tone

            iqgen.Amplitude = 0;

            // Fade in
            volinc = (float)level / (float)(smprate/60);

            i = 0;
            while(i < (smprate/5)) // ~ 200 ms
            {
                for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
                {
                    if(iqgen.Amplitude+volinc <= level)
                        iqgen.Amplitude += volinc;

                    if(serial_is_tx_reg_empty(&ser))
                    {
                        serial_tx_settxreg(&ser,0xAA);
                    }

                    ser_state = serial_tx_getlinestate(&ser);

                    if( ser_state & 1 )
                    {
                        if(freqidx < settle_size - 1)
                            freqidx++;
                    }
                    else
                    {
                        if(freqidx)
                            freqidx--;
                    }

                    iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
                    iq_wave_buf[j] = get_next_iq(&iqgen);
                }

                write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
                i += BUFFER_IQ_SAMPLES_SIZE;
            }

            serial_init(&ser, 8, smprate, baud);

            // Main loop :  Generate the message
            while( !pocctx.frm_end || !serial_is_tx_reg_empty(&ser) )
            {
                j = 0;
                while( ( j < BUFFER_IQ_SAMPLES_SIZE ) && ( !pocctx.frm_end || !serial_is_tx_reg_empty(&ser) ) )
                {
                    if(serial_is_tx_reg_empty(&ser))
                    {
                        unsigned char tmp = pocsag_nextbyte(&pocctx, (poc_batch *)&batches, batchescnt);
                        if(!pocctx.frm_end)
                            serial_tx_settxreg(&ser,tmp);
                    }

                    ser_state = serial_tx_getlinestate(&ser);

                    if( ser_state & 1 )
                    {
                        if(freqidx < settle_size - 1)
                            freqidx++;
                    }
                    else
                    {
                        if(freqidx)
                            freqidx--;
                    }

                    if( (ser_state & 0x2) && data_stream_mode )
                    {
                        printf("%d", ser_state & 1);
                    }

                    iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
                    iq_wave_buf[j] = get_next_iq(&iqgen);
                    j++;
                }

                write_wave(wave1, &iq_wave_buf,j);
            }

            // Fade out + Blank
            i = 0;
            volinc = iqgen.Amplitude / (smprate/40);
            while(i < (smprate/40)) // ~ 50 ms
            {
                for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
                {
                    if(iqgen.Amplitude >= volinc)
                        iqgen.Amplitude -= volinc;
                    else
                        iqgen.Amplitude = 0;

                    ser_state = serial_tx_getlinestate(&ser);

                    if( ser_state & 1 )
                    {
                        if(freqidx < settle_size - 1)
                            freqidx++;
                    }
                    else
                    {
                        if(freqidx)
                            freqidx--;
                    }

                    iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
                    iq_wave_buf[j] = get_next_iq(&iqgen);
                }
                write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
                i+= BUFFER_IQ_SAMPLES_SIZE;
            }
*/
        }

/*        int main(int argc, char* argv[])
        {
            char temp_str[512];
            unsigned int i,j;
            char message[512];
            int ric;
            int func;
            int baud;
            int smprate;
            int freqshift;
            int batchescnt;
            int alpha;
            int batchbruteforce;
            int wavout;
            int settle_time;
            int settle_size;
            int freqidx;
            int * settle_buf;
            double volinc;
            int level;
            int verbose;
            int message_size;
            int ser_state;
            int data_stream_mode;
            int quiet;

            wave_io * wave1;

            iq_wave_gen iqgen;

            poc pocctx;
            poc_batch batches[MAXBATCHCNT];

            uint16_t iq_wave_buf[BUFFER_IQ_SAMPLES_SIZE];

            serial_gen ser;

            quiet = 0;

            message_size = 0;

            memset(&pocctx,0,sizeof(pocctx));

            stdoutmode = 0;
            if(isOption(argc,argv,"stdout",NULL,NULL)>0)
            {
                stdoutmode = 1;
            }

            baud = 1200;
            if(isOption(argc,argv,"baud",(char*)&temp_str,NULL)>0)
            {
                baud = atoi(temp_str);
            }

            ric = 8;
            if(isOption(argc,argv,"ric",(char*)&temp_str,NULL)>0)
            {
                ric = atoi(temp_str);
            }

            func = 0;
            if(isOption(argc,argv,"func",(char*)&temp_str,NULL)>0)
            {
                func = atoi(temp_str);
            }

            freqshift = 4500;
            if(isOption(argc,argv,"freqshift",(char*)&temp_str,NULL)>0)
            {
                freqshift = atoi(temp_str);
            }

            data_stream_mode = 0;
            if(isOption(argc,argv,"bits_stream_out",NULL,NULL)>0)
            {
                data_stream_mode = 1;
                stdoutmode = 0;
            }

            message[0] = 0;
            if(isOption(argc,argv,"message",(char*)&message,NULL)>0)
            {
                message_size = strlen(message);
            }

            temp_str[0] = 0;
            if(isOption(argc,argv,"fmessage",(char*)&temp_str,NULL)>0)
            {
                if(strlen(temp_str))
                {
                    FILE * f;
                    int size;

                    message_size = 0;
                    f = fopen(temp_str,"rb");
                    if(f)
                    {
                        memset( message,0, sizeof(message));

                        fseek(f, 0, SEEK_END);

                        size = ftell(f);

                        if(size >= sizeof(message))
                            size = sizeof(message) - 1;

                        fseek(f, 0, SEEK_SET);
                        if(fread(&message,size,1,f) != 1 )
                        {
                            fprintf(stderr,"Error : Can't read %s ...\n",temp_str);
                        }
                        else
                        {
                            message_size = size;
                        }

                        fclose(f);
                    }
                    else
                    {
                        fprintf(stderr,"Error : Can't open %s ...\n",temp_str);
                    }
                }
            }

            if(isOption(argc,argv,"stdin_message",NULL,NULL)>0)
            {
                memset( message,0, sizeof(message));
                if(fgets(message, sizeof(message), stdin))
                {
                    if (message[strlen(message)-1] == '\n')
                    {
                        message[strlen(message)-1] = 0;
                    }
                    message_size = strlen(message);
                }
            }

            alpha = 0;
            if(isOption(argc,argv,"alpha",NULL,NULL)>0)
            {
                alpha = 1;
            }

            smprate = IQ_SAMPLE_RATE;
            if(isOption(argc,argv,"smprate",(char*)&temp_str,NULL)>0)
            {
                smprate = atoi(temp_str);
            }

            batchbruteforce = 0;
            if(isOption(argc,argv,"batch",NULL,NULL)>0)
            {
                batchbruteforce = 1;
            }

            wavout = 0;
            if(isOption(argc,argv,"wav",NULL,NULL)>0)
            {
                wavout = 1;
            }

            settle_time = 20;
            if(isOption(argc,argv,"settle_time",(char*)&temp_str,NULL)>0)
            {
                settle_time = atoi(temp_str);
                if( settle_time < 0 )
                    settle_time = 0;

                if( settle_time > 100 )
                    settle_time = 100;
            }

            level = 126;
            if(isOption(argc,argv,"level",(char*)&temp_str,NULL)>0)
            {
                level = atoi(temp_str);
                if( level < 0 )
                    level = 0;

                if( level > 127 )
                    level = 127;
            }

            verbose = 0;
            if(isOption(argc,argv,"verbose",NULL,NULL)>0)
            {
                verbose = 1;
            }

            quiet = 0;
            if(isOption(argc,argv,"quiet",NULL,NULL)>0)
            {
                quiet = 1;
            }

            if(!stdoutmode && !quiet)
            {
                printf("pocsag v0.0.1.1\n");
                printf("Copyright (C) 2026 Jean-Francois DEL NERO\n");
                printf("This program comes with ABSOLUTELY NO WARRANTY\n");
                printf("This is free software, and you are welcome to redistribute it\n");
                printf("under certain conditions;\n\n");
            }

            // help option...
            if(isOption(argc,argv,"help",0,NULL)>0)
            {
                printhelp(argv);
            }

            settle_size = (int) ( (float)smprate / (float)baud*2 ) * ((float)settle_time/100.0);
            if(settle_size<2)
                settle_size = 2;


            if(isOption(argc,argv,"generate",0,NULL)>0)
            {
                if(!quiet)
                    fprintf(stderr,"\nBaud:%d, RIC: %d, Function:%d, Alpha:%d, Message:%s\n", baud, ric, func, alpha, message );

                settle_buf = calloc( 1, settle_size * sizeof(int) );
                if(!settle_buf)
                    exit(-1);

                for(i=0;i<settle_size;i++)
                {
                    settle_buf[i] = cos( (double)i * ( (double)M_PI / (double)(settle_size-1)) ) * freqshift;
                }

                // IQ Modulator
                iqgen.phase = 0;
                iqgen.Frequency = CENTRAL_IF_FREQ;
                iqgen.Amplitude = 0;
                iqgen.sample_rate = smprate;

                wave1 = NULL;

                if( !data_stream_mode )
                {
                    if(stdoutmode)
                    {
                        // stdout / stream mode : IQ are outputed to the stdout -> use a pipe to hackrf_transfer
                        wave1 = create_wave(NULL,iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
                    }
                    else
                    {
                        // file mode : create iq + wav files
                        if(wavout)
                            wave1 = create_wave("out.wav",iqgen.sample_rate,WAVE_FILE_FORMAT_WAV_8BITS_STEREO);
                        else
                            wave1 = create_wave("out.sdriq",iqgen.sample_rate,WAVE_FILE_FORMAT_SDRIQ_8BITS_IQ);
                    }
                }

                if(wave1 || data_stream_mode)
                {
                    serial_init(&ser, 8, smprate, baud);

                    memset(&pocctx,0,sizeof(pocctx));

                    for(i=0;i<MAXBATCHCNT;i++)
                    {
                        init_batch((poc_batch *)&batches[i]);
                    }

                    if(batchbruteforce)
                    {
                        int ricbatch;

                        ricbatch = ric & ~7;

                        for(j=0;j<16;j++)
                        {
                            for(i=0;i<8;i++)
                            {
                                if( alpha )
                                    batchescnt =  set_alphanum_frame((poc_batch *)&batches[j], MAXBATCHCNT-1, ricbatch+i, func&3, (char*)&message, message_size);
                                else
                                    batchescnt = set_numerical_frame((poc_batch *)&batches[j], MAXBATCHCNT-1, ricbatch+i, func&3, (char*)&message);
                            }
                            ricbatch += 8;
                        }
                        batchescnt += j;
                    }
                    else
                    {
                        if( alpha )
                            batchescnt =  set_alphanum_frame((poc_batch *)&batches, MAXBATCHCNT-1, ric, func&3, (char*)&message, message_size);
                        else
                            batchescnt = set_numerical_frame((poc_batch *)&batches, MAXBATCHCNT-1, ric, func&3, (char*)&message);
                    }

                    if(verbose)
                    {
                        fprintf(stderr,"\nFrames/batches data:\n");
                        unsigned char * ptr;
                        ptr = (unsigned char *)&batches;
                        for(i=0;i<sizeof(poc_batch) * batchescnt;i++)
                        {
                            if(!(i&0xF))
                                fprintf(stderr,"\n");
                            fprintf(stderr,"%.2X ",*ptr++);
                        }
                        fprintf(stderr,"\n");
                    }

                    if( batchescnt < 0 )
                    {
                        free(settle_buf);
                        exit(1);
                    }

                    // Blank
                    i = 0;
                    while(i < (smprate/20)) // ~ 50 ms
                    {
                        for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
                        {
                            iq_wave_buf[j] = 0;
                        }
                        write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
                        i += BUFFER_IQ_SAMPLES_SIZE;
                    }

                    freqidx = settle_size / 2;

                    // Initial 750Hz tone
                    serial_init(&ser, 8, smprate, 750*2); // 750 Hz tone

                    iqgen.Amplitude = 0;

                    // Fade in
                    volinc = (float)level / (float)(smprate/60);

                    i = 0;
                    while(i < (smprate/5)) // ~ 200 ms
                    {
                        for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
                        {
                            if(iqgen.Amplitude+volinc <= level)
                                iqgen.Amplitude += volinc;

                            if(serial_is_tx_reg_empty(&ser))
                            {
                                serial_tx_settxreg(&ser,0xAA);
                            }

                            ser_state = serial_tx_getlinestate(&ser);

                            if( ser_state & 1 )
                            {
                                if(freqidx < settle_size - 1)
                                    freqidx++;
                            }
                            else
                            {
                                if(freqidx)
                                    freqidx--;
                            }

                            iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
                            iq_wave_buf[j] = get_next_iq(&iqgen);
                        }

                        write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
                        i += BUFFER_IQ_SAMPLES_SIZE;
                    }

                    serial_init(&ser, 8, smprate, baud);

                    // Main loop :  Generate the message
                    while( !pocctx.frm_end || !serial_is_tx_reg_empty(&ser) )
                    {
                        j = 0;
                        while( ( j < BUFFER_IQ_SAMPLES_SIZE ) && ( !pocctx.frm_end || !serial_is_tx_reg_empty(&ser) ) )
                        {
                            if(serial_is_tx_reg_empty(&ser))
                            {
                                unsigned char tmp = pocsag_nextbyte(&pocctx, (poc_batch *)&batches, batchescnt);
                                if(!pocctx.frm_end)
                                    serial_tx_settxreg(&ser,tmp);
                            }

                            ser_state = serial_tx_getlinestate(&ser);

                            if( ser_state & 1 )
                            {
                                if(freqidx < settle_size - 1)
                                    freqidx++;
                            }
                            else
                            {
                                if(freqidx)
                                    freqidx--;
                            }

                            if( (ser_state & 0x2) && data_stream_mode )
                            {
                                printf("%d", ser_state & 1);
                            }

                            iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
                            iq_wave_buf[j] = get_next_iq(&iqgen);
                            j++;
                        }

                        write_wave(wave1, &iq_wave_buf,j);
                    }

                    // Fade out + Blank
                    i = 0;
                    volinc = iqgen.Amplitude / (smprate/40);
                    while(i < (smprate/40)) // ~ 50 ms
                    {
                        for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
                        {
                            if(iqgen.Amplitude >= volinc)
                                iqgen.Amplitude -= volinc;
                            else
                                iqgen.Amplitude = 0;

                            ser_state = serial_tx_getlinestate(&ser);

                            if( ser_state & 1 )
                            {
                                if(freqidx < settle_size - 1)
                                    freqidx++;
                            }
                            else
                            {
                                if(freqidx)
                                    freqidx--;
                            }

                            iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
                            iq_wave_buf[j] = get_next_iq(&iqgen);
                        }
                        write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
                        i+= BUFFER_IQ_SAMPLES_SIZE;
                    }

                    close_wave(wave1);
                }

                free(settle_buf);
            }

            if( (isOption(argc,argv,"help",0,NULL)<=0) &&
                (isOption(argc,argv,"generate",0,NULL)<=0)
                )
            {
                printhelp(argv);
            }

            return 0;
        }
*/
    }
}