#include "ColumnChunk.hpp"
#include "Parquet.hpp"
#include "Output.hpp"
#include "Page.hpp"

char *writeDataPageHeader(Page p, Output& output);

void ColumnChunk::add(void *v)
{
    char *dst = m_buffer.getPos(m_elt.size());
    char *src = reinterpret_cast<char *>(v) + m_elt.offset();
    m_elt.fetch()(dst, src);
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

void ColumnChunk::done()
{
    calcStats();
    tole();
std::cerr << "Done column chunk for " << m_elt.name() << " with num values = " << m_numValues << "!\n";
    std::cerr << "Num values = " << m_numValues << "!\n";
    compress();

    Page p;
    p.uncompressedSize = m_uncompressedSize;
    p.compressedSize = m_compressedSize;
    p.numValues = (int32_t)m_numValues;
    writeDataPageHeader(p, m_pageData);

    m_uncompressedTotalSize = m_pageData.size() + m_uncompressedSize;
    m_compressedTotalSize = m_pageData.size() + m_compressedSize;
}

void ColumnChunk::write(std::ofstream& out)
{
std::cerr << "Writing column chunk for " << m_elt.name() << " at " << out.tellp() << "!\n";
    setOffset(out.tellp());
    out.write(m_pageData.data(), m_pageData.size());
    out.write(m_compressionBuffer.data(), m_compressionBuffer.size());
    m_compressionBuffer.release();
std::cerr << "End column chunk at " << out.tellp() << "!\n";
}
