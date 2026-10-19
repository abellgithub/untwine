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

#include <numeric>
#include <random>

#include "../untwine/GridKey.hpp"

#include <pdal/PDALUtils.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/filters/SortFilter.hpp>
#include <pdal/util/Algorithm.hpp>

#include "Processor.hpp"
#include "PyramidManager.hpp"

#include <stringconv.hpp>  // untwine/os

namespace untwine
{
namespace bu
{

static const int MinimumPoints = 100;
static const int MinimumTotalPoints = 1500;

Processor::Processor(PyramidManager& manager, const BaseInfo& b, const VoxelInfo& v) :
    m_manager(manager), m_b(b), m_vi(v), m_points(m_b)
{}


void Processor::run()
{
    // Don't let any exception sneak out of here.
    try
    {
        runLocal();
    }
    catch (const std::exception& ex)
    {
        m_manager.queueWithError(m_vi.octant(), ex.what());
        return;
    }
    catch (...)
    {
        std::string msg = std::string("Unexpected error processing ") + m_vi.key().toString() + ".";
        m_manager.queueWithError(m_vi.octant(), msg);
        return;
    }
    m_manager.queue(m_vi.octant());
}

void Processor::runLocal()
{
    // If we don't merge small files into one, we'll end up trying to deal with too many
    // open files later and run out of file descriptors.
    for (int i = 0; i < 8; ++i)
    {
        OctantInfo& child = m_vi[i];
        if (child.fileInfos().size() >= 4)
            child.mergeSmallFiles(m_b.opts.tempDir, m_b.pointSize);
    }

    size_t totalPoints = 0;
    size_t totalFileInfos = 0;
    for (int i = 0; i < 8; ++i)
    {
        OctantInfo& child = m_vi[i];
        totalFileInfos += child.fileInfos().size();
        totalPoints += child.numPoints();
        if (child.numPoints() < MinimumPoints)
            m_vi.octant().appendFileInfos(child);
    }

    // It's possible that all the file infos have been moved above, but this is cheap.
    if (totalPoints < MinimumTotalPoints)
        for (int i = 0; i < 8; ++i)
            m_vi.octant().appendFileInfos(m_vi[i]);

    // Accepted points are those that will go in this (the parent) cell.
    // Rejected points will remain in the child cell they were in previously.
    Index accepted;
    Index rejected;

    // If the file infos haven't all been hoisted, sample.
    if (m_vi.octant().fileInfos().size() != totalFileInfos)
        sample(accepted, rejected);

    write(accepted, rejected);
}


void Processor::sample(Index& accepted, Index& rejected)
{
    int totalPoints = 0;
    for (int i = 0; i < 8; ++i)
    {
        OctantInfo& child = m_vi[i];
        for (FileInfo& fi : child.fileInfos())
        {
            m_points.read(fi);
            totalPoints += fi.numPoints();
        }
    }

    std::deque<int> index(totalPoints);
    std::iota(index.begin(), index.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd());
    /**
    std::vector<int32_t> v{1234};
    std::seed_seq seed(v.begin(), v.end());
    std::mt19937 g(seed);
    **/

    //ABELL - This may not be the best way to do this. Probably better to work from some
    //  point (center, whatever) out, but this is cheap because you don't have to do
    //  any computation with the point data. And I would think you should still get good
    //  output, but it may be more sparse. Seems you could fix that by just choosing a
    //  smaller radius.  Should be tested.
    std::shuffle(index.begin(), index.end(), g);

    while (index.size())
    {
        int i = index.back();

        const Point& p = m_points[i];
        GridKey k = m_vi.gridKey(p);

        // If we're accepting this point into this voxel from it's child, add it
        // to the accepted list and also stick it in the grid.
        if (acceptable(i, k))
        {
            accepted.push_back(i);
            m_vi.grid().insert( {k, i} );
        }
        else
            rejected.push_back(i);

        index.pop_back();
    }
}


void Processor::write(Index& accepted, Index& rejected)
{
    // If this is the final key, append any remaining file infos as accepted points and
    // write the accepted points as compressed.
    if (m_vi.key() == VoxelKey(0, 0, 0, 0))
    {
        appendRemainder(accepted);
        writeOctantCompressed(m_vi.octant(), accepted, accepted.begin());
    }
    else
        writeBinOutput(accepted);
    writeCompressedOutput(rejected);
}


bool Processor::acceptable(int pointId, GridKey key)
{
    //ABELL - Currently unused - see commented-out code.
    (void)pointId;

    VoxelInfo::Grid& grid = m_vi.grid();

    auto it = grid.find(key);

    // If the cell is already occupied the point is not acceptable.
    if (it != grid.end())
        return false;
    return true;
/**
    // We place points in a voxel grid to reduce the number of tests necessary
    // to determine if a new point can be placed without being too close.
    // We size the voxels such that the diagonal is the length of our radius.
    // This way we KNOW that once a cell is occupied, no other point can
    // be placed there. This means the edge length of the voxel cell is
    // radius / √3 = .577 * radius. So, when trying to place a point on the far
    // right side of a cell, it's possible that there's another point already in
    // a cell 2 cells to the right that's only radius * .577 + ε away.

    // ABELL - This should probably be moved to a Grid class.

    // Ignore cells outside of the area of interest.
    int i0 = std::max(key.i() - 2, 0);
    int j0 = std::max(key.j() - 2, 0);
    int k0 = std::max(key.k() - 2, 0);
    int i1 = std::min(key.i() + 2, m_vi.gridXCount());
    int j1 = std::min(key.j() + 2, m_vi.gridYCount());
    int k1 = std::min(key.k() + 2, m_vi.gridZCount());

    for (int i = i0; i <= i1; ++i)
    for (int j = j0; j <= j1; ++j)
    for (int k = k0; k <= k1; ++k)
    {
        //ABELL - Is it worth skipping key location itself or the corner cells?
        auto gi = grid.find(GridKey(i, j, k));
        if (gi != grid.end() && tooClose(pointId, gi->second))
            return false;
    }
    return true;
**/
}


/**
bool Processor::tooClose(pdal::PointId id1, pdal::PointId id2)
{
    const Point& p1 = m_points[id1];
    const Point& p2 = m_points[id2];

    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    double dz = p1.z() - p2.z();

    return dx * dx + dy * dy + dz * dz <= m_vi.squareSpacing();
}
**/


void Processor::writeBinOutput(Index& index)
{
    if (index.empty())
        return;

    // Write the accepted points in binary format. Create a FileInfo to describe the
    // file and it to the octant representing this voxel as it bubbles up.
    // Note that we write the the input directory, as this will be input to a later
    // pass.
    std::string filename = m_vi.key().toString() + ".bin";
    std::string fullFilename = m_b.opts.tempDir + "/" + filename;
    std::ofstream out(os::toNative(fullFilename), std::ios::binary | std::ios::trunc);
    if (!out)
        throw FatalError("Couldn't open '" + fullFilename + "' for output.");
    for (size_t i = 0; i < index.size(); ++i)
        out.write(m_points[index[i]].cdata(), m_b.pointSize);
    m_vi.octant().appendFileInfo(FileInfo(filename, index.size()));
}


// This is a bit confusing.  When we get to the last node, we have two sets of points that
// need to get written to the final (0-0-0-0) node. Those include the points accepted from
// sampling as well as any points that were simply hoisted here due to small size.
//
// We read the hoisted points to stick them on the PointAccessor then we number the points
// by adding them to the index (accepted list).  Then we move the FileInfos from the
void Processor::appendRemainder(Index& index)
{
    std::sort(index.begin(), index.end());

    // Save the current size;
    size_t offset = m_points.size();

    // Read the points from the remaining FileInfos.
    for (FileInfo& fi : m_vi.octant().fileInfos())
        m_points.read(fi);
    size_t numRead = m_points.size() - offset;
    size_t origIndexSize = index.size();

    // Resize the index to contain the read points.
    index.resize(origIndexSize + numRead);

    // The points in the remaining hoisted FileInfos are just numbered sequentially.
    std::iota(index.begin() + origIndexSize, index.end(), offset);

    // NOTE: We need to maintain the order of the file infos as they were read, which
    //   is why they're prepended in reverse.
    // NOTE: The FileInfo pointers in the PointAccessor should remain valid as the
    //   file infos are spliced from the child octant lists onto the parent list.
    for (int i = 8; i > 0; i--)
    {
        FileInfoList& fil = m_vi[i - 1].fileInfos();
        for (auto fi = fil.rbegin(); fi != fil.rend(); ++fi)
            m_vi.octant().prependFileInfo(*fi);
    }
}


void Processor::writeCompressedOutput(Index& index)
{
    // By sorting the rejected points, they will be ordered to match the FileInfo items --
    // meaning that all points that belong in one file will be consecutive.
    std::sort(index.begin(), index.end());

    IndexIter pos = index.begin();

    // If any of our octants has points, we have to write the parent octant, whether or not
    // it contains points, in order to create a full tree.
    for (int octant = 0; octant < 8; ++octant)
        if (m_vi[octant].hasPoints() || m_vi[octant].mustWrite())
        {
            m_vi.octant().setMustWrite(true);
            pos = writeOctantCompressed(m_vi[octant], index, pos);
        }
}

} // namespace bu
} // namespace untwine
