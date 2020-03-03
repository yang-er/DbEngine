#include "index.h"
#include <cstring>
#include <cassert>

index_t::index_t()
{
    columnId = -1;
    columnLength = -1;
    rootNumber = -1;
    blockCount = -1;
    columnType = -1;
}

index_t::index_t(string indexName, string tableName, field field, int colId, int blockCount, int rootNubmer)
    : indexName(std::move(indexName)), tableName(std::move(tableName)), fieldName(field.name)
{
    this->blockCount = blockCount;
    this->rootNumber = rootNubmer;
    this->columnId = colId;
    this->columnLength = field.length;
    this->columnType = field.type;
}

index_t index_t::deserialize(FILE *fp, const string &tname, vector<field_t> &fields)
{
    char buf[48];
    memset(buf, 0, sizeof(buf));
    fread(buf, 48, 1, fp);
    assert(buf[0] == '@' && buf[1] == 'I');
    int* arr = reinterpret_cast<int*>(buf+2);
    int cid = arr[0], rnu = arr[1], bcn = arr[2];
    string in(buf+14);
    return index_t(in, tname, fields[cid], cid, bcn, rnu);
}

void index_t::serialize(FILE *fp)
{
    fputc('@', fp), fputc('I', fp);
    fwrite(&columnId, sizeof(int), 1, fp);
    fwrite(&rootNumber, sizeof(int), 1, fp);
    fwrite(&blockCount, sizeof(int), 1, fp);
    char buf[34];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s", indexName.data());
    fwrite(buf, 34, 1, fp);
}
