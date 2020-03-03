#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "field.h"
#include "index.h"
using std::string;
using std::vector;
using std::unordered_map;

class table_t
{
private:
    string name;
    int tupleCount;
    int tupleLength;
    friend class catalog_t;

public:
    int primaryKey;
    vector<field_t> fields;
    string getName() const;
    field getField(int i);
    int getFieldLength() const;
    int getTupleLength() const;
    int getTupleCount() const;
    unordered_map<string, index_t> indexs;
    //const vector<field_t> getFields() const;

    table_t();
    table_t(const table_t &) noexcept;
    table_t(string name, const string &key, vector<field_t> fields);
    table_t(string name, int key, const vector<field_t> &fields, const vector<index_t> &indexs, int tupleCount);

    static table_t deserialize(FILE* fp);
    void serialize(FILE* fp);
};

typedef table_t &table;
