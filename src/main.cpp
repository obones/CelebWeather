/*
 CelebWeather project

 The Original Code is main.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <Arduino.h>
#include "portal.h"
#include "network.h"
#include "status.h"

void setup()
{
    delay(250);         // Time needed to switch back from Upload to Console
    Serial.begin(115200); // Initialise the serial port

    Serial.println(); // split from ESP "boot" message
    Serial.print(F("Arduino IDE Version :\t"));
    Serial.println(ARDUINO);
    Serial.print(F("ESP SDK version:\t"));
    Serial.println(ESP.getSdkVersion());
    Serial.println(F("Compiled on :\t\t" __DATE__ " at " __TIME__));

    CelebWeather::Portal::setup();
    CelebWeather::Network::setup();
    CelebWeather::Status::setup();

    Serial.println("Ready.");
}

void loop()
{
    CelebWeather::Portal::loop();
    CelebWeather::Status::loop();
}