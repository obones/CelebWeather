/*
 CelebWeather project

 The Original Code is config.h

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#ifndef CONFIG_H
#define CONFIG_H
namespace CelebWeather
{
    namespace Config
    {
        const int NTPServerNameLength = 100;

        const int OpenMeteoBaseURILength = 100;
        const int OpenMeteoAPIKeyLength = 256;

        extern char NTPServerName[NTPServerNameLength];

        extern char OpenMeteoBaseURI[OpenMeteoBaseURILength];
        extern char OpenMeteoAPIKey[OpenMeteoAPIKeyLength];

        extern int RefreshPeriod;
    }
}
#endif