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

#include <atomic>

#include "PyramidManager.hpp"
#include "parquet/Parquet.hpp"
#include "parquet/Schema.hpp"

namespace untwine
{
namespace bu
{

class PcopcPyramidManager : public PyramidManager
{
public:
    PcopcPyramidManager(const BaseInfo& b);
    ~PcopcPyramidManager() override;

    uint64_t newChunk(const VoxelKey& key, uint32_t size, uint32_t count) override;

private:
    void prepare() override;
    void finish() override;
    void addProcessor(const VoxelInfo& vi) override;
    void createSchema();

    Parquet m_file;
    // Location where data should be written. Data starts at offset 4 (after the magic number).
    std::atomic<uint64_t> m_position = 4;
};

} // namespace bu
} // namespace untwine
