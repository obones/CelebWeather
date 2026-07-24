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
#include <ArduinoJson.h>

#include "status.h"
#include "config.h"
#include "weather_api_generated.h"
#include "encoder.h"
#include "pocsag.h"
#include "radio.h"

namespace CelebWeather
{
    namespace Status
    {
        #define FORCE_REFRESH_PIN 21

        struct timeval TimeAtBoot;
        bool Connected = false;
        static volatile bool forceRefresh = true;

        void setForceRefresh(bool value)
        {
            forceRefresh = value;
        }

        static volatile unsigned long previousForceRefreshISRMillis = 0;
        void IRAM_ATTR forceRefreshPinISR()
        {
            unsigned long currentMillis = millis();

            // debounce
            if (currentMillis - previousForceRefreshISRMillis > 500)
            {
                if (digitalRead(FORCE_REFRESH_PIN) == 0)
                    setForceRefresh(true);
            }

            previousForceRefreshISRMillis = currentMillis;
        }

        void setup()
        {
            Serial.println("===> Setting up status");
            if (gettimeofday(&TimeAtBoot, NULL) != 0)
            {
                Serial.println(F("Status::setup() -> Failed to obtain time"));
            }

            pinMode(FORCE_REFRESH_PIN, INPUT_PULLUP);
            ::attachInterrupt(digitalPinToInterrupt(FORCE_REFRESH_PIN), &forceRefreshPinISR, CHANGE);
            Serial.println("---> done");
        }

        void retrieveDepartment()
        {
            String department = "75";

            Serial.println("Retrieving department");
            WiFiClient wifiClient;   // wifi client object
            wifiClient.stop(); // close connection before sending a new request
            HTTPClient http;
            http.setTimeout(15000);
            http.setConnectTimeout(15000);

            String uri =
                String("https://geo.api.gouv.fr/communes") +
                "?lat=" + Config::Latitude +
                "&lon=" + Config::Longitude +
                "&fields=codeDepartement&format=json";

            Serial.printf("Department Uri: %s\n", uri.c_str());

            http.begin(uri);
            int httpCode = http.GET();

            if (httpCode == HTTP_CODE_OK)
            {
                String reply = http.getString();
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, reply);
                if (error)
                {
                    Serial.print("deserializeJson() returned ");
                    Serial.println(error.c_str());
                }
                else
                {
                    auto rootArray = doc.as<JsonArray>();
                    if (!rootArray.isNull() && (rootArray.size() > 0))
                    {
                        auto firstItem = rootArray[0].as<JsonObject>();
                        if (firstItem.isNull())
                        {
                            Serial.println("First item is not an object");
                        }
                        else
                        {
                            const char* value = firstItem["codeDepartement"];
                            department = value;
                        }

                    }
                    else
                    {
                        Serial.println("Root JSON has no members");
                    }
                }

            }
            else
            {
                String errorString = http.errorToString(httpCode);
                Serial.printf("connection failed, error %d: %s\n", httpCode, errorString.c_str());
                Serial.println(http.getString());
            }

            Serial.printf("Department found: %s\n", department.c_str());
            strcpy(Config::Department, department.c_str());
        }

        void sendFrame(const unsigned char* frame, int frameSize)
        {
            #define MAX_BYTES 400

            uint8_t* bytes = new uint8_t[MAX_BYTES];

            int actualBytesCount = Pocsag::GetBytes(frame, frameSize, bytes, MAX_BYTES);

            Serial.printf("Sending %d bytes\n", actualBytesCount);
            bool transmitResult = Radio::transmit(bytes, actualBytesCount);
            if (transmitResult)
                Serial.println("  -> Done");
            else
                Serial.println("  -> Failed transmission");
        }

        void sendTimeSyncMessage()
        {
            const int maxFrameSize = 100;
            unsigned char frame[maxFrameSize] = {};

            int actualFrameSize = Encoder::EncodeTime(frame, maxFrameSize);

            Serial.print("Encoded time sync: ");
            for(int index = 0; index < actualFrameSize; index++)
            {
                Serial.printf("%c", frame[index]);
            }
            Serial.println();

            sendFrame(frame, actualFrameSize);
        }

        void transmitForecast(const openmeteo_sdk::WeatherApiResponse* forecast, int8_t department)
        {
            const int maxFrameSize = 100;
            unsigned char frame[maxFrameSize] = {};

            int actualFrameSize = Encoder::EncodeForecast(forecast, department, frame, maxFrameSize);

            Serial.print("Encoded forecast: ");
            for(int index = 0; index < actualFrameSize; index++)
            {
                Serial.printf("%c", frame[index]);
            }
            Serial.println();

            sendFrame(frame, actualFrameSize);
        }

        void sendForecastMessage()
        {
            if (Config::OpenMeteoBaseURI[0] != 0)
            {
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
                    Serial.println(F("Status::sendForecastMessage() -> Failed to obtain time"));
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

                    // Send first with 75 (and maybe bogus forecast), then retry with actual department if different
                    transmitForecast(forecast, 75);
                    int8_t department = atoi(Config::Department);
                    if (department != 75)
                        transmitForecast(forecast, department);

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
                // retrieve department if needed
                if (Config::Department[0] == 0)
                    retrieveDepartment();

                // as forceRefresh is volatile, we must store it locally to avoid a change of value while we work and reset
                // it as fast as possible to allow quick reuse of the functionality.
                bool localForceRefresh = forceRefresh;
                forceRefresh = false;

                if (localForceRefresh)
                    Serial.println("---- Refresh forced ---");

                // send time sync if time interval has elapsed
                if (((millis() - previousTimeSyncMillis > Config::RefreshPeriodSeconds * 1000) || localForceRefresh))
                {
                    previousTimeSyncMillis = millis();

                    sendTimeSyncMessage();
                }

                // forecast messages are sent twice less than time sync
                if (((millis() - previousForecastMillis > Config::RefreshPeriodSeconds * 2 * 1000) || localForceRefresh))
                {
                    previousForecastMillis = millis();

                    sendForecastMessage();
                }
            }
        }
    }
}