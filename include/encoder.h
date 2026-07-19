/*
 CelebWeather project

 The Original Code is encoder.h

 The Initial Developer of the Original Code is Olivier Sannier.
 Portions created by Olivier Sannier are Copyright (C) of Olivier Sannier. All rights reserved.
*/
#include "weather_api_generated.h"

namespace CelebWeather
{
    namespace Encoder
    {
        int EncodeForecast(const openmeteo_sdk::WeatherApiResponse *forecast, int8_t department, unsigned char* destFrame, size_t destFrameSize);

        int EncodeTime(unsigned char* destFrame, size_t destFrameSize);
    }
}