#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

#include "ColumnChunk.hpp"
#include "Output.hpp"
#include "Page.hpp"
#include "RowGroup.hpp"

// Thrift type for structs.
enum class ThriftType
{
    BOOLEAN_TRUE = 1,
    BOOLEAN_FALSE = 2,
    I8 = 3,
    I16 = 4,
    I32 = 5,
    I64 = 6,
    DOUBLE = 7,
    BINARY = 8,
    STRING = 8,
    LIST = 9,
    SET = 10,
    MAP = 11,
    STRUCT = 12,
    UNION = 12,
    UUID = 13
};

// Convert to a representation where small values just have low-order bits set, regardless
// of sign.
uint32_t zigzag(int32_t value)
{
    // Move the sign bit all the way right and change representation:
    // 0 -> 0
    // -1 -> 1
    // 1 -> 2
    // -2 -> 3
    // 2 -> 4 ...
    return (static_cast<uint32_t>(value) << 1) ^ (value >> 31);
}

uint16_t zigzag(int16_t value)
{
    return (static_cast<uint16_t>(value) << 1) ^ (value >> 15);
}

uint64_t zigzag(int64_t value)
{
    return (static_cast<uint64_t>(value) << 1) ^ (value >> 63);
}

// Write bits in 7 bit chunks until all that's left to write are zeros, which we don't write.
// The high bit in each written byte indicates that more bytes follow.
template<typename UINT>
requires (std::same_as<UINT, uint16_t> || std::same_as<UINT, uint32_t> ||
          std::same_as<UINT, uint64_t>)
void writeVarint(UINT value, Output& o)
{
    // Write value in 7-bit chunks, setting the high bit if more bytes follow.
    while (true)
    {
        // If all bits above the low 7 are 0, this is the last byte.
        if ((value & ~0x7F) == 0)
        {
            o.put(static_cast<int8_t>(value));
            break;
        }
        else
        {
            // Set the high bit to indicate more bytes to follow.
            o.put(static_cast<int8_t>((value & 0x7F) | 0x80));
            value >>= 7;
        }
    }
}

void writeString(const std::string& s, Output& o)
{
    // Size followed by non-null-terminated string.
    writeVarint((uint32_t)s.size(), o);
    o.put(s.data(), s.size());
}

class Container
{
public:
    Container(Output& output) : output(output)
    {}

    Container(const Container& parent) : output(parent.output)
    {}

    void write(int16_t ivalue)
    {
        writeVarint(zigzag(ivalue), output);
    }
    void write(int32_t ivalue)
    {
        writeVarint(zigzag(ivalue), output);
    }
    void write(int64_t ivalue)
    {
        writeVarint(zigzag(ivalue), output);
    }
    void write(const std::string& val)
    {
        writeString(val, output);
    }

    void done()
    {
        output.put(0);
    }

protected:
    Output& output;
};

struct Struct : public Container
{
    Struct(Output& output) : Container(output), lastId(0)
    {}

    Struct(const Container& parent) : Container(parent), lastId(0)
    {}

    void writeHeader(int16_t id, ThriftType type)
    {
        int32_t delta = id - lastId;
        if (delta > 0 && delta < 16)
        {
            uint8_t header = (delta << 4) | (uint8_t)type;
            output.put(header);
        }
        else
        {
            // High 4 bits are 0 because the type values are small.
            output.put((uint8_t)type);
            write(id);  // For some reason this value is zigzag written.
        }
        lastId = id;
    }

    int16_t lastId;
};

struct List : public Container
{
    List(const Container& parent) : Container(parent)
    {}

    void writeHeader(uint32_t size, ThriftType type)
    {
        if (size < 0)
            throw "Invalid negative list size.";

        if (size <= 14)
        {
            output.put((size << 4) | (uint8_t)type);
        }
        else
        {
            output.put(0xF0 | (uint8_t)type);
            writeVarint(size, output);
        }
    }
};

void writeSchemaElt(const SchemaElt& col, const Container& parent)
{
    Struct s(parent);
    if (col.numChildren() == 0)
    {
        s.writeHeader(1, ThriftType::I32);
        s.write((int32_t)col.type());
    }

    if (col.type() == ParquetType::FIXED_LEN_BYTE_ARRAY)
    {
        s.writeHeader(2, ThriftType::I32);
        s.write((int32_t)col.size());
    }

    // Don't write for root
    if (!col.isRoot())
    {
        s.writeHeader(3, ThriftType::I32);
        s.write((int32_t)0);  // Required & unique element.
    }

    s.writeHeader(4, ThriftType::STRING);
    s.write(col.name());

    if (col.numChildren())
    {
        s.writeHeader(5, ThriftType::I32);
        s.write((int32_t)col.numChildren());
    }

    s.done();
}

