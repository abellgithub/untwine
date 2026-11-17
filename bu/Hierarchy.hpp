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

#pragma once

#include "../untwine/VoxelKey.hpp"

#include <pdal/util/OStream.hpp>

namespace untwine
{
namespace bu
{

class Hierarchy
{
public:
    struct Entry
    {
        uint64_t offset;
        int32_t byteSize;
        int32_t pointCount;
    };

    using CountMap = std::unordered_map<VoxelKey, int>;
    using Entries = std::vector<std::pair<VoxelKey, Entry>>;

    void newChunk(const VoxelKey& key, int32_t size, int32_t count, uint64_t chunkStart);
    Entry emitRoot(pdal::OLeStream& out, const VoxelKey& root, const CountMap& counts);

private:
    std::unordered_map<VoxelKey, Entry> m_hierarchy;

    void emitChildren(pdal::OLeStream& out, const VoxelKey& root, const CountMap& counts,
        Entries& entries, int stopLevel);
};

} // namesapce bu
} // namesapce untwine
