#include "Parquet.hpp"
#include "Output.hpp"
#include "Page.hpp"

char *writeFooter(const Schema& schema, const RowGroups& rowGroups, int64_t numRows,
    int64_t copcVlrOffset, Output& output);
char *writeDataPageHeader(Page p, Output& output);

Parquet::Parquet(const std::filesystem::path& filename) :
    m_out(filename, std::ios_base::binary)
{
    if (!m_out)
        throw "Couldn't open file '" + std::string(filename) + "' for output.";

    write("PAR1", 4);
}

Schema& Parquet::schema()
{
    return m_schema;
}

int64_t Parquet::position()
{
    return (int64_t)m_out.tellp();
}

void Parquet::addRowGroup(RowGroup&& rg)
{
    // Save row group information to be written as footer.
    m_numRows += rg.numRows();
    m_rowGroups.push_back(std::move(rg));
}

void Parquet::write(const char *c, std::size_t size)
{
    m_out.write(c, size);
}

// Write the footer and such
void Parquet::done()
{
    int64_t copcVlrOffset = 0;
    BackedOutput footer(10'000);

    writeFooter(schema(), m_rowGroups, m_numRows, copcVlrOffset, footer);
    write(footer.data(), footer.size());
    std::cerr << "Footer size = " << footer.size() << "!\n";
    uint32_t footerSize = tole((uint32_t)footer.size());
    write((char *)&footerSize, 4);
    write("PAR1", 4);
}

