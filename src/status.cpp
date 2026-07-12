/*
 CelebWeather project

 The Original Code is status.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <Arduino.h>
/*#include <esp32HTTPrequest.h>
#include <ArduinoJson.h>
#include <base64.h>*/

#include "status.h"
#include "config.h"

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

/*        typedef void (*responseTextCallback)(String&);

        void requestReadyStateChange(void* optParm, esp32HTTPrequest* request, int readyState)
        {
            if (readyState == 4)
            {
                request->setDebug(false);

                String responseText = request->responseText();

                if (request->responseHTTPcode() == 200)
                {
                    if (optParm)
                    {
                        auto callback = (responseTextCallback)optParm;
                        callback(responseText);
                    }
                }
                else
                {
                    Serial.printf("Request failed: %d\r\n", request->responseHTTPcode());
                    Serial.println(responseText);
                }
            }
        }

        void prepareRequest(esp32HTTPrequest& request, const char* method, responseTextCallback callback)
        {
            String url = "http://";
            url += Config::OpenHabServerName;
            url += "/rest/items/";
            url += Config::AlarmItemName;

            request.setDebug(false);
            request.onReadyStateChange(requestReadyStateChange, (void*)callback);
            request.open(method, url.c_str());
            request.setReqHeader("accept", "application/json");
            request.setReqHeader("Content-Type", "text/plain");
            if (Config::OpenHabAPIKey[0] != 0)
            {
                String userId = Config::OpenHabAPIKey;
                userId += ":";
                userId = base64::encode(userId);
                userId = "Basic " + userId;
                request.setReqHeader("Authorization", userId.c_str());
            }
        }

        void itemRetrievalCallback(String& responseText)
        {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, responseText);
            if (error)
            {
                Serial.print(F("Failed to read response:"));
                Serial.println(error.f_str());
                Serial.println(responseText);
            }
            else
            {
                auto root = doc.as<JsonObject>();
                AlarmState = root["state"].as<String>();
                Serial.print("State is ");
                Serial.println(AlarmState);

                if (AlarmState == "ON")
                {
                    digitalWrite(ALARM_ON_LED_PIN, LED_ON);
                    digitalWrite(ALARM_OFF_LED_PIN, LED_OFF);
                }
                else if (AlarmState == "OFF")
                {
                    digitalWrite(ALARM_ON_LED_PIN, LED_OFF);
                    digitalWrite(ALARM_OFF_LED_PIN, LED_ON);
                }
                else
                {
                    digitalWrite(ALARM_ON_LED_PIN, LED_OFF);
                    digitalWrite(ALARM_OFF_LED_PIN, LED_OFF);
                }
            }
        }

        void setAlarmStateCallback(String& responseText)
        {
            MustRefreshAlarmState = true;
        }

        void setAlarmState(const char* state)
        {
            esp32HTTPrequest request;
            prepareRequest(request, "POST", setAlarmStateCallback);
            request.send(state);
        }*/

        void loop()
        {
            static unsigned long previousMillis = 0;
            if (Connected && ((millis() - previousMillis > Config::RefreshPeriod * 1000)))
            {
                previousMillis = millis();

                if (Config::OpenMeteoBaseURI[0] != 0)
                {
                    Serial.println("Retrieving Open-Meteo forecast");

                    /*esp32HTTPrequest request;
                    prepareRequest(request, "GET", itemRetrievalCallback);
                    request.send();*/
                }
            }
        }
    }
}