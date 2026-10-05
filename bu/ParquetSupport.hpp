#pragma once

#include "../untwine/Common.hpp"

namespace untwine
{
namespace bu
{

class ParquetSupport
{
public:
    ParquetSupport(const BaseInfo& b);

private:
    const BaseInfo& m_b;
};

} // namespace bu
} // namespace untwine
