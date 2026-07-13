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
        void EncodeForecast(const openmeteo_sdk::WeatherApiResponse *forecast, unsigned char* destFrame, size_t destFrameSize);

        void EncodeTime(unsigned char* destFrame, size_t destFrameSize);
    }
}