#include "ColumnChunk.hpp"
#include "Parquet.hpp"
#include "Output.hpp"
#include "Page.hpp"

char *writeDataPageHeader(Page p, Output& output);

void ColumnChunk::add(void *v)
{
    m_buffer.add((char *)v + m_elt.offset(), m_elt.size());
    m_numValues++;
}

void ColumnChunk::compress()
{
    m_uncompressedSize = m_buffer.size();
    m_buffer.compress(m_compressionBuffer);
    m_compressedSize = m_compressionBuffer.size();
    m_buffer.release();
}

void ColumnChunk::tole()
{
    switch (m_elt.type())
    {
    case ParquetType::INT32:
        toleTyped<int32_t>();
        break;
    case ParquetType::INT64:
        toleTyped<int64_t>();
        break;
    case ParquetType::FLOAT:
        toleTyped<float>();
        break;
    case ParquetType::DOUBLE:
        toleTyped<double>();
        break;
    default:
        break;
    }
}

void ColumnChunk::calcStats()
{}

void ColumnChunk::done(Parquet& file)
{
    calcStats();
    tole();
    compress();
    setOffset(file.position());

    BackedOutput pageData(100);

    Page p;
    p.uncompressedSize = m_uncompressedSize;
    p.compressedSize = m_compressedSize;
    p.numValues = (int32_t)m_numValues;
    writeDataPageHeader(p, pageData);
    file.write(pageData.data(), pageData.size());
    file.write(m_compressionBuffer.data(), m_compressionBuffer.size());

    m_uncompressedTotalSize = pageData.size() + m_uncompressedSize;
    m_compressedTotalSize = pageData.size() + m_compressedSize;
    m_compressionBuffer.release();
}

