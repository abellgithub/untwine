#pragma once

#include <filesystem>

#include "Schema.hpp"
#include "RowGroup.hpp"

struct Page;

class Parquet
{
public:
    static const std::ios_base::openmode FileBits = std::ios::in | std::ios::out | std::ios::binary;

    Parquet(const std::filesystem::path& filename);

    Schema& schema();
    void addRowGroup(RowGroup&& rg);
    void done();
    const std::filesystem::path& name() const
    { return m_filename; }

private:
    std::filesystem::path m_filename;
    Schema m_schema;
    RowGroups m_rowGroups;
    int64_t m_numRows = 0;
};
