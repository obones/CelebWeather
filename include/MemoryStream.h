#ifndef MEMORYSTREAM_H_
#define MEMORYSTREAM_H_

#include <Stream.h>

class MemoryStream: public Stream
{
private:
    const uint8_t* buffer;
    size_t bufferSize;
    size_t position;

public:
    MemoryStream(const uint8_t* buffer, const size_t bufferSize);

    size_t write(const uint8_t *buffer, size_t size) override;
    size_t write(uint8_t data) override;

    int available() override;
    int read() override;
    int peek() override;
};

#endif