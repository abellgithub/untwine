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

#include <pdal/PointView.hpp>

#include "Processor.hpp"

namespace untwine
{
namespace bu
{

class CopcProcessor : public Processor
{
public:
    CopcProcessor(PyramidManager& manager, const BaseInfo& b, const VoxelInfo& vi);

private:
    void createChunk(const VoxelKey& key, pdal::PointViewPtr view);
    void fillPointBuf(pdal::PointRef& point, std::vector<char>& buf, pdal::Dimension::Id bitsDim);
    void flushCompressed(pdal::PointViewPtr view, const OctantInfo& oi, IndexedStats& stats);
    DimInfoList makeSchema(pdal::PointLayoutPtr layout, IndexedStats& stats);
    IndexIter writeOctantCompressed(const OctantInfo& o, Index& index, IndexIter pos);
    void appendCompressed(pdal::PointViewPtr view, const DimInfoList& dims,
        const FileInfo& fi, IndexIter begin, IndexIter end);

    pdal::DimTypeList m_extraDims;
};

} // namespace bu
} // namespace untwine
