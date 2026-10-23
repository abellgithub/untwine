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

#include "PcopcPyramidManager.hpp"
#include "PcopcProcessor.hpp"

#include "../untwine/FileDimInfo.hpp"

namespace untwine
{
namespace bu
{

PcopcPyramidManager::PcopcPyramidManager(const BaseInfo& b) :
    PyramidManager(b), m_file(b.opts.outputName)
{}

PcopcPyramidManager::~PcopcPyramidManager()
{}

uint64_t PcopcPyramidManager::newChunk(const VoxelKey& key, uint32_t size, uint32_t count)
{
    std::cerr << "New chunk!\n";
    // We currently do nothing with these, but need to do logging.
    (void)key;
    (void)count;

    return m_position.fetch_add(size);
}

void PcopcPyramidManager::createSchema()
{
    using namespace pdal::Dimension;

    Schema& schema = m_file.schema();
    for (const FileDimInfo& fdi : m_b.dimInfo)
    {
        if (fdi.name == UntwineBitsDimName)
        {
            schema.addColumn<ParquetType::INT32>("Synthetic").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = bits & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
            schema.addColumn<ParquetType::INT32>("Keypoint").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 1) & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
            schema.addColumn<ParquetType::INT32>("Withheld").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 2) & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
            schema.addColumn<ParquetType::INT32>("Overlap").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 3) & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
            schema.addColumn<ParquetType::INT32>("Scanner Channel").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 4) & 0x3;
                    std::memcpy(dst, &bits, 1);
                }
            );
            schema.addColumn<ParquetType::INT32>("Scan Direction").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 6) & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
            schema.addColumn<ParquetType::INT32>("Edge Of Flight Line").
                setOffset(fdi.offset).
                setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 7) & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
        }
        else
        {
            switch (fdi.type)
            {
                case Type::Signed8:
                case Type::Unsigned8:
                {
                    SchemaElt& elt = schema.addColumn<ParquetType::INT32>(fdi.name);
                    elt.setOffset(fdi.offset);
                    elt.setFetch([](void *dst, void *src)
                        {
                            uint8_t *src8 = reinterpret_cast<uint8_t *>(src);
                            uint32_t src32 = *src8;
                            std::memcpy(dst, &src32, sizeof(src32));
                        }
                    );
                    break;
                }
                case Type::Signed16:
                case Type::Unsigned16:
                {
                    SchemaElt& elt = schema.addColumn<ParquetType::INT32>(fdi.name);
                    elt.setOffset(fdi.offset);
                    elt.setFetch([](void *dst, void *src)
                        {
                            uint16_t *src16 = reinterpret_cast<uint16_t *>(src);
                            uint32_t src32 = *src16;
                            std::memcpy(dst, &src32, sizeof(src32));
                        }
                    );
                    break;
                }
                case Type::Signed32:
                case Type::Unsigned32:
                {
                    schema.addColumn<ParquetType::INT32>(fdi.name).setOffset(fdi.offset);
                    break;
                }
                case Type::Signed64:
                case Type::Unsigned64:
                    {
                        schema.addColumn<ParquetType::INT64>(fdi.name).setOffset(fdi.offset);
                        break;
                    }
                case Type::Float:
                {
                    schema.addColumn<ParquetType::FLOAT>(fdi.name).setOffset(fdi.offset);
                    break;
                }
                case Type::Double:
                {
                    schema.addColumn<ParquetType::DOUBLE>(fdi.name).setOffset(fdi.offset);
                    break;
                }
                case Type::None:
                    throw FatalError("Bad 'None' type found in schema when writing to PCOPC");
            } // type switch
        }
    } // dimension loop
}

void PcopcPyramidManager::prepare()
{
    createSchema();
}

void PcopcPyramidManager::finish()
{
    m_file.done();
}

void PcopcPyramidManager::addProcessor(const VoxelInfo& vi)
{
std::cerr << "Add processor for " << vi.key() << "!\n";
std::cerr << "Voxel num points = " << vi.numPoints() << "!\n";
    m_pool.add([vi, this]()
    {
        PcopcProcessor p(*this, m_b, vi, m_file);
        p.run();
        // ABELL - Fix progress.
        //m_progress->writeIncrement("Sample complete for " + vi.key().toString());
    });
}

} // namespace bu
} // namespace untwine
