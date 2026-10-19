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

#include "PyramidManager.hpp"
#include "CopcSupport.hpp"

namespace untwine
{
namespace bu
{

class CopcPyramidManager : public PyramidManager
{
public:
    CopcPyramidManager(const BaseInfo& b);
    ~CopcPyramidManager() override;

    void addProcessor(const VoxelInfo& vi) override;
    uint64_t newChunk(const VoxelKey& key, uint32_t size, uint32_t count) override;

private:
    CopcSupport m_copc;

    void finish() override;
};

} // namespace bu
} // namespace untwine
