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

#include "Processor.hpp"

namespace untwine
{
namespace bu
{

class PcopcProcessor : public Processor
{
public:
    PcopcProcessor(PyramidManager& manager, const VoxelInfo&, const BaseInfo& b);

private:
    template <typename T>
    size_t makePage(pdal::DimType dt, std::span<T> data);
    override createChunk(const VoxelKey& key, pdal::PointViewPtr view);
    void flushChunk();
};

template <typename T>
size_t PcopcProcessor::makePage(pdal::DimType dt, std::span<T> data)
{
    //ABELL - Need to add this information to metadata.
    (void)dt;

    size_t offset = m_output.offset();

    // No compression, so compressed and uncompressed size are the same.
    Page page;
    page.uncompressedSize = data.size() * sizeof(T);
    page.compressedSize = data.size() * sizeof(T);
    page.numValues = data.size();

    writeDataPageHeader(page, m_output);

    // No repetition or definition levels.

    m.output.push(data.data(), data.size());

    return offset;
}

} // namespace bu
} // namespace untwine
