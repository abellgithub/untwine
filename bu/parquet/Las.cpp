#include <array>
#include <cstddef>

#include "Parquet.hpp"
#include "Schema.hpp"

int main()
{
    Parquet parquet("file.parquet");
    Schema& schema = parquet.schema();

    SchemaElt& x = schema.addColumn<ParquetType::INT32>("X");
    SchemaElt& y = schema.addColumn<ParquetType::INT32>("Y");
    SchemaElt& z = schema.addColumn<ParquetType::INT32>("Z");

    struct Las
    {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    std::array<Las, 5> las
    {
        {{ 1, 2, 3},
        { -1, -2, -3},
        { 4, 5, 6},
        { -4, -5, -6},
        { 10, 20, 30}}
    };

    //ABELL - Integrate this into addColumn?
    x.setOffset(offsetof(Las, x));
    y.setOffset(offsetof(Las, y));
    z.setOffset(offsetof(Las, z));

    RowGroup rg(parquet);

    // Add rows to the row group.
    for (Las& l: las)
        rg.add(&l);
    rg.done();

    parquet.done();

    return 0;
}
