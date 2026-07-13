// Largely inspired by https://github.com/jfdelnero/rf-tools/blob/master/src/pocsag/
//
// Part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2026 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
//     Adapted by Olivier Sannier for the CelebWeather project
///////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

#include "encoder.h"
#include "config.h"

#define MAX_MSG_SIZE 512

namespace CelebWeather
{
    namespace Encoder
    {
        typedef struct frame_
        {
            unsigned char frame[MAX_MSG_SIZE];
            unsigned char decodeFrame[MAX_MSG_SIZE];
            unsigned char quartetFrame[MAX_MSG_SIZE*3];
            int quartets_cnt;
        } frame;

        // Convert 6 bits word to char. (encoding)
        unsigned char raw2char(unsigned char r)
        {
            if( r >= 59 && r <= 63 )
                return 'k' + (r - 59);

            switch( r )
            {
                case 0x20:
                    return 'p';
                break;
                case 0x03:
                    return 's';
                break;
            }

            r = r + 0x20;

            return r;
        }

        // Set quartet to an encoded 6 bits words array
        void set_quartet( unsigned char * buf, int idx, unsigned char q)
        {
            int i,j,bitidx;

            bitidx = (idx * 4);
            j = 0;
            while(j<4)
            {
                i = bitidx / 6;

                if( q & (0x8>>j) )
                {
                    buf[i] = buf[i] | ( 0x01 << (5-(bitidx%6)));
                }
                else
                {
                    buf[i] = buf[i] & ~( 0x01 << (5-(bitidx%6)));
                }

                bitidx++;
                j++;
            }

            return;
        }

        // Encode temperature to BCD+40
        unsigned char encodeTemperature_to_bcd(int temp)
        {
            temp += 40;
            return (((temp / 10)&0xF) << 4) | ((temp%10) & 0xF);
        }

        // Generate and encode current time frame.
        int gen_current_time(unsigned char * quartets)
        {
            int i;
            time_t t = time(NULL);
            struct tm tm;

            tm = *localtime(&t);

            quartets[0] = 0xF;

            if( tm.tm_hour < 10)
            {
                quartets[1] = tm.tm_hour;
                quartets[2] = tm.tm_min/10;
                quartets[3] = tm.tm_min%10;
            }
            else
            {
                if( tm.tm_hour >= 10 &&  tm.tm_hour <= 19  )
                {
                    quartets[1] = tm.tm_hour - 10;
                    quartets[2] = (tm.tm_min/10) + 10;
                    quartets[3] = tm.tm_min%10;
                }
                else
                {
                    //20 <> 23
                    quartets[1] = tm.tm_hour - 10;
                    quartets[2] = (tm.tm_min/10);
                    quartets[3] = tm.tm_min%10;
                }
            }

            quartets[4] =  tm.tm_mon + 1;
            quartets[5] =  ( ((tm.tm_mday/10)&3)<<2 ) | ( (((tm.tm_mday%10)>>2)&3) );
            quartets[6] =  ( ((tm.tm_mday%10)&3) )<<2;
            quartets[6] |= ( ((tm.tm_year-100)>>4) & 0x3);
            quartets[7] =  ( ((tm.tm_year-100)) & 0xF);

            int sum = 0x7;
            i = 0;
            while(i<8)
            {
                sum += quartets[i];
                i++;
            }
            quartets[8] = sum & 0xF;

            return 9;
        }

