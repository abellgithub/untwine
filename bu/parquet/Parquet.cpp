#include <fstream>

#include "Parquet.hpp"
#include "Output.hpp"
#include "Page.hpp"
#include "../../untwine/FatalError.hpp"

char *writeFooter(const Schema& schema, const RowGroups& rowGroups, int64_t numRows,
    int64_t copcVlrOffset, Output& output);
char *writeDataPageHeader(Page p, Output& output);

Parquet::Parquet(const std::filesystem::path& filename) :
    m_filename(filename)
{
    std::ofstream out(m_filename, std::ios::out | std::ios::binary);
    out.write("PAR1", 4);
    if (!out)
        throw untwine::FatalError("Couldn't open file '" + std::string(filename) + "' for output.");
}

Schema& Parquet::schema()
{
    return m_schema;
}

void Parquet::addRowGroup(RowGroup&& rg)
{
    std::cerr << "Add row group!\n";
    // Save row group information to be written as footer.
    m_numRows += rg.numRows();
    m_rowGroups.push_back(std::move(rg));
}

// Write the footer and such
void Parquet::done()
{
    int64_t copcVlrOffset = 0;
    BackedOutput footer(10'000);

    writeFooter(schema(), m_rowGroups, m_numRows, copcVlrOffset, footer);

    std::ofstream out(m_filename, std::ios::in | std::ios::out | std::ios::binary);
    out.seekp(0, std::ios_base::end);
    out.write(footer.data(), footer.size());
    std::cerr << "Footer size = " << footer.size() << "!\n";
    uint32_t footerSize = tole((uint32_t)footer.size());
    out.write((char *)&footerSize, 4);
    out.write("PAR1", 4);
}

