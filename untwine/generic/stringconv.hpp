#pragma once

#include <string>

namespace untwine
{
namespace os
{

inline std::string toNative(const std::string& in)
{
    return in;
}

inline std::string fromNative(const std::string& in)
{
    return in;
}

} // namespace os
} // namespace untwine