        int getPictogramIndexFromWMOCode(int WMOCode, bool isDay)
        {
            switch (WMOCode)
            {
                case 0:   // clear sky
                    return (isDay) ? 0x00 : 0x18;
                case 1:   // mainly clear
                    return (isDay) ? 0x01 : 0x19;
                case 2:   // partly clear
                    return (isDay) ? 0x02 : 0x19;
                case 3:   // overcast
                    return (isDay) ? 0x03 : 0x19;
                case 45:  // fog
                    return (isDay) ? 0x03 : 0x19;
                case 48:  // depositing rime fog
                    return (isDay) ? 0x10 : 0x20;
                case 51:  // Light drizzle
                    return (isDay) ? 0x07 : 0x1A;
                case 53:  // Moderate drizzle
                    return (isDay) ? 0x08 : 0x1B;
                case 55:  // Dense drizzle
                    return (isDay) ? 0x09 : 0x1C;
                case 56:  // Light freezing drizzle
                    return (isDay) ? 0x07 : 0x1A;
                case 57:  // Dense freezing drizzle
                    return (isDay) ? 0x08 : 0x1B;
                case 61:  // Slight rain
                    return (isDay) ? 0x04 : 0x1A;
                case 63:  // Moderate rain
                    return (isDay) ? 0x05 : 0x1B;
                case 65:  // Heavy rain
                    return (isDay) ? 0x06 : 0x1C;
                case 66:  // Light freezing rain
                    return (isDay) ? 0x04 : 0x1A;
                case 67:  // Heavy freezing rain
                    return (isDay) ? 0x06 : 0x1C;
                case 71:  // Slight snow fall
                    return (isDay) ? 0x10 : 0x20;
                case 73:  // Moderate snow fall
                    return (isDay) ? 0x10 : 0x20;
                case 75:  // Heavy snow fall
                    return (isDay) ? 0x11 : 0x21;
                case 77:  // Snow grains
                    return (isDay) ? 0x10 : 0x20;
                case 80:  // Slight rain showers
                    return (isDay) ? 0x07 : 0x1A;
                case 81:  // Moderate rain showers
                    return (isDay) ? 0x08 : 0x1B;
                case 82:  // Heavy rain showers
                    return (isDay) ? 0x09 : 0x1C;
                case 85:  // Slight snow showers
                    return (isDay) ? 0x12 : 0x20;
                case 86:  // Heavy snow showers
                    return (isDay) ? 0x13 : 0x21;
                case 95:  // Slight or moderate thunderstorm
                    return (isDay) ? 0x0A : 0x1D;
                case 96:  // Slight hail thunderstorm
                    return (isDay) ? 0x0B : 0x1E;
                case 99:  // Heavy hail thunderstorm
                    return (isDay) ? 0x0C : 0x1F;
                case 255:
                    return (isDay) ? 0x17 : 0x23;
                default:
                    return (isDay) ? 0x16 : 0x22;
            }
        }

        int gen_forecast(unsigned char * quartets, const openmeteo_sdk::WeatherApiResponse *forecast)
        {
            // format for one day
            // ltemp_0,htemp_0,pic_0,pic_1,pic_2,pic_3,pic_4
            //
            // pictures meaning:
            //   pic_0: whole day picture
            //   pic_1: night picture      ( 00:00 -> 05:59 )
            //   pic_2: morning picture    ( 06:00 -> 11:59 )
            //   pic_3: afternoon picture  ( 12:00 -> 17:59 )
            //   pic_4: evening picture    ( 18:00 -> 23:59 )

            const openmeteo_sdk::VariableWithValues *minTempVariable = nullptr;
            const openmeteo_sdk::VariableWithValues *maxTempVariable = nullptr;
            const openmeteo_sdk::VariableWithValues *wmoCodeVariable = nullptr;

            size_t variablesLength = 0;

            for (int variableIndex = 0; variableIndex < forecast->hourly()->variables()->Length(); variableIndex++)
            {
                auto variable = forecast->hourly()->variables()->Get(variableIndex);

                Serial.printf("Variable %d : %s - %s\n", variableIndex, openmeteo_sdk::EnumNameVariable(variable->variable()), openmeteo_sdk::EnumNameAggregation(variable->aggregation()));

                variablesLength = max(variablesLength, variable->values()->Length());

                switch(variable->variable())
                {
                    case openmeteo_sdk::Variable::temperature:
                        // This should work but for some reason, min and max are returned as none and efi...
                        // Reported at https://github.com/open-meteo/open-meteo/issues/1985
                        /*switch(variable->aggregation())
                        {
                            case openmeteo_sdk::Aggregation::minimum:
                                minTempVariable = variable;
                                break;
                            case openmeteo_sdk::Aggregation::maximum:
                                maxTempVariable = variable;
                                break;
                        }*/
                        if (minTempVariable == nullptr)
                            minTempVariable = variable;
                        else if (maxTempVariable == nullptr)
                            maxTempVariable = variable;
                        break;
                    case openmeteo_sdk::Variable::weather_code:
                        wmoCodeVariable = variable;
                        break;
                }
            }

            if (minTempVariable == nullptr)
            {
                Serial.println("MinTemp variable not found!");
                return 0;
            }

            if (maxTempVariable == nullptr)
            {
                Serial.println("MaxTemp variable not found!");
                return 0;
            }

            if (wmoCodeVariable == nullptr)
            {
                Serial.println("WMOCode variable not found!");
                return 0;
            }

            const int periodsPerDay = 4;
            const int quartetsPerDay = 15;

            int dayCount = variablesLength / periodsPerDay;

            Serial.printf("variablesLength = %d\n", variablesLength);
            Serial.printf("dayCount = %d\n", dayCount);

            for (int dayIndex = 0; dayIndex < dayCount; dayIndex++)
            {
                int lowestTemperature = 90;
                int highestTemperature = -90;

                int periodPictograms[periodsPerDay + 1] = {};

                for (int periodIndex = 0; periodIndex < periodsPerDay; periodIndex++)
                {
                    int valueIndex = dayIndex * periodsPerDay + periodIndex;

                    int minTemperature = floor(minTempVariable->values()->Get(valueIndex));
                    int maxTemperature = ceil(maxTempVariable->values()->Get(valueIndex));

                    int periodWMOCode = trunc(wmoCodeVariable->values()->Get(valueIndex));

                    lowestTemperature = min(lowestTemperature, minTemperature);
                    highestTemperature = max(highestTemperature, maxTemperature);
                    periodPictograms[0] = max(periodPictograms[0], periodWMOCode);
                    periodPictograms[periodIndex + 1] = getPictogramIndexFromWMOCode(periodWMOCode, periodIndex != 0);
                }
                periodPictograms[0] = getPictogramIndexFromWMOCode(periodPictograms[0], true);

                int quartetsOffset = dayIndex * quartetsPerDay;

                // Low temp
                uint8_t b = encodeTemperature_to_bcd(lowestTemperature);
                quartets[quartetsOffset + 0] = (b>>4);
                quartets[quartetsOffset + 1] = (b&0xF);

                // High temp
                b = encodeTemperature_to_bcd(highestTemperature);
                quartets[quartetsOffset + 2] = (b>>4);
                quartets[quartetsOffset + 3] = (b&0xF);

                // Pictos
                for(int j = 0; j < 5; j++)
                {
                    quartets[quartetsOffset + 4+(j*2)]     = (periodPictograms[j] >> 4);
                    quartets[quartetsOffset + 4+(j*2) + 1] = (periodPictograms[j] & 0xF);
                }

                // Checksum
                quartets[quartetsOffset + 14] = 7;
                for(int i = 0; i < quartetsPerDay - 1; i++)
                {
                    quartets[quartetsOffset + 14] += quartets[i];
                }
            }

            return quartetsPerDay * dayCount;
        }

