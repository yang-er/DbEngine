#pragma once

#include <string>
#include <vector>
#include "field.h"
using std::string;
using std::vector;

class index_t
{
public:
    string indexName;
    string tableName;
    string fieldName;
    int columnId;
    int columnLength;
    int rootNumber;
    int blockCount;
    char columnType;

    index_t();
    index_t(string indexName, string tableName, field field, int colId, int blockCount = 0, int rootNubmer = 0);

    static index_t deserialize(FILE *fp, const string &tname, vector<field_t> &fields);
    void serialize(FILE *fp);
};

typedef index_t &index;
