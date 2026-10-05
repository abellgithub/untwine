#pragma once

#include <filesystem>
#include <fstream>

#include "Schema.hpp"
#include "RowGroup.hpp"

struct Page;

class Parquet
{
public:
    Parquet(const std::filesystem::path& filename);

    Schema& schema();
    int64_t position();
    void addRowGroup(RowGroup&& rg);
    void write(Page p, Buffer& buf);
    void write(const char *c, std::size_t size);
    void done();

private:
    std::ofstream m_out;
    Schema m_schema;
    RowGroups m_rowGroups;
    int64_t m_position = 0;
    int64_t m_numRows = 0;
};
