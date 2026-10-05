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

class CopcProcessor : public Processor
{
public:
    CopcProcessor(PyramidManager& manager, const VoxelInfo&, const BaseInfo& b);

private:
    override createChunk(const VoxelKey& key, pdal::PointViewPtr view);
    void fillPointBuf(pdal::PointRef& point, std::vector<char>& buf, pdal::Dimension::Id bitsDim);
};

} // namespace bu
} // namespace untwine