        /*int main(int argc, char* argv[])
        {
            int i,idx,fidx;
            int quiet,verbose;
            int param_start_index;
            frame * genfrm;
            int sum;
            char tmp_str[512];
            int forcast_cnt;

            verbose = 0;
            if(isOption(argc, argv,"verbose",NULL, NULL) )
            {
                verbose = 1;
            }

            quiet = 0;
            if(isOption(argc, argv,"quiet",NULL, NULL) )
            {
                quiet = 1;
            }

            if(!quiet)
                printf("startmeteo v0.1 -help format command line syntax.\n");

            if(isOption(argc, argv,"help",NULL, NULL) )
            {
                printf("Syntax:\n");
                printf("%s -decode [files]\n",argv[0]);
                printf("%s -encode:[HEX Quartets]\n",argv[0]);
                printf("%s -checksum     (Update checksum with \"-encode\")\n",argv[0]);
                printf("%s -curtime       Generate current date/hour frame\n",argv[0]);
                printf("%s -forecast:[LowTemp],[HighTemp],[MainPicto],[Picto_2],[Picto_3],[Picto_4],[Picto_5]\n",argv[0]);
                printf("%s -areaid:[idcode]\n",argv[0]);

                printf("%s -quiet\n",argv[0]);
                printf("%s -verbose\n",argv[0]);
                printf("\n");
                printf("Example: %s -decode ../previsions_ok/*.txt\n",argv[0]);
                printf("Example: %s -curtime -quiet\n",argv[0]);
                printf("Example: %s -forecast:-10,40,1,2,3,4,5 -forecast:-11,41,6,7,8,9,10 -forecast:-12,42,11,12,13,14,15 -forecast:-13,43,16,17,18,19,20 -areaid:75 -quiet\n",argv[0]);
                exit(0);
            }

            param_start_index = 1;
            if(isOption(argc, argv,"curtime",(char*)tmp_str, &param_start_index) )
            {
                genfrm = calloc(sizeof(frame)*1,1);
                if(!genfrm)
                    exit(-1);

                genfrm->quartets_cnt = gen_current_time(genfrm->quartetfrm);

                i = 0;
                while( i < genfrm->quartets_cnt )
                {
                    set_quartet( (unsigned char*)(genfrm->dcodefrm), i, genfrm->quartetfrm[i]);
                    i++;
                }

                int size = (genfrm->quartets_cnt*4)/6;
                i = 0;
                while( i < size )
                {
                    genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
                    printf("%c",genfrm->frm[i]);
                    i++;
                }

                if(!quiet)
                    printf("\n");

                free(genfrm);
            }

            forcast_cnt = 0;
            param_start_index = 0;
            while( isOption(argc, argv,"forecast",(char*)tmp_str, &param_start_index) )
            {
                forcast_cnt++;
                param_start_index++;
            }

            if(forcast_cnt)
            {
                int departement;

                departement = 75;

                if(isOption(argc, argv,"areaid",(char*)tmp_str, NULL) )
                {
                    departement = atoi(tmp_str);
                }

                genfrm = calloc(sizeof(frame)*1,1);
                if(!genfrm)
                    exit(-1);

                genfrm->quartetfrm[0] = 0x4;
                genfrm->quartetfrm[1] = (departement>>4) & 0xF;
                genfrm->quartetfrm[2] = (departement   ) & 0xF;
                genfrm->quartetfrm[3] = 0x0;
                genfrm->quartetfrm[4] = 0x4;

                genfrm->quartetfrm[5] = 0x7;
                for(i=0;i<5;i++)
                {
                    genfrm->quartetfrm[5] += genfrm->quartetfrm[i];
                }

                genfrm->quartets_cnt = 6;

                i = 0;
                while( i < genfrm->quartets_cnt )
                {
                    set_quartet( (unsigned char*)(genfrm->dcodefrm), i, genfrm->quartetfrm[i]);
                    i++;
                }

                int size = (genfrm->quartets_cnt*4)/6;
                i = 0;
                while( i < size )
                {
                    genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
                    printf("%c",genfrm->frm[i]);
                    i++;
                }

                if(!quiet)
                    printf("\n");

                free(genfrm);
            }

            param_start_index = 0;
            while( isOption(argc, argv,"forecast",(char*)tmp_str, &param_start_index) )
            {
                genfrm = calloc(sizeof(frame)*1,1);
                if(!genfrm)
                    exit(-1);

                genfrm->quartets_cnt = gen_forecast(genfrm->quartetfrm,tmp_str);

                i = 0;
                while( i < genfrm->quartets_cnt )
                {
                    set_quartet( (unsigned char*)(genfrm->dcodefrm), i, genfrm->quartetfrm[i]);
                    i++;
                }

                int size = (genfrm->quartets_cnt*4)/6;
                i = 0;
                while( i < size )
                {
                    genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
                    printf("%c",genfrm->frm[i]);
                    i++;
                }

                if(!quiet)
                    printf("\n");

                free(genfrm);

                param_start_index++;
            }
        }*/

