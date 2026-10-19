#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>

//ABELL
#include <iostream>

class Buffer
{
private:
    static const std::size_t DefaultElts = 10'000;
    static const std::size_t DefaultEltSize = 4;
    static const std::size_t DefaultSize = DefaultElts * DefaultEltSize;

public:
    Buffer()
    {
        m_start = m_pos = (char *)malloc(DefaultSize);
        m_end = m_start + DefaultSize;
    }

    Buffer(Buffer&& other)
    {
        m_start = other.m_start;
        m_end = other.m_end;
        m_pos = other.m_pos;
        other.m_start = nullptr;
        other.m_end = nullptr;
        other.m_pos = nullptr;
    }

    Buffer(std::size_t size)
    {
        m_start = m_pos = (char *)malloc(size);
        m_end = m_start + size;
    }

    ~Buffer()
    {
        release();
    }

    void expand()
    {
        std::size_t used = m_pos - m_start;
        std::size_t size = m_end - m_start + DefaultSize;
        m_start = (char *)realloc(m_start, size);
        m_end = m_start + size;
        m_pos = m_start + used;
    }

    /**
    void add(void *v, size_t size)
    {
        if (m_pos + size > m_end)
            expand();

        std::memcpy(m_pos, v, size);
        m_pos += size;
    }
    **/
    char *getPos(size_t size)
    {
        assert(size < DefaultSize);
        if (m_pos + size > m_end)
            expand();
        char *pos = m_pos;
        m_pos += size;
        return pos;
    }

    // Right now we just copy the data to the destination buffer.
    void compress(Buffer& dst)
    {
        assert(dst.capacity() >= size());
        std::memcpy(dst.m_start, m_start, size());
        dst.m_end = dst.m_pos = dst.m_start + size();
    }

    void release()
    {
        free(m_start);
        m_start = nullptr;
        m_pos = nullptr;
        m_end = nullptr;
    }

    const char *data() const
    { return m_start; }

    char *start()
    { return m_start; }

    std::size_t size() const
    { return m_pos - m_start; }

    std::size_t capacity() const
    { return m_end - m_start; }

private:
    char *m_start;
    char *m_pos;
    char *m_end;
};

