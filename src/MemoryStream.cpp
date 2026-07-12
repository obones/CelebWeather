#include <String.h>
#include "MemoryStream.h"

MemoryStream::MemoryStream(const uint8_t* buffer, const size_t bufferSize)
{
    this->buffer = buffer;
    this->bufferSize = bufferSize;
    this->position = 0;
}

size_t MemoryStream::write(const uint8_t *buffer, size_t size)
{
    return 0;
}

size_t MemoryStream::write(uint8_t data)
{
    return write(&data, sizeof(data));
}

int MemoryStream::available()
{
    return 0;
}

int MemoryStream::read()
{
    return 0;
}

int MemoryStream::peek()
{
    return 0;
}
