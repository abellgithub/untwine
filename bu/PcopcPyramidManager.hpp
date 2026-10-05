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
#include "parquet/Schema.hpp"

namespace untwine
{
namespace bu
{

class PyramidManager
{
public:
    PcopcPyramidManager(const BaseInfo& b);
    ~PcopcPyramidManager();

private:
    void prepare() override;
    void addProcessor(const VoxelInfo& vi) override;
    void createSchema();

    parquet::Schema m_schema;
};

} // namespace bu
} // namespace untwine
