#pragma once

template <typename T>
T tole(T t)
{
    if constexpr (std::endian::native == std::endian::little)
      return t;
    char *s = reinterpret_cast<char *>(&t);
    char *e = reinterpret_cast<char *>(&t) + sizeof(T) - 1;
    for (size_t i = 0; i < sizeof(T) / 2; ++i)
    {
        std::swap(*s, *e);
        ++s;
        --e;
    }
    return t;
}
