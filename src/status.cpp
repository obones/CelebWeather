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
        static int retrieveForecastMinute = -1;

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

            retrieveForecastMinute = random(1, 59);
            Serial.printf("   retrieveForecastMinute = %d\n", retrieveForecastMinute);

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
            Serial.println("========== Sending time ===========");

            const int maxFrameSize = 100;
            unsigned char frame[maxFrameSize] = {};

            int8_t department = atoi(Config::Department);
            int actualFrameSize = Encoder::EncodeTime(department, frame, maxFrameSize);

            Serial.print("Encoded time sync: ");
            for(int index = 0; index < actualFrameSize; index++)
            {
                Serial.printf("%c", frame[index]);
            }
            Serial.println();

            sendFrame(frame, actualFrameSize);
        }

        void transmitForecast(const unsigned char* frame, int frameSize)
        {
            Serial.println("========== Sending forecast ===========");

            Serial.print("Encoded forecast: ");
            for(int index = 0; index < frameSize; index++)
            {
                Serial.printf("%c", frame[index]);
            }
            Serial.println();

            sendFrame(frame, frameSize);
        }

        void transmitForecast(const openmeteo_sdk::WeatherApiResponse* forecast, int8_t department)
        {
            const int maxFrameSize = 100;
            unsigned char frame[maxFrameSize] = {};

            int actualFrameSize = Encoder::EncodeForecast(forecast, department, frame, maxFrameSize);

            transmitForecast(frame, actualFrameSize);
        }

        using ForecastProcessorCallback = void(*)(const openmeteo_sdk::WeatherApiResponse* forecast);

        void retrieveForecast(ForecastProcessorCallback callback)
        {
            if (Config::OpenMeteoBaseURI[0] != 0)
            {
                Serial.println("========== Retrieving Open-Meteo forecast ===========");

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
                    Serial.println(F("Status::retrieveForecast() -> Failed to obtain time"));
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

                    callback(forecast);

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

        const int maxForecastFrameSize = 100;
        static int actualForecastFrameSize = 0;
        static unsigned char forecastFrame[maxForecastFrameSize] = {};

        void storeEncodedForecast(const openmeteo_sdk::WeatherApiResponse* forecast)
        {
            int8_t department = atoi(Config::Department);
            actualForecastFrameSize = Encoder::EncodeForecast(forecast, department, forecastFrame, maxForecastFrameSize);
        }

        void loopViaMillisRetrieveForecastHourlySendForecastEverySix()
        {
            constexpr int timeCheckPeriodSeconds = 1;
            constexpr int timeAndForecastMinuteOffset = 1;

            static unsigned long previousMillis = 0;
            static int previousRetrieveForecastHour = -1;
            static int previousRetrieveTimeOnlyMinute = -1;

            if (Connected)
            {
                // as forceRefresh is volatile, we must store it locally to avoid a change of value while we work and reset
                // it as fast as possible to allow quick reuse of the functionality.
                bool localForceRefresh = forceRefresh;
                forceRefresh = false;

                if (localForceRefresh)
                    Serial.println("---- Refresh forced ---");

                // retrieve department if needed
                if (Config::Department[0] == 0)
                    retrieveDepartment();

                // check time every timeCheckPeriodSeconds seconds
                if ((millis() - previousMillis > timeCheckPeriodSeconds * 1000) || localForceRefresh)
                {
                    time_t t = time(NULL);
                    struct tm tm;

                    tm = *localtime(&t);

                    // udpdate forecast from open-meteo every hour, at past a random minute
                    // do it also if there was no previously stored forecast but don't do it in case of forced refresh
                    // to avoid overloading the Open Meteo service
                    if ((tm.tm_hour != previousRetrieveForecastHour && tm.tm_min == retrieveForecastMinute) || (actualForecastFrameSize == 0))
                    {
                        previousRetrieveForecastHour = tm.tm_hour;

                        retrieveForecast(storeEncodedForecast);
                    }

                    // send time and predictions every 6 hours, one minute past the hour
                    // or immediately in case of forced refresh
                    if (((tm.tm_hour % 6 == 0) && (tm.tm_min == 1)) || localForceRefresh)
                    {
                        transmitForecast(forecastFrame, actualForecastFrameSize);

                        sendTimeSyncMessage();

                        transmitForecast(forecastFrame, actualForecastFrameSize);
                    }
                    // if not already done just before, send time every 10 minutes at 01, 11, 21, 31...
                    else if ((tm.tm_min % 10 == 1) && (tm.tm_min != previousRetrieveTimeOnlyMinute))
                    {
                        sendTimeSyncMessage();

                        previousRetrieveTimeOnlyMinute = tm.tm_min;
                    }

                    previousMillis = millis();
                }
            }
        }

        typedef struct {
            int count;
            const char* frames[7];
        } ReplayFrame;

        const ReplayFrame replayFrames[] = {
            // 12:11
            {5, {"lK&$5Z  9 $*-G%K-Y+FSK-NY=O7Q?,", "1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 12:16
            {0, {}},
            // 12:21
            {5, {"lL&$5Z  9 $*-G%K-Y+FSK-NY=O7Q?,", "1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 12:26
            {1, {"lL:$5R  9 $*-G%K-Y+FSK-NY=O7Q?,"}},
            // 12:31
            {6, {"lM&$5Z  9 $*-G%K-Y+FSK-NY=O7Q?,", "1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 12:36
            {0, {}},
            // 12:41
            {5, {"lN&$5Z  9 $*-G%K-Y+FSK-NY=O7Q?,", "1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 12:46
            {1, {"lN:$5T  9 $*-G%K-Y+FSK-NY=O7Q?,"}},
            // 12:51
            {5, {"1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 12:56
            {0, {}},
            // 13:01
            {7, {"lZ&$5Z  9 $*-G%K-Y+FSK-NY=O7Q?,", "1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", " S!!<5%'&  \"6J9Ep-6pU'&>9%(\"F0(\"F29E4sf", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 13:06
            {0, {}},
            // 13:11
            {6, {"lk&$5Z  9 $*-G%K-Y+FSK-NY=O7Q?,", "1+!.:%p-& )''39ED-'0T!&49%8\"F0(\"F69E8s&0,s&;", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)", "ZH<2H:HBHRI\"I*I:IZK)"}},
            // 13:16
            {0, {}},
        };
        constexpr int replayFramesLength = sizeof(replayFrames) / sizeof(replayFrames[0]);

        void loopReplay()
        {
            constexpr int timeCheckPeriodSeconds = 1;
            constexpr int timeAndForecastMinuteOffset = 1;

            static unsigned long previousMillis = 0;
            static int previousTransmissionMinute = 0;
            static int replayFrameIndex = 0;

            if (Connected)
            {
                // as forceRefresh is volatile, we must store it locally to avoid a change of value while we work and reset
                // it as fast as possible to allow quick reuse of the functionality.
                bool localForceRefresh = forceRefresh;
                forceRefresh = false;

                if (localForceRefresh)
                {
                    replayFrameIndex = 0;
                    Serial.println("---- Refresh forced ---");
                }

                // check time every timeCheckPeriodSeconds seconds
                if (millis() - previousMillis > timeCheckPeriodSeconds * 1000)
                {
                    time_t t = time(NULL);
                    struct tm tm;

                    tm = *localtime(&t);

                    // send the replayed frame over the air every 5 minutes at 01, 06, 11, 16...
                    if ((tm.tm_min != previousTransmissionMinute) && (tm.tm_min % 5 == timeAndForecastMinuteOffset))
                    {
                        previousTransmissionMinute = tm.tm_min;
                        Serial.printf("===== Replaying %d ====== \n", replayFrameIndex);

                        const ReplayFrame* replayFrame = &replayFrames[replayFrameIndex];

                        for (int frameIndex = 0; frameIndex < replayFrame->count; frameIndex++)
                        {
                            const char* replayFrameContent = replayFrame->frames[frameIndex];

                            Serial.printf("Encoded message: %s\n", replayFrameContent);

                            sendFrame((const unsigned char*)replayFrameContent, strlen(replayFrameContent));
                        }

                        replayFrameIndex++;
                        if (replayFrameIndex >= replayFramesLength)
                            replayFrameIndex = 0;
                    }
                }
            }
        }

        void loop()
        {
            //loopReplay();
            loopViaMillisRetrieveForecastHourlySendForecastEverySix();
        }
    }
}