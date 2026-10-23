#include <fstream>

#include "../../untwine/FatalError.hpp"

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
    m_columnChunks.clear();
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
    // This doesn't appear to include the page headers, but check this.
    m_totalByteSize = m_numRows * m_file.schema().rowLength();
std::cerr << "Total byte size (num rows * rowLength) = " << m_totalByteSize << "(" <<
    m_numRows << " * " << m_file.schema().rowLength() << ")!\n";

    for (ColumnChunk& col : m_columnChunks)
    {
        col.done();
        m_compressedTotalSize += col.compressedTotalSize();
    }
}

void RowGroup::write(Parquet& file, int64_t position)
{
    std::ofstream out(file.name(), std::ios::binary | std::ios::in | std::ios::out);
    out.seekp(std::ofstream::pos_type(position));
    for (ColumnChunk& col : m_columnChunks)
        col.write(out);
    out.close();

    if (!out)
        throw untwine::FatalError("Failure writing to file " + file.name().string() +
            " at position " + std::to_string(position) + ".");

    m_file.addRowGroup(std::move(*this));
    m_numRows = -1;
}

