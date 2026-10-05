#pragma once

#include <cstdint>
#include <string>
#include <deque>

#include "Definitions.hpp"

class SchemaElt
{
public:
    SchemaElt(const std::string& name, ParquetType type = ParquetType::NONE,
              uint32_t length = (std::numeric_limits<uint32_t>::max)()) :
        _name(name), _elementType(type), _typeLength(length), _offset(0), _numChildren(0)
    {
        if (_typeLength == (std::numeric_limits<uint32_t>::max)())
            _typeLength = ParquetTypeSize[(int)_elementType];
    }
    virtual ~SchemaElt()
    {}

    void setNumChildren(size_t numChildren)
    { _numChildren = numChildren; }
    void setOffset(size_t offset)
    { _offset = offset; }
    std::size_t offset() const
    { return _offset; }
    std::size_t size() const
    { return _typeLength; }
    ParquetType type() const
    { return _elementType; }
    std::string name() const
    { return _name; }
    std::size_t numChildren() const
    { return _numChildren; }
    virtual bool isRoot() const // Need a vtable.
    { return false; }


private:
    std::string _name;
    ParquetType _elementType;
    uint32_t _typeLength; // Same as ParquetTypeSize unless FIXED_LEN_BYTE_ARRAY.
    std::size_t _offset;
    std::size_t _numChildren;
};

class RootSchemaElt : public SchemaElt
{
public:
    RootSchemaElt(size_t numChildren) : SchemaElt("root")
    {
        setNumChildren(numChildren);
    }

    bool isRoot() const override
    { return true; }
};

class Schema
{
public:
    template<ParquetType TYPE>
    requires (TYPE != ParquetType::FIXED_LEN_BYTE_ARRAY)
    SchemaElt& addColumn(const std::string& name)
    {
        m_rowLength += ParquetTypeSize[(int)TYPE];
        return m_elements.emplace_back(name, TYPE, ParquetTypeSize[(int)TYPE]);
    }

    template<ParquetType TYPE>
    requires (TYPE == ParquetType::FIXED_LEN_BYTE_ARRAY)
    SchemaElt& addColumn(const std::string& name, uint32_t length)
    {
        m_rowLength += length;
        return m_elements.emplace_back(name, ParquetType::FIXED_LEN_BYTE_ARRAY, length);
    }

    std::size_t rowLength() const
    { return m_rowLength; }
    const std::deque<SchemaElt>& elements() const
    { return m_elements; }

private:
    std::size_t m_rowLength = 0;
    std::deque<SchemaElt> m_elements;
};
