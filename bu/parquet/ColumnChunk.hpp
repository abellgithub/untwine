#pragma once

//ABELL
#include <iostream>
#include <fstream>

#include "Buffer.hpp"
#include "Misc.hpp"
#include "Output.hpp"
#include "Schema.hpp"

class Parquet;

class ColumnChunk
{
public:
    ColumnChunk(const SchemaElt& elt) : m_elt(elt), m_pageData(100)
    {}

    void add(void *v);
    void compress();
    void tole();
    void calcStats();
    void done();
    void write(std::ofstream& out);

    template <typename T> void toleTyped();

    ParquetType type() const
    { return m_elt.type(); }
    CompressionCodec compression() const
    { return m_compression; }
    int64_t numValues() const
    { return m_numValues; }
    int64_t uncompressedSize() const
    { return m_uncompressedSize; }
    int64_t uncompressedTotalSize() const
    { return m_uncompressedTotalSize; }
    int64_t compressedSize() const
    { return m_compressedSize; }
    int64_t compressedTotalSize() const
    { return m_compressedTotalSize; }
    int64_t offset() const
    { return m_dataPageOffset; }
    void setOffset(int64_t offset)
    { m_dataPageOffset = offset; }

private:
    const SchemaElt& m_elt;
    Buffer m_buffer;
    Buffer m_compressionBuffer;
    BackedOutput m_pageData;  // Page header data.
    CompressionCodec m_compression = CompressionCodec::UNCOMPRESSED;
    int64_t m_numValues = 0;
    int64_t m_uncompressedSize = 0;
    int64_t m_uncompressedTotalSize = 0;
    int64_t m_compressedSize = 0;
    int64_t m_compressedTotalSize = 0;
    int64_t m_dataPageOffset = 0;
};
using ColumnChunks = std::vector<ColumnChunk>;


template <typename T>
void ColumnChunk::toleTyped()
{
    T *val = reinterpret_cast<T *>(m_buffer.start());
    for (int64_t i = 0; i < m_numValues; i++)
    {
        *val = ::tole(*val);
        val++;
    }
}
