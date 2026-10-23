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

#include "parquet/RowGroup.hpp"

namespace untwine
{
namespace bu
{

class PcopcProcessor : public Processor
{
public:
    PcopcProcessor(PyramidManager& manager, const BaseInfo& b, const VoxelInfo& vi,
        Parquet& parquet);

private:
    Processor::IndexIter writeOctantCompressed(const OctantInfo& o, Index& index, IndexIter pos)
        override;

    void appendCompressed(RowGroup& rowGroup, const FileInfo& fi, IndexIter begin, IndexIter end);
    void flushCompressed(RowGroup& rowGroup, const OctantInfo& o);

    Parquet& m_file;
};

} // namespace bu
} // namespace untwine
