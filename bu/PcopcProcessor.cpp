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
#include "PyramidManager.hpp"

class Parquet;

namespace untwine
{
namespace bu
{

PcopcProcessor::PcopcProcessor(PyramidManager& manager, const BaseInfo& b, const VoxelInfo& vi,
    Parquet& parquet) : Processor(manager, b, vi), m_file(parquet), m_rowGroup(parquet)
{
}

Processor::IndexIter
PcopcProcessor::writeOctantCompressed(const OctantInfo& o, Index& index, IndexIter pos)
{
    auto fii = o.fileInfos().begin();
    auto fiiEnd = o.fileInfos().end();
    size_t count = 0;

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
                appendCompressed(*fii, begin, pos);
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
    flushCompressed(o);

    //
    (void)count;
    // m_manager.logOctant(o.key(), count);

    // Need to log the octant after stats.
    return pos;
}

void PcopcProcessor::flushCompressed(const OctantInfo& o)
{
    m_rowGroup.done();
    int64_t position = m_manager.newChunk(o.key(), m_rowGroup.compressedTotalSize(),
        m_rowGroup.numRows());
    m_rowGroup.write(m_file, position);
}

void PcopcProcessor::appendCompressed(const FileInfo& fi, IndexIter begin, IndexIter end)
{
    for (IndexIter it = begin; it != end; ++it)
    {
        char *base = fi.address() + ((*it - fi.start()) + m_b.pointSize);
        m_rowGroup.add(base);
    }
}

} // namespace bu
} // namespace untwine
