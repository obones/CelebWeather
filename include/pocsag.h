#include <Arduino.h>

namespace CelebWeather
{
    namespace Pocsag
    {
        int GetBytes(const unsigned char* frame, size_t frameSize, uint8_t * bytes, int maxBytes);
    }
}
