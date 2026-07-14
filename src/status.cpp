/*
 CelebWeather project

 The Original Code is status.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MemoryStream.h>

#include "status.h"
#include "config.h"
#include "weather_api_generated.h"
#include "encoder.h"
#include "pocsag.h"

namespace CelebWeather
{
    namespace Status
    {
        struct timeval TimeAtBoot;
        bool Connected = false;
        static bool forceRefresh = true;

        void setForceRefresh(bool value)
        {
            forceRefresh = value;
        }

        void setup()
        {
            if (gettimeofday(&TimeAtBoot, NULL) != 0)
            {
                Serial.println(F("Failed to obtain time"));
            }
        }

        void sendTimeSyncMessage()
        {
            const int destFrameSize = 100;
            unsigned char destFrame[destFrameSize] = {};

            Encoder::EncodeTime(destFrame, destFrameSize);

            Serial.print("Encoded time sync: ");
            for(int index = 0; index < destFrameSize; index++)
            {
                Serial.printf("%c", destFrame[index]);
            }
            Serial.println();

            Pocsag::GetBits(destFrame, destFrameSize);
        }

        void sendForecastMessage()
        {
            if (Config::OpenMeteoBaseURI[0] != 0)
            {
                forceRefresh = false;
                Serial.println("Retrieving Open-Meteo forecast");

                WiFiClient wifiClient;   // wifi client object
                wifiClient.stop(); // close connection before sending a new request
                HTTPClient http;
                http.setTimeout(15000);
                http.setConnectTimeout(15000);

                String uriTimeZone = Config::Timezone;
                uriTimeZone.replace("/", "%2F");

                // we get values starting from today's 03:00 for 6 days (including the current one)
                struct tm timeinfo;
                if(!getLocalTime(&timeinfo))
                {
                    Serial.println(F("Failed to obtain time"));
                    return;
                }
                timeinfo.tm_hour = 3;
                timeinfo.tm_min = 0;
                timeinfo.tm_sec = 0;
                char timeBuffer[25] = "";
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%dT%H%%3A%M%%3A%S", &timeinfo);

                String startDate = timeBuffer;

                timeinfo.tm_mday += 5;  // strftime does not handle month overrun, so we must use mktime/localtime
                timeinfo.tm_hour = 23;
                timeinfo.tm_min = 59;
                timeinfo.tm_sec = 59;
                time_t endDateTime = mktime(&timeinfo);
                struct tm *endDateTm = localtime(&endDateTime);
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%dT%H%%3A%M%%3A%S", endDateTm);

                String endDate = timeBuffer;

                String uri =
                    String(Config::OpenMeteoBaseURI) +
                    "forecast" +
                    "?latitude=" + Config::Latitude +
                    "&longitude=" + Config::Longitude +
                    "&temperature_unit=celsius&wind_speed_unit=ms&precipitation_unit=mm" +
                    "&timezone=" + uriTimeZone +
                    "&start_hour=" + startDate +
                    "&end_hour=" + endDate +
                    "&hourly=temperature_2m_min,temperature_2m_max,cloud_cover,snowfall,precipitation_probability,rain,weather_code" +
                    "&temporal_resolution=hourly_6&format=flatbuffers";

                Serial.printf("Uri: %s\n", uri.c_str());

                http.begin(uri);
                int httpCode = http.GET();

                if (httpCode == HTTP_CODE_OK)
                {
                    int bufferSize = http.getSize();
                    if (bufferSize < 0)
                        bufferSize = 1.2 * 1024;

                    uint8_t* buffer = reinterpret_cast<uint8_t*>(malloc(bufferSize));

                    MemoryStream stream(buffer, bufferSize);

                    http.writeToStream(&stream);

                    Serial.printf("Received %d bytes\n", stream.getPosition());

                    // this does a simple mapping to the buffer, no memory copy occurs
                    auto forecast = openmeteo_sdk::GetSizePrefixedWeatherApiResponse(buffer);

                    const int destFrameSize = 100;
                    unsigned char destFrame[destFrameSize] = {};

                    Encoder::EncodeForecast(forecast, destFrame, destFrameSize);

                    Serial.print("Encoded forecast: ");
                    for(int index = 0; index < destFrameSize; index++)
                    {
                        Serial.printf("%c", destFrame[index]);
                    }
                    Serial.println();

                    Pocsag::GetBits(destFrame, destFrameSize);

                    free(buffer);
                }
                else
                {
                    String errorString = http.errorToString(httpCode);
                    Serial.printf("connection failed, error %d: %s\n", httpCode, errorString.c_str());
                    Serial.println(http.getString());
                }
                wifiClient.stop();
                http.end();
            }
        }

        void loop()
        {
            static unsigned long previousForecastMillis = 0;
            static unsigned long previousTimeSyncMillis = 0;

            if (Connected)
            {
                // send time sync if time interval has elapsed
                if (((millis() - previousTimeSyncMillis > Config::RefreshPeriodSeconds * 1000) || forceRefresh))
                {
                    previousTimeSyncMillis = millis();

                    sendTimeSyncMessage();
                }

                // forecast messages are sent twice less than time sync
                if (((millis() - previousForecastMillis > Config::RefreshPeriodSeconds * 2 * 1000) || forceRefresh))
                {
                    previousForecastMillis = millis();

                    sendForecastMessage();
                }
            }
        }
    }
}