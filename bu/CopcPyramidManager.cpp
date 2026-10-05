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

#include "CopcPyramidManager.hpp"

namespace untwine
{
namespace bu
{

CopcPyramidManager::CopcPyramidManager(const BaseInfo& b) :
    PyramidManager(b), m_copc(b)
{}

CopcPyramidManager::~CopcPyramidManager()
{}

void CopcPyramidManager::finish()
{
    createHierarchy();

    m_copc.writeChunkTable();
    m_copc.writeHierarchy(m_childCounts);
    m_copc.updateHeader(m_stats);
    // The header is last because we don't have the evlr position until the end.
    m_copc.writeHeader();
}

void CopcPyramidManager::addProcessor(const VoxelInfo& vi)
{
    m_pool.add([vi, this]()
    {
        CopcProcessor p(*this, vi, m_b);
        p.run();
        m_progress->writeIncrement("Sample complete for " + vi.key().toString());
    }
}

// Called when a processor wants to write a chunk. Returns the location
// where the chunk should be written.
uint64_t CopcPyramidManager::newChunk(const VoxelKey& key, uint32_t size, uint32_t count)
{
    static std::mutex m;

    std::unique_lock<std::mutex> lock(m);
    return m_copc.newChunk(key, size, count);
}

} // namespace bu
} // namespace untwine