void writeColumnMetadata(const ColumnChunk& col, const Container& parent)
{
    // Type (repeat from schema)
    Struct s(parent);

    s.writeHeader(1, ThriftType::I32);
    s.write((int32_t)col.type());

    // Encodings - our column is simple plain data.
    s.writeHeader(2, ThriftType::LIST);
    {
        List l(s);
        l.writeHeader(1, ThriftType::I32);
        l.write((int32_t)Encoding::PLAIN);
    }

    // Path-in-schema - unused but required.
    s.writeHeader(3, ThriftType::LIST);
    {
        List l(s);
        l.writeHeader(0, ThriftType::STRING);
    }

    // Compression
    s.writeHeader(4, ThriftType::I32);
    s.write((int32_t)col.compression());

    // Number of values
    s.writeHeader(5, ThriftType::I64);
    s.write(col.numValues());

    // Uncompressed total size
    s.writeHeader(6, ThriftType::I64);
    s.write(col.uncompressedTotalSize());

    // Compressed total size
    s.writeHeader(7, ThriftType::I64);
    s.write(col.compressedTotalSize());

    // 8 is key/value metadata

    // Data page offset
std::cerr << "Write column byte size = " << col.compressedTotalSize() << "!\n";
std::cerr << "Write data page offset = " << col.offset() << "!\n";
    s.writeHeader(9, ThriftType::I64);
    s.write(col.offset());

    s.done();
}

void writeRowGroup(const RowGroup& rowGroup, const Container& parent)
{
    Struct s(parent);

    s.writeHeader(1, ThriftType::LIST);
    {
        List l(s);
        l.writeHeader((uint32_t)rowGroup.columns().size(), ThriftType::STRUCT);
        for (const ColumnChunk& colChunk : rowGroup.columns())
        {
            Struct cc(l);

            cc.writeHeader(3, ThriftType::STRUCT);
            writeColumnMetadata(colChunk, cc);

            cc.writeHeader(2, ThriftType::I64);
            cc.write((int64_t)0);
            cc.done();
        }
    }

    s.writeHeader(2, ThriftType::I64);
    s.write(rowGroup.totalByteSize());

    s.writeHeader(3, ThriftType::I64);
    s.write(rowGroup.numRows());

    s.writeHeader(5, ThriftType::I64);
    s.write(rowGroup.offset());

    s.writeHeader(6, ThriftType::I64);
    s.write(rowGroup.compressedTotalSize());

    s.done();
}

void writeDataPageHeader(Page page, Output& parent)
{
    // Page header.

    // Page type.

    Struct s(parent);
    s.writeHeader(1, ThriftType::I32);
    s.write(3);  // Data page V2 is 3.

    s.writeHeader(2, ThriftType::I32);
    s.write(page.uncompressedSize);

    s.writeHeader(3, ThriftType::I32);
    s.write(page.compressedSize);

    s.writeHeader(8, ThriftType::STRUCT);

        // Data page header is a separate struct.
        Struct dp(s);

        dp.writeHeader(1, ThriftType::I32);
        dp.write(page.numValues);

        // No nulls.
        dp.writeHeader(2, ThriftType::I32);
        dp.write(0);

        // Num rows.
        dp.writeHeader(3, ThriftType::I32);
        // Why would num_rows every be different from num_values?
        dp.write(page.numValues);

        // Data encoding.
        dp.writeHeader(4, ThriftType::I32);
        dp.write((int32_t)Encoding::PLAIN);

        // Definition level length.
        dp.writeHeader(5, ThriftType::I32);
        dp.write(0);

        // Repetition level length.
        dp.writeHeader(6, ThriftType::I32);
        dp.write(0);

        // Compression yes/no.
        dp.writeHeader(7, ThriftType::BOOLEAN_FALSE);

        dp.done();

    s.done();
}

void writeFooter(const Schema& schema, const RowGroups& rowGroups, int64_t numRows,
    int64_t copcHierOffset, int64_t copcHierSize, Output& output)
{
    Struct s(output);

    // Version
    s.writeHeader(1, ThriftType::I32);
    s.write((int32_t)2);

    // Schema is a list of structs.
    s.writeHeader(2, ThriftType::LIST);
    {
        List l(s);
        l.writeHeader((uint32_t)schema.elements().size() + 1, ThriftType::STRUCT);

        writeSchemaElt(RootSchemaElt(schema.elements().size()), l);
        for (const SchemaElt& col : schema.elements())
            writeSchemaElt(col, l);
    }

    std::cerr << "Num rows = " << numRows << "!\n";
    s.writeHeader(3, ThriftType::I64);
    s.write(numRows);

    s.writeHeader(4, ThriftType::LIST);
    {
        List l(s);
        l.writeHeader((uint32_t)rowGroups.size(), ThriftType::STRUCT);
        for (const RowGroup& rg : rowGroups)
            writeRowGroup(rg, l);
    }

    s.writeHeader(5, ThriftType::LIST);
    {
        List l(s);
        l.writeHeader(2, ThriftType::STRUCT);
            Struct kv(l);
            kv.writeHeader(1, ThriftType::STRING);
            kv.write("pcopc_hierarchy_offset");
            kv.writeHeader(2, ThriftType::STRING);
            kv.write(std::to_string(copcHierOffset));
            kv.done();

            kv.writeHeader(1, ThriftType::STRING);
            kv.write("pcopc_hierarchy_size");
            kv.writeHeader(2, ThriftType::STRING);
            kv.write(std::to_string(copcHierSize));
            kv.done();
    }

    s.writeHeader(6, ThriftType::STRING);
    s.write("pcparquet version 1.0 (build XXX)");
    return s.done();
}
