#include "RowGroup.hpp"
#include "Page.hpp"
#include "Parquet.hpp"
#include "Schema.hpp"

RowGroup::RowGroup(Parquet& parquet) : m_file(parquet)
{
    init();
}

void RowGroup::init()
{
    for (const SchemaElt& elt : m_file.schema().elements())
        m_columnChunks.push_back(ColumnChunk(elt));

    m_totalByteSize = 0;
    m_numRows = 0;
    m_fileOffset = 0;
    m_compressedTotalSize = 0;
}

void RowGroup::add(void *v)
{
    if (m_numRows < 0)  // Row group already completed.
        return;
    for (ColumnChunk& col : m_columnChunks)
        col.add(v);
    m_numRows++;
}

void RowGroup::done()
{
    m_totalByteSize = m_numRows * m_file.schema().rowLength();
std::cerr << "Total byte size = " << m_totalByteSize << "!\n";

    for (ColumnChunk& col : m_columnChunks)
    {
        col.done(m_file);
        m_compressedTotalSize += col.compressedTotalSize();
    }

    m_file.addRowGroup(std::move(*this));
    m_numRows = -1;
}
