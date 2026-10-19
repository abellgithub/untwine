/*****************************************************************************
 *   Copyright (c) 2020, Hobu, Inc. (info@hobu.co)                           *
 *                                                                           *
 *   All rights reserved.                                                    *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 ****************************************************************************/

#pragma once

#include "PointAccessor.hpp"
#include "Stats.hpp"
#include "VoxelInfo.hpp"

namespace untwine
{

class GridKey;

namespace bu
{

class OctantInfo;
class PyramidManager;

class Processor
{
public:
    Processor(PyramidManager& manager, const BaseInfo& b, const VoxelInfo& vi);

    void run();

protected:
    using Index = std::deque<int>;
    using IndexIter = Index::const_iterator;

    PyramidManager& m_manager;
    const BaseInfo& m_b;

private:
    void runLocal();
    void sample(Index& accepted, Index& rejected);
    void write(Index& accepted, Index& rejected);
    bool acceptable(int pointId, GridKey key);
    // Only used in more complex sampling.
    //    bool tooClose(pdal::PointId id1, pdal::PointId id2);

    void appendRemainder(Index& index);
    void writeBinOutput(Index& index);
    void writeCompressedOutput(Index& index);
    virtual IndexIter writeOctantCompressed(const OctantInfo& o, Index& index, IndexIter pos) = 0;

    VoxelInfo m_vi;
    PointAccessor m_points;
};

} // namespace bu
} // namespace untwine
