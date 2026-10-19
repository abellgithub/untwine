#pragma once

#include <fstream>

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
//    const BaseInfo& m_b;
    std::ofstream m_f;
};

} // namespace bu
} // namespace untwine
