//
// $ g++ -std=c++20 pt.cpp -I/Users/abell/arrowinstall/include -L/Users/abell/arrowinstall/lib -lparquet -larrow -o pt
// $ DYLD_LIBRARY_PATH=/Users/abell/arrowinstall/lib:/Users/abell/thriftinstall/lib:${CONDA_PREFIX}/lib ./pt ../build/foo.parquet 2>&1 | less
//

#include <iostream>
#include <iomanip>
#include <string>

#include "parquet/file_reader.h"
#include "parquet/column_reader.h"
#include "arrow/util/key_value_metadata.h"

using ParquetFileReaderPtr = std::unique_ptr<parquet::ParquetFileReader>;
using RowGroupReaderPtr = std::shared_ptr<parquet::RowGroupReader>;
using FileMetaDataPtr = std::shared_ptr<parquet::FileMetaData>;
using KVMetadataPtr = std::shared_ptr<const parquet::KeyValueMetadata>;
using RowGroupMetaDataPtr = std::unique_ptr<parquet::RowGroupMetaData>;
using ColumnReaderPtr = std::shared_ptr<parquet::ColumnReader>;

// Read the column data for a row group.
template<typename ReaderType, typename DataType>
void readData(ColumnReaderPtr baseReader, int64_t numRows, double& total)
{
    using ReaderTypePtr = std::shared_ptr<ReaderType>;

    ReaderTypePtr columnReader = static_pointer_cast<ReaderType>(baseReader);

    int64_t cnt;
    const size_t batchSize = 100;
    std::vector<DataType> values(batchSize);
    int outcnt = 0;
    while (numRows > 0)
    {
        int64_t levels = columnReader->ReadBatch(batchSize, nullptr, nullptr, values.data(), &cnt);

        for (int j = 0; j < cnt; j++)
        {
            double val = values[j];
            total += val;
        }
        numRows -= cnt;
    }
}

int main(int argc, char *argv[])
{
    using namespace parquet;

    if (argc != 2)
    {
        std::cerr << "pt <filename>!\n";
        return -1;
    }
    std::string filename = argv[1];

        ParquetFileReaderPtr reader_ = ParquetFileReader::OpenFile(filename);

        FileMetaDataPtr metadata_ = reader_->metadata();
        int64_t numColumns = metadata_->num_columns();
        int64_t numSchemaElements = metadata_->num_schema_elements();
        int64_t numRows = metadata_->num_rows();
        int64_t numRowGroups = metadata_->num_row_groups();

        std::cerr << "Num columns = " << numColumns << "!\n";
        std::cerr << "Num schema elts = " << numSchemaElements << "!\n";
        std::cerr << "Num rows = " << numRows << "!\n";
        std::cerr << "Num row groups = " << numRowGroups << "!\n";

        std::cerr << "Created by = " << metadata_->created_by() << "!\n";
        const KVMetadataPtr kv = metadata_->key_value_metadata();
        for (size_t i = 0; i < kv->size(); ++i)
            std::cerr << "Metadata key/value = " << kv->key(i) << "/" << kv->value(i) << "!\n";
        std::cout << "\n";

        const SchemaDescriptor *s = metadata_->schema();
        for (size_t c = 0; c < s->num_columns(); c++)
        {
            const ColumnDescriptor *col = s->Column(c);
            std::cerr << "Column '" << col->name() << "'!\n";
            std::cerr << "\tType = " << TypeToString(col->physical_type()) << "!\n";
            std::cerr << "\tType Length = " << GetTypeByteSize(col->physical_type()) << "!\n";
        }

        std::vector<double> colTotals(numColumns);

std::cerr << "Num row groups = " << numRowGroups << "!\n";
        for (size_t r = 0; r < numRowGroups; ++r)
        {
            RowGroupReaderPtr rgReader = reader_->RowGroup(r);
            RowGroupMetaDataPtr rg = metadata_->RowGroup(r);
            for (size_t c = 0; c < numColumns; ++c)
            {
                ColumnReaderPtr columnReader = rgReader->Column(c);
                const ColumnDescriptor *col = s->Column(c);
                switch (col->physical_type())
                {
                    case Type::INT32:
                        readData<Int32Reader, int32_t>(columnReader, rg->num_rows(), colTotals[c]);
                        break;
                    case Type::INT64:
                        readData<Int64Reader, int64_t>(columnReader, rg->num_rows(), colTotals[c]);
                        break;
                    case Type::FLOAT:
                        readData<FloatReader, float>(columnReader, rg->num_rows(), colTotals[c]);
                        break;
                    case Type::DOUBLE:
                        readData<DoubleReader, double>(columnReader, rg->num_rows(), colTotals[c]);
                        break;
                    default:
                        std::cerr << "Found unexpected column type = " <<
                            TypeToString(col->physical_type()) << "!\n";
                        return -1;

                }
            }
        }

        for (size_t c = 0; c < numColumns; ++c)
        {
            const ColumnDescriptor *col = s->Column(c);
            std::cerr << std::setprecision(9) << col->name() << " avg = " <<
                (colTotals[c] / numRows) << "!\n";
        }

    return 0;
}
