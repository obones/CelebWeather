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

namespace CelebWeather
{
    namespace Status
    {
        struct timeval TimeAtBoot;
        bool Connected = false;

        void setup()
        {
            if (gettimeofday(&TimeAtBoot, NULL) != 0)
            {
                Serial.println(F("Failed to obtain time"));
            }
        }

        void loop()
        {
            static unsigned long previousMillis = 0;
            static bool forceRefresh = true;
            if (Connected && ((millis() - previousMillis > Config::RefreshPeriodSeconds * 1000) || forceRefresh))
            {
                previousMillis = millis();

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

                    String uri =
                        String(Config::OpenMeteoBaseURI) +
                        "forecast" +
                        "?latitude=" + Config::Latitude +
                        "&longitude=" + Config::Longitude +
                        "&temperature_unit=celsius&wind_speed_unit=ms&precipitation_unit=mm" +
                        "&timezone=" + uriTimeZone +
                        "&forecast_hours=144&hourly=temperature_2m,cloud_cover,snowfall,precipitation_probability,rain,weather_code" +
                        "&temporal_resolution=hourly_6&format=flatbuffers";

                    Serial.printf("Uri: %s\n", uri.c_str());

                    http.begin(uri);
                    int httpCode = http.GET();

                    if (httpCode == HTTP_CODE_OK)
                    {
                        int bufferSize = http.getSize();
                        if (bufferSize < 0)
                            bufferSize = 1 * 1024;

                        uint8_t* buffer = reinterpret_cast<uint8_t*>(malloc(bufferSize));

                        MemoryStream stream(buffer, bufferSize);

                        http.writeToStream(&stream);

                        Serial.printf("Received %d bytes\n", stream.getPosition());

                        // this does a simple mapping to the buffer, no memory copy occurs
                        auto apiResponse = openmeteo_sdk::GetWeatherApiResponse(buffer);
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
        }
    }
}