        void Encode(const openmeteo_sdk::WeatherApiResponse *forecast, unsigned char* destFrame, size_t destFrameSize)
        {
            int destFrameIndex = 0;

            frame* genfrm = reinterpret_cast<frame*>(calloc(sizeof(frame)*1, 1));
            if(!genfrm)
                return;

            // department
            {
                int8_t departement = atoi(Config::Department);

                genfrm->quartetFrame[0] = 0x4;
                genfrm->quartetFrame[1] = (departement>>4) & 0xF;
                genfrm->quartetFrame[2] = (departement   ) & 0xF;
                genfrm->quartetFrame[3] = 0x0;
                genfrm->quartetFrame[4] = 0x4;

                genfrm->quartetFrame[5] = 0x7;
                for(int i = 0; i < 5; i++)
                {
                    genfrm->quartetFrame[5] += genfrm->quartetFrame[i];
                }

                genfrm->quartets_cnt = 6;

                int i = 0;
                while( i < genfrm->quartets_cnt )
                {
                    set_quartet( (unsigned char*)(genfrm->decodeFrame), i, genfrm->quartetFrame[i]);
                    i++;
                }

                int size = (genfrm->quartets_cnt*4)/6;
                i = 0;

                Serial.printf("Encoding department: size = %d, destFrameIndex = %d, destFrameSize = %d\n", size, destFrameIndex, destFrameSize);
                while( i < size && destFrameIndex < destFrameSize )
                {
                    genfrm->frame[i] = raw2char(genfrm->decodeFrame[i]);
                    destFrame[destFrameIndex] = genfrm->frame[i];
                    // printf("%c",genfrm->frm[i]);
                    i++;
                    destFrameIndex++;
                }
            }

            // forecast
            {
                genfrm->quartets_cnt = gen_forecast(genfrm->quartetFrame, forecast);

                int i = 0;
                while( i < genfrm->quartets_cnt )
                {
                    set_quartet( (unsigned char*)(genfrm->decodeFrame), i, genfrm->quartetFrame[i]);
                    i++;
                }

                int size = (genfrm->quartets_cnt*4)/6;
                i = 0;
                Serial.printf("Encoding forecast: size = %d, destFrameIndex = %d, destFrameSize = %d\n", size, destFrameIndex, destFrameSize);
                while( i < size && destFrameIndex < destFrameSize )
                {
                    genfrm->frame[i] = raw2char(genfrm->decodeFrame[i]);
                    destFrame[destFrameIndex] = genfrm->frame[i];
                    // printf("%c",genfrm->frm[i]);
                    i++;
                    destFrameIndex++;
                }
            }

            free(genfrm);
        }

    }
}