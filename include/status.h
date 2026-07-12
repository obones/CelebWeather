/*
 CelebWeather project

 The Original Code is status.h

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#ifndef STATUS_H
#define STATUS_H

#include <time.h>
#include <string>

namespace CelebWeather
{
    namespace Status
    {
        extern struct timeval TimeAtBoot;
        extern bool Connected;

        void setup();

        void loop();
    }
}
#endif