/*
 CelebWeather project

 The Original Code is config.cpp

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include "config.h"

namespace CelebWeather
{
    namespace Config
    {
        char NTPServerName[NTPServerNameLength] = "pool.ntp.org";

        char Latitude[LatitudeLength] = "";
        char Longitude[LongitudeLength] = "";
        char Timezone[TimezoneLength] = "Europe/Paris";

        char OpenMeteoBaseURI[OpenMeteoBaseURILength] = "https://api.open-meteo.com/v1/";
        char OpenMeteoAPIKey[OpenMeteoAPIKeyLength] = "";

        // not managed via the portal
        char Department[DepartmentLength] = "";
        int RefreshPeriodSeconds = 60 * 60;

        void reset()
        {
            Department[0] = 0;
        }
    }
}
