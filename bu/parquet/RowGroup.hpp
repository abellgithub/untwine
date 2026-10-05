#pragma once

#include "ColumnChunk.hpp"

class Parquet;

struct RowGroup
{
    friend class Parquet;

public:
    RowGroup(Parquet& parquet);

    void init();
    void add(void *v);
    void done();
    const ColumnChunks& columns() const
    { return m_columnChunks; }
    int64_t totalByteSize() const
    { return m_totalByteSize; }
    int64_t numRows() const
    { return m_numRows; }
    int64_t offset() const
    { return m_fileOffset; }
    int64_t compressedTotalSize() const
    { return m_compressedTotalSize; }

private:
    Parquet& m_file;
    ColumnChunks m_columnChunks;
    int64_t m_totalByteSize = 0;
    int64_t m_numRows = 0;
    int64_t m_fileOffset = 0;
    int64_t m_compressedTotalSize = 0;
};
using RowGroups = std::deque<RowGroup>;
