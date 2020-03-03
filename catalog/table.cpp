#include "table.h"
#include "index.h"
#include <cstring>
#include <cassert>

table_t::table_t()
{
    primaryKey = -1;
    tupleCount = 0;
    tupleLength = -1;
}

// 创建表时使用
table_t::table_t(string name, const string &key, vector<field_t> _fields)
    : name(std::move(name)), fields(std::move(_fields))
{
    tupleCount = 0;
    int tol = 0, tot = 0;
    primaryKey = -1;

    for (auto &f : fields)
    {
        if (f.name == key)
        {
            f.unique = true;
            primaryKey = tot;
        }

        f.offset = tol;
        tol += f.length, tot++;
    }

    tupleLength = tol;
}

// 反序列化表时使用
table_t::table_t(string name, int key, const vector<field_t> &_fields, const vector<index_t> &_indexs, int tupleCount)
    : table_t(std::move(name), _fields[key].name, _fields)
{
    this->tupleCount = tupleCount;
    for (auto &i : _indexs)
        indexs[i.indexName] = i;
}

table_t::table_t(const table_t &a) noexcept
    : table_t(a.name, a.fields[a.primaryKey].name, a.fields)
{
    tupleCount = a.tupleCount;
    indexs = a.indexs;
}

int table_t::getFieldLength() const
{
    return fields.size();
}

int table_t::getTupleLength() const
{
    return tupleLength;
}

int table_t::getTupleCount() const
{
    return tupleCount;
}

field table_t::getField(int i)
{
    return fields[i];
}

string table_t::getName() const
{
    return name;
}

void table_t::serialize(FILE *fp)
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
    buf[0] = '@', buf[1] = 'T';
    sprintf(buf+4, "%s", name.data());
    fwrite(buf, sizeof(buf), 1, fp);
    int tof[4];
    tof[0] = primaryKey;
    tof[1] = tupleCount;
    tof[2] = fields.size();
    tof[3] = indexs.size();
    fwrite(tof, sizeof(int), 4, fp);
    for (auto &i : fields) i.serialize(fp);
    for (auto &i : indexs) i.second.serialize(fp);
}

table_t table_t::deserialize(FILE *fp)
{
    char buf[32]; int tof[4];
    fread(buf, sizeof(buf), 1, fp);
    fread(tof, sizeof(int), 4, fp);
    assert(buf[0] == '@' && buf[1] == 'T');
    string name(buf+4);
    vector<field_t> fields(tof[2]);
    for (int i = 0; i < tof[2]; i++)
        fields[i] = field_t::deserialize(fp);
    vector<index_t> indexs(tof[3]);
    for (int i = 0; i < tof[3]; i++)
        indexs[i] = index_t::deserialize(fp, name, fields);
    return table_t(name, tof[0], fields, indexs, tof[1]);
}
