/*
 CelebWeather project

 The Original Code is network.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <WiFi.h>
#include <esp_sntp.h>
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

            // reset the current time to force complete time read
            struct tm timeInfo;
            struct timeval val;

            timeInfo.tm_hour = 0;
            timeInfo.tm_min = 0;
            timeInfo.tm_sec = 0;
            timeInfo.tm_year = 1970;
            timeInfo.tm_mon = 0;
            timeInfo.tm_mday = 1;
            val.tv_sec = mktime(&timeInfo);
            val.tv_usec = 0;
            settimeofday(&val, NULL);

            const char* ntpTimezone = "UTC0";
            int gmtOffset_sec       = 0;
            int daylightOffset_sec  = 0;

            // Mapping can be done with https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
            // The file is 15KB, it would be a waste of memory to embed it when, realistically, this project will mostly be used in France.
            if (strcmp(Config::Timezone, "Europe/Paris") == 0)
            {
                ntpTimezone        = "CET-1CEST,M3.5.0,M10.5.0/3";
                gmtOffset_sec      = 3600;  // France normal time is GMT + 1, so GMT Offset is 3600, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
                daylightOffset_sec = 3600;  // In France DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset
            }

            // start NTP sync
            configTime(gmtOffset_sec, daylightOffset_sec, Config::NTPServerName, "0.fr.pool.ntp.org"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)

            setenv("TZ", ntpTimezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
            tzset(); // Set the TZ environment variable

            while (!getLocalTime(&timeInfo, 15000)) // Wait for 15-sec for time to synchronize
            {
                Serial.println("Failed to obtain time");
                Status::Connected = false;
                return;
            }

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
