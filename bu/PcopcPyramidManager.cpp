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
    // We currently do nothing with these, but need to do logging.
    (void)key;
    (void)count;

    return m_position.fetch_add(size);
}

void PcopcPyramidManager::createSchema()
{
    using namespace pdal::Dimension;

    for (const FileDimInfo& fdi : m_b.dimInfo)
    {
        if (isUntwineBitsDim(fdi.name))
        {
            SchemaElt& rnElt = m_schema.addColumn<ParquetType::INT32>("Return Number");
            rnElt.setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = bits & 0x7;
                    std::memcpy(dst, &bits, 1);
                }
            );
            SchemaElt& nrElt = m_schema.addColumn<ParquetType::INT32>("Number Of Returns");
            nrElt.setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 3) & 0x7;
                    std::memcpy(dst, &bits, 1);
                }
            );
            SchemaElt& sdElt = m_schema.addColumn<ParquetType::INT32>("Scan Direction Flag");
            sdElt.setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 6) & 0x1;
                    std::memcpy(dst, &bits, 1);
                }
            );
            SchemaElt& eofElt = m_schema.addColumn<ParquetType::INT32>("End of Flight Line");
            eofElt.setFetch([](void *dst, void *src)
                {
                    uint8_t bits = *reinterpret_cast<uint8_t *>(src);
                    bits = (bits >> 7);
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
                    SchemaElt& elt = m_schema.addColumn<ParquetType::INT32>(fdi.name);
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
                    SchemaElt& elt = m_schema.addColumn<ParquetType::INT32>(fdi.name);
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
                    m_schema.addColumn<ParquetType::INT32>(fdi.name);
                    break;
                }
                case Type::Signed64:
                case Type::Unsigned64:
                    {
                        m_schema.addColumn<ParquetType::INT64>(fdi.name);
                        break;
                    }
                case Type::Float:
                {
                    m_schema.addColumn<ParquetType::FLOAT>(fdi.name);
                    break;
                }
                case Type::Double:
                {
                    m_schema.addColumn<ParquetType::DOUBLE>(fdi.name);
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

void PcopcPyramidManager::addProcessor(const VoxelInfo& vi)
{
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
