/*
 CelebWeather project

 The Original Code is network.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <WiFi.h>
#include <sntp.h>
#include "config.h"
#include "status.h"

namespace CelebWeather
{
    namespace Network
    {
        void printLocalTime(struct timeval *newTime = nullptr)
        {
            struct tm tmp;
            struct tm *timeinfo;

            if(newTime == nullptr)
            {
                if(!getLocalTime(&tmp))
                {
                    Serial.println(F("Failed to obtain time"));
                    return;
                }
                timeinfo = &tmp;
            }
            else
            {
                timeinfo = localtime(&newTime->tv_sec);
            }

            char timeBuffer[25] = "";
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S %Z", timeinfo);

            Serial.printf_P(PSTR("Current time is %s\r\n"), timeBuffer);
        }

        void ntpUpdateCallback(struct timeval *newTime)
        {
            if(Status::TimeAtBoot.tv_sec < 10000 )
            {
                Status::TimeAtBoot.tv_sec = newTime->tv_sec - 10;
                printLocalTime(newTime);
            }
        }

        void reconnectServices()
        {
            // in case NTP forces a time drift we need to recalculate timeAtBoot
            sntp_set_time_sync_notification_cb(ntpUpdateCallback);

            configTzTime("UTC", Config::NTPServerName);
            Status::Connected = true;
        }

        void eventHandler_WiFiStationGotIp(WiFiEvent_t event, WiFiEventInfo_t info)
        {
            Serial.printf_P(PSTR("WiFi Client has received a new IP: %s\r\n"), WiFi.localIP().toString().c_str());
            reconnectServices();
        }

        void eventHandler_WiFiStationLostIp(WiFiEvent_t event, WiFiEventInfo_t info)
        {
            Serial.println("WiFi Client has lost its IP");
            Status::Connected = false;
        }

        void setup()
        {
            Serial.print("WiFi power is currently ");
            Serial.println(WiFi.getTxPower());
            Serial.println("WiFi power set to WIFI_POWER_19_5dBm");
            WiFi.setTxPower(WIFI_POWER_19_5dBm);

            WiFi.onEvent(eventHandler_WiFiStationGotIp, ARDUINO_EVENT_WIFI_STA_GOT_IP);
            WiFi.onEvent(eventHandler_WiFiStationLostIp, ARDUINO_EVENT_WIFI_STA_LOST_IP);
        }
    }
}
