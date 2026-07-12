/*
 CelebWeather project

 The Original Code is portal.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <IotWebConf.h>
#include "config.h"
#include "network.h"

namespace CelebWeather
{
    namespace Portal
    {
        // -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
        const char thingName[] = "CelebWeather";

        // -- Initial password to connect to the Thing, when it creates an own Access Point.
        const char wifiInitialApPassword[] = "ESP32CelebWeather";

        // -- Method declarations.
        void handleRoot();
        void configSaved();

        DNSServer dnsServer;
        WebServer server(80);

        IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

        auto ntpServerName = iotwebconf::TextParameter("NTP server", "ntpServer", Config::NTPServerName, sizeof(Config::NTPServerName));

        auto openMeteoGroup = iotwebconf::ParameterGroup("openMeteo", "Open-Meteo parameters");
        auto openMeteoServerName = iotwebconf::TextParameter("Base URI", "openMeteoServerName", Config::OpenMeteoBaseURI, sizeof(Config::OpenMeteoBaseURI));
        auto openMeteoApiKey = iotwebconf::TextParameter("API Key", "openMeteoApiKey", Config::OpenMeteoAPIKey, sizeof(Config::OpenMeteoAPIKey));
        //auto alarmItemName = iotwebconf::TextParameter("Alarm item name", "alarmItemName", Config::AlarmItemName, sizeof(Config::AlarmItemName), "Alarm");

        void setup()
        {
            // -- Setting up parameters
            iotWebConf.getApTimeoutParameter()->visible = true;
            iotWebConf.addSystemParameter(&ntpServerName);

            openMeteoGroup.addItem(&openMeteoServerName);
            openMeteoGroup.addItem(&openMeteoApiKey);
            //openMeteoGroup.addItem(&alarmItemName);

            iotWebConf.addParameterGroup(&openMeteoGroup);

            iotWebConf.setConfigSavedCallback(&configSaved);

            // -- Initializing the configuration.
            iotWebConf.init();

            // -- Set up required URL handlers on the web server.
            server.on("/", handleRoot);
            server.on("/config", []{ iotWebConf.handleConfig(); });
            server.onNotFound([](){ iotWebConf.handleNotFound(); });
        }

        void loop()
        {
            iotWebConf.doLoop();
        }

        /**
         * Handle web requests to "/" path.
         */
        void handleRoot()
        {
            extern const uint8_t index_html_start[] asm("_binary_data_index_html_start");

            // -- Let IotWebConf test and handle captive portal requests.
            if (iotWebConf.handleCaptivePortal())
            {
                // -- Captive portal request were already served.
                return;
            }

            char localTime[25] = "Failed to obtain time";
            struct tm timeInfo;
            if (getLocalTime(&timeInfo))
                strftime(localTime, sizeof(localTime), "%Y-%m-%d %H:%M:%S %Z", &timeInfo);

            auto indexContent = new String((char*)&index_html_start[0]);
            indexContent->replace(F("%cur_time%"), localTime);

            server.send(200, "text/html", indexContent->c_str());
        }

        void configSaved()
        {
            Network::reconnectServices();
        }
   }
}