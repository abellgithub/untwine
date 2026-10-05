/*****************************************************************************
 *   Copyright (c) 2026, Hobu, Inc. (info@hobu.co)                           *
 *                                                                           *
 *   All rights reserved.                                                    *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 ****************************************************************************/

#include "PcopcProcessor.hpp"

namespace untwine
{
namespace bu
{

int32_t tole(int32_t i)
{
    return std::bit_cast<int32_t>(htole32(std::bit_cast<uint32_t>(i)));
}

int64_t tole(int64_t i)
{
    return std::bit_cast<int64_t>(htole64(std::bit_cast<uint64_t>(i)));
}

float tole(float f)
{
    return std::bit_cast<float>(htole32(std::bit_cast<uint32_t>(f)));
}

double tole(double d)
{
    return std::bit_cast<double>(htole64(std::bit_cast<uint64_t>(i)));
}

void PcopcProcessor::flushChunk(const VoxelKey& key, size_t writeSize, size_t numRows)
{
    uint64_t location = m_manager.newChunk(key, writeSize, (uint32_t)numRows);

    std::ofstream out(os::toNative(m_b.opts.outputName),
        std::ios::out | std::ios::in | std::ios::binary);
    out.seekp(std::ofstream::pos_type(location));
    out.write(reinterpret_cast<const char *>(chunk.data()), chunk.size());
    out.close();
    if (!out)
        throw FatalError("Failure writing to file '" + m_b.opts.outputName + "'.");
}

Processor::IndexIter
PcopcProcessor::writeOctantCompressed(const OctantInfo& o, Index& index, IndexIter pos)
{
    auto fii = o.fileInfos.begin();
    auto fiiEnd = o.fileInfos.end();

    IndexIter begin = pos;
    if (fii != fiiEnd)
    {
        // We're trying to find the range of points that come from a single FileInfo.
        // If pos is the end of the index of the the current file info, append the points
        // to the view.  Otherwise, advance the position.
        while (true)
        {
            if (pos == index.end() || *pos >= fii->start() + fii->numPoints())
            {
                count += std::distance(begin, pos);
                appendCompressed(view, dims, *fii, begin, pos);
                if (pos == index.end())
                    break;
                begin = pos;

                // Advance through file infos as long as we don't have points that
                // correspond to it.
                do
                {
                    fii++;
                    if (fii == fiiEnd)
                        goto flush;
                } while (*begin >= fii->start() + fii->numPoints());
            }
            pos++;
        }
    }
flush:
    try
    {
        flushCompressed(o);
    }
    // Need to log the octant after stats.
    return pos;
}

void PcopcProcessor::flushCompressed(const OctantInfo& o)
{
    m_rowGroup.done();
    m_manager.newChunk(o.key(), m_rowGroup.totalCompressedSize(), m_rowGroup.numRows());
}

void PcopcProcessor::appendCompressed(const FileInfo& fi, IndexIter begin, IndexIter end)
{
    for (IndexIter it = begin; it != end; ++it)
    {
        char *base = fi.address() + ((*it - fi.start()) + m_b.pointSize);
        m_rowGroup.add(base);
    }
}

/**
void PcopcProcessor::createChunk(const VoxelKey& key, pdal::PointViewPtr view)
{
    using namespace pdal;
    using namespace pdal::Dimension;

    if (view->size() == 0)
    {
        m_manager.newChunk(key, 0, 0);
        return;
    }

    PointLayoutPtr layout = view->layout();

    std::vector<int32_t> i(view->size());
    std::vector<int64_t> l(view->size());
    std::vector<float> f(view->size());
    std::vector<double> d(view->size());
    for (pdal::DimType dimType : layout->dimsTypes())
    {
        if (dimType.m_id == UntwineBits)
        {
            std::vector<int32_t> rn(view->size());
            std::vector<int32_t> nr(view->size());
            std::vector<int32_t> scandir(view->size());
            std::vector<int32_t> eof(view->size());
            size_t i = 0;
            for (PointRef ref : view)
            {
                uint8_t u = view->getFieldAs<uint8_t>(dimType.m_id);

                rn[i] = tole((int32_t)(u & 0x7));
                nr[i] = tole((int32_t)((u >> 3) & 0x7));
                scandir[i] = tole((int32_t((u >> 6) & 0x1));
                eof[i] = tole((int32_t)(u >> 7));
                ++i;
            }
            makePage(dimType, rn); // Need to store type somewhere.
            makePage(dimType, nr); // Need to store type somewhere.
            makePage(dimType, scandir); // Need to store type somewhere.
            makePage(dimType, eof); // Need to store type somewhere.
            continue;
        }
        switch (dimType.m_type)
        {
            case Type::Signed8:
            case Type::Unsigned8:
            case Type::Signed16:
            case Type::Unsigned16:
            case Type::Signed32:
            {
                size_t pos = 0;
                for (PointRef ref : view)
                    i[pos++] = tole(view->getFieldAs<int32_t>(dimType.m_id));
                makePage(dimType, i);
                break;
            }
            case Type::Unsigned32:
            {
                size_t pos = 0
                for (PointRef ref : view)
                {
                    uint32_t u = view->getFieldAs<uint32_t>(dimType.m_id);
                    i[pos++] = tole(std::bit_cast<int32_t>(u));
                }
                makePage(dimType, i);
                break;
            }
            case Type::Signed64:
            {
                size_t pos = 0;
                for (PointRef ref : view)
                    l[pos++] = tole(view->getFieldAs<int64_t>(dimType.m_id));
                makePage(dimType, l);
                break;
            }
            case Type::Unsigned64:
            {
                size_t pos = 0;
                for (PointRef ref : view)
                {
                    uint64_t u = view->getFieldAs<unt64_t>(dimType.m_id);
                    l[pos++] = tole(std::bit_cast<int64_t>(u));
                }
                makePage(dimType, l);
                break;
            }
            case Type::Float:
            {
                size_t pos = 0;
                for (PointRef ref : view)
                    f[pos++] = tole(view->getFieldAs<float>(dimType.m_id));
                makePage(dimType, f);
                break;
            }
            case Type::Double:
            {
                for (PointRef ref : view)
                    d[pos++] = tole(view->getFieldAs<double>(dimType.m_id));
                makePage(dimType, d);
                break;
            }
        }
    }
    flushChunk(key, m_output.size(), view->size());
}
**/

} // namespace bu
} // namespace untwine
