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

#include "Hierarchy.hpp"

namespace untwine
{
namespace bu
{

/// \param  size  Size of the chunk in bytes
/// \param  count  Number of points in the chunk
/// \param  key  Key of the voxel the chunk represents
/// \param  chunkStart  Starting location of the chunk.
void Hierarchy::newChunk(const VoxelKey& key, int32_t size, int32_t count, uint64_t chunkStart)
{
    // Chunks of size zero are a special case.
    if (count == 0)
    {
        m_hierarchy[key] = { 0, 0, 0 };
        return;
    }

    m_hierarchy[key] = { chunkStart, size, count };
}

Hierarchy::Entry Hierarchy::emitRoot(pdal::OLeStream& out, const VoxelKey& root,
    const CountMap& counts)
{
    const int LevelBreak = 4;
    Entries entries;

    int stopLevel = root.level() + LevelBreak;
    entries.push_back({root, m_hierarchy[root]});
    emitChildren(out, root, counts, entries, stopLevel);

    uint64_t startPos = out.position();
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        VoxelKey& key = it->first;
        Entry& e = it->second;
        out << key.level() << key.x() << key.y() << key.z();
        out << e.offset << e.byteSize << e.pointCount;
    }
    uint64_t endPos = out.position();

    // This is the information about where the hierarchy node was written, to be
    // written with the parent.
    return { startPos, (int32_t)(endPos - startPos), -1 };
}


void Hierarchy::emitChildren(pdal::OLeStream& out, const VoxelKey& p, const CountMap& counts,
    Entries& entries, int stopLevel)
{
    const int MinHierarchySize = 50;

    for (int i = 0; i < 8; ++i)
    {
        VoxelKey c = p.child(i);
        auto ci = counts.find(c);
        if (ci != counts.end())
        {
            // If we're not at a stop level or the number of child nodes is less than 50,
            // just stick them here.
            if (c.level() != stopLevel || ci->second <= MinHierarchySize)
            {
                entries.push_back({c, m_hierarchy[c]});
                emitChildren(out, c, counts, entries, stopLevel);
            }
            else
                entries.push_back({c, emitRoot(out, c, counts)});
        }
    }
}

} // namespace bu
} // namespace untwine
