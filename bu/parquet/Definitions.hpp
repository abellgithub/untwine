#pragma once

enum class ParquetType
{
    // Change size below if you add or subtract.
    BOOLEAN,
    INT32,
    INT64,
    INT96,
    FLOAT,
    DOUBLE,
    BYTE_ARRAY,
    FIXED_LEN_BYTE_ARRAY,
    // Change size below if you add or subtract.
    NONE,
};

namespace
{
    const std::array<std::size_t, 9> ParquetTypeSize
    {
        1,
        4,
        8,
        12,
        4,
        8,
        0,
        0,
        0
    };
}

enum class CompressionCodec
{
    UNCOMPRESSED = 0,
    SNAPPY = 1,
    GZIP = 2,
    LZO = 3,
    BROTLI = 4,
    LZ4 = 5,
    ZSTD = 6,
    LZ4_RAW = 7
};

enum class Encoding
{
    PLAIN = 0
};

