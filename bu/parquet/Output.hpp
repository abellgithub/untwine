#pragma once

#include <vector>

class Output
{
public:
    Output(char *buf, std::size_t bufSize) : start(buf), buffer(buf), bufSize(bufSize), curSize(0)
    {}
    Output() : start(nullptr), buffer(nullptr), bufSize(0), curSize(0)
    {}

    void put(char c)
    {
        ensure(sizeof(char));
        *buffer++ = c;
    }

    void put(const char *c, std::size_t size)
    {
        ensure(size);
        memcpy(buffer, c, size);
        buffer += size;
    }

    std::size_t size() const
    { return curSize; }

    std::size_t offset() const
    { return curSize; }

    const char *data() const
    { return start; }

private:
    virtual void ensure(std::size_t size)
    {
        if (size + curSize > bufSize)
            throw "Exceeded output buffer size";
        curSize += size;
    }

protected:
    char *start;
    char *buffer;
    std::size_t bufSize;
    std::size_t curSize;
};

class BackedOutput : public Output
{
public:
    BackedOutput(size_t _bufSize) : storage(_bufSize)
    {
        start = buffer = storage.data();
        bufSize = _bufSize;
    }

    void ensure(std::size_t size) override
    {
        if (size + curSize > bufSize)
        {
            storage.resize(size + curSize + 100);
            start = storage.data();

            bufSize = size + curSize + 100;
            buffer = start + curSize;
        }
        curSize += size;
    }

    std::vector<char> storage;
};

