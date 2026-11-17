#pragma once

#include <filesystem>

#include "Schema.hpp"
#include "RowGroup.hpp"

struct Page;

class Parquet
{
public:
    Parquet(const std::filesystem::path& filename);

    Schema& schema();
    void addRowGroup(RowGroup&& rg);
    void done(int64_t copcHierOffset, int64_t copcHierSize);
    const std::filesystem::path& name() const
    { return m_filename; }

private:
    std::filesystem::path m_filename;
    Schema m_schema;
    RowGroups m_rowGroups;
    int64_t m_numRows = 0;
};
