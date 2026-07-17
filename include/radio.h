/*
 CelebWeather project

 The Original Code is radio.h

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include <Arduino.h>

namespace CelebWeather
{
    namespace Radio
    {
        void setup();

        bool transmit(uint8_t* bytes, unsigned int len);
    }
}