
#include <stringconv.hpp>

#include "ParquetSupport.hpp"

namespace untwine
{
namespace bu
{

//ParquetSupport::ParquetSupport(const BaseInfo& b) : m_b(b)
ParquetSupport::ParquetSupport(const BaseInfo& b)
{
/**
    if (b.opts.a_srs.size())
        m_wkt
**/

    m_f.open(os::toNative(b.opts.outputName), std::ios::out | std::ios::binary);
    m_f.write("PAR1", 4);
}

} // namespace bu
} // namespace untwine
