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

#include "CopcProcessor.hpp"
#include "PyramidManager.hpp"

#include <pdal/io/BufferReader.hpp>
#include <pdal/filters/SortFilter.hpp>

#include <lazperf/lazperf.hpp>
#include <lazperf/writers.hpp>
#include <lazperf/readers.hpp>

#include <stringconv.hpp>  // untwine/os

namespace untwine
{
namespace bu
{

namespace
{

void sortChunk(pdal::PointViewPtr view)
{
    pdal::BufferReader r;
    r.addView(view);

    pdal::SortFilter s;
    s.setInput(r);
    pdal::Options o;
    o.add("dimension", "GpsTime");
    s.setOptions(o);

    s.prepare(view->table());
    s.execute(view->table());
}

} // namespace

CopcProcessor::CopcProcessor(PyramidManager& manager, const BaseInfo& b, const VoxelInfo& vi) :
    Processor(manager, b, vi)
{}

void CopcProcessor::flushCompressed(pdal::PointViewPtr view, const OctantInfo& oi,
    IndexedStats& stats)
{
    // For single file output we need the stats for
    if (m_b.opts.stats)
    {
        for (pdal::PointId id = 0; id < view->size(); ++id)
        {
            for (auto& sp : stats)
            {
                pdal::Dimension::Id dim = sp.first;
                Stats& s = sp.second;
                s.insert(view->getFieldAs<double>(dim, id));
            }
        }
    }

    createChunk(oi.key(), view);
}

void CopcProcessor::fillPointBuf(pdal::PointRef& point, std::vector<char>& buf,
    pdal::Dimension::Id bitsDim)
{
    using namespace pdal;

    LeInserter ostream(buf.data(), buf.size());

    bool hasTime = true; //  m_lasHeader.hasTime();
    bool hasColor = m_b.pointFormatId == 7 || m_b.pointFormatId == 8;
    bool hasInfrared = m_b.pointFormatId == 8;

    // we always write the base fields
    using namespace Dimension;

    uint8_t returnNumber(1);
    uint8_t numberOfReturns(1);
    if (point.hasDim(Id::ReturnNumber))
        returnNumber = point.getFieldAs<uint8_t>(Id::ReturnNumber);
    if (point.hasDim(Id::NumberOfReturns))
        numberOfReturns = point.getFieldAs<uint8_t>(Id::NumberOfReturns);

    auto converter = [](double d, Dimension::Id dim) -> int32_t
    {
        int32_t i(0);

        if (!Utils::numericCast(d, i))
            throw FatalError("Unable to convert scaled value (" +
                Utils::toString(d) + ") to "
                "int32 for dimension '" + Dimension::name(dim) +
                "' when writing LAS/LAZ file.");
        return i;
    };

    double x = point.getFieldAs<double>(Id::X);
    int32_t xi = converter((x - m_b.xform.offset.x) / m_b.xform.scale.x, Id::X);
    double y = point.getFieldAs<double>(Id::Y);
    int32_t yi = converter((y - m_b.xform.offset.y) / m_b.xform.scale.y, Id::Y);
    double z = point.getFieldAs<double>(Id::Z);
    int32_t zi = converter((z - m_b.xform.offset.z) / m_b.xform.scale.z, Id::Z);

    ostream << xi << yi << zi;

    ostream << point.getFieldAs<uint16_t>(Id::Intensity);
    ostream << (uint8_t)(returnNumber | (numberOfReturns << 4));
    ostream << point.getFieldAs<uint8_t>(bitsDim);
    ostream << point.getFieldAs<uint8_t>(Id::Classification);

    uint8_t userData = point.getFieldAs<uint8_t>(Id::UserData);
     // Guaranteed to fit if scan angle rank isn't wonky.
    int16_t scanAngleRank =
        static_cast<int16_t>(std::round(
            point.getFieldAs<float>(Id::ScanAngleRank) / .006f));
    ostream << userData << scanAngleRank;

    ostream << point.getFieldAs<uint16_t>(Id::PointSourceId);

    if (hasTime)
        ostream << point.getFieldAs<double>(Id::GpsTime);

    if (hasColor)
    {
        ostream << point.getFieldAs<uint16_t>(Id::Red);
        ostream << point.getFieldAs<uint16_t>(Id::Green);
        ostream << point.getFieldAs<uint16_t>(Id::Blue);
    }

    if (hasInfrared)
        ostream << point.getFieldAs<uint16_t>(Id::Infrared);

    Everything e;
    for (auto& dim : m_extraDims)
    {
        point.getField((char *)&e, dim.m_id, dim.m_type);
        Utils::insertDim(ostream, dim.m_type, e);
    }
}

//ABELL - Should make m_b.dimInfo modifiable and just set the dimension ID directly.
DimInfoList CopcProcessor::makeSchema(pdal::PointLayoutPtr layout, IndexedStats& stats)
{
    DimInfoList dims = m_b.dimInfo;
    m_extraDims.clear();
    for (FileDimInfo& fdi : dims)
    {
        fdi.dim = layout->registerOrAssignDim(fdi.name, fdi.type);
        if (m_b.opts.stats)
        {
            if (fdi.dim == pdal::Dimension::Id::Classification)
                stats.push_back({fdi.dim, Stats(fdi.name, Stats::EnumType::Enumerate, false)});
            else if (fdi.dim == pdal::Dimension::Id::ReturnNumber)
                stats.push_back({fdi.dim, Stats(fdi.name, Stats::EnumType::Enumerate, false)});
            else
                stats.push_back({fdi.dim, Stats(fdi.name, Stats::EnumType::NoEnum, false)});
        }
        if (fdi.extraDim)
            m_extraDims.push_back(pdal::DimType(fdi.dim, fdi.type));
    }
    return dims;
}

// Copy data from the source file to the point view.
void CopcProcessor::appendCompressed(pdal::PointViewPtr view, const DimInfoList& dims,
    const FileInfo& fi, IndexIter begin, IndexIter end)
{
    //ABELL - This could be improved by making a point table that handles a bunch
    //  of FileInfos/raw addresses. It would totally avoid the copy.
    pdal::PointId pointId = view->size();
    for (IndexIter it = begin; it != end; ++it)
    {
        char *base = fi.address() + ((*it - fi.start()) * m_b.pointSize);
        for (const FileDimInfo& fdi : dims)
            view->setField(fdi.dim, fdi.type, pointId,
                reinterpret_cast<void *>(base + fdi.offset));
        pointId++;
    }
}

// o        Octant we're writing.
// index    Index of all rejected points that were rejected and not hoisted into the parent.
// pos      Start position of this octant's point in the index.
// \return  Position of the first point in the next octant of our index.
Processor::IndexIter
CopcProcessor::writeOctantCompressed(const OctantInfo& o, Index& index, IndexIter pos)
{
    using namespace pdal;

    PointTable table;
    IndexedStats stats;

    DimInfoList dims = makeSchema(table.layout(), stats);
    table.finalize();

    PointViewPtr view(new pdal::PointView(table));

std::cerr << "Write compressed octant = " << o.key() << "!\n";
    // The octant's points can came from one or more FileInfo.  The points are sorted such
    // all the points that come from a single FileInfo are consecutive.
    auto fii = o.fileInfos().begin();
    auto fiiEnd = o.fileInfos().end();
    size_t count = 0;

    IndexIter begin = pos;
    if (fii != fiiEnd)
    {
        // We're trying to find the range of points that come from a single FileInfo.
        // If pos is the end of the index of the the current file info, append the points
        // to the view.  Otherwise, advance the position.
        while (true)
        {
            if (pos == index.end() || *pos >= fii->start() + fii->numPoints())
            {
                count += std::distance(begin, pos);
                appendCompressed(view, dims, *fii, begin, pos);
                if (pos == index.end())
                    break;
                begin = pos;

                // Advance through file infos as long as we don't have points that
                // correspond to it.
                do
                {
                    fii++;
                    if (fii == fiiEnd)
                        goto flush;
                } while (*begin >= fii->start() + fii->numPoints());
            }
            pos++;
        }
    }
flush:
    try
    {
        flushCompressed(view, o, stats);
    }
    catch (pdal_error& err)
    {
        throw FatalError(err.what());
    }
    m_manager.logOctant(o.key(), count, stats);
    return pos;
}

void CopcProcessor::createChunk(const VoxelKey& key, pdal::PointViewPtr view)
{
    using namespace pdal;

    if (view->size() == 0)
    {
        m_manager.newChunk(key, 0, 0);
        return;
    }

    // Sort the chunk on GPS time.
    if (view->layout()->hasDim(Dimension::Id::GpsTime))
        sortChunk(view);

    PointLayoutPtr layout = view->layout();


    int ebCount {0};
    for (DimType dim : m_extraDims)
        ebCount += layout->dimSize(dim.m_id);

    std::vector<char> buf(lazperf::baseCount(m_b.pointFormatId) + ebCount);
    lazperf::writer::chunk_compressor compressor(m_b.pointFormatId, ebCount);
    for (PointId idx = 0; idx < view->size(); ++idx)
    {
        PointRef point(*view, idx);
        fillPointBuf(point, buf, layout->findDim(UntwineBitsDimName));
        compressor.compress(buf.data());
    }
    std::vector<unsigned char> chunk = compressor.done();

    uint64_t location = m_manager.newChunk(key, chunk.size(), (uint32_t)view->size());

    std::ofstream out(os::toNative(m_b.opts.outputName),
        std::ios::out | std::ios::in | std::ios::binary);
    out.seekp(std::ofstream::pos_type(location));
    out.write(reinterpret_cast<const char *>(chunk.data()), chunk.size());
    out.close();
    if (!out)
        throw FatalError("Failure writing to file '" + m_b.opts.outputName + "'.");
}

} // namespace bu
} // namespace untwine